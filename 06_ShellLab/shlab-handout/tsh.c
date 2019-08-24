/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
			break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
			break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
			break;
		default:
			usage();
		}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {
		
		/* Read command line */
		if (emit_prompt) {
			printf("%s", prompt);
			fflush(stdout);
		}
		if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
			app_error("fgets error");
		if (feof(stdin)) { /* End of file (ctrl-d) */
			fflush(stdout);
			exit(0);
		}
		
		/* Evaluate the command line */
		eval(cmdline);
		fflush(stdout);
		fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
    char *argv[MAXARGS]; // Argument list execve()
    char buf[MAXLINE]; // Holds modified command line
    int bg; // Should the job run in bg or fg?
    pid_t pid; // Process id
    sigset_t mask_all, mask_chld, prev_one;
    sigfillset(&mask_all);
    sigemptyset(&mask_chld);
    sigaddset(&mask_chld, SIGCHLD);

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)
        return; // Ignore empty lines

    // 如果是内置命令则直接在当前进程执行，否则fork子进程execve。
    if (!builtin_cmd(argv)) {
        // • In eval, the parent must use sigprocmask to block SIGCHLD signals before it forks the child,
		// and then unblock these signals, again using sigprocmask after it adds the child to the job list by
		// calling addjob. Since children inherit the blocked vectors of their parents, the child must be sure
		// to then unblock SIGCHLD signals before it execs the new program.
		// The parent needs to block the SIGCHLD signals in this way in order to avoid the race condition where
		// the child is reaped by sigchld handler (and thus removed from the job list) before the parent
		// calls addjob.
        sigprocmask(SIG_BLOCK, &mask_chld, &prev_one); // avoid race
        if ((pid=fork()) < 0)
            unix_error("fork error");
        if (pid == 0) { // Child runs user job
            sigprocmask(SIG_SETMASK, &prev_one, NULL); // unblock SIGCHLD 
            // When you run your shell from the standard Unix shell, your shell is running in the foreground process
			// group. If your shell then creates a child process, by default that child will also be a member of the
			// foreground process group. Since typing ctrl-c sends a SIGINT to every process in the foreground
			// group, typing ctrl-c will send a SIGINT to your shell, as well as to every process that your shell
			// created, which obviously isn’t correct.
			// Here is the workaround: After the fork, but before the execve, the child process should call
			// setpgid(0, 0), which puts the child in a new process group whose group ID is identical to the
			// child’s PID. This ensures that there will be only one process, your shell, in the foreground process
			// group. When you type ctrl-c, the shell should catch the resulting SIGINT and then forward it
			// to the appropriate foreground job (or more precisely, the process group that contains the foreground
			// job).
			// 否则当有ctrl-c时，当前shell进程的sigint_handler就会被不断调用。
			if (setpgid(0, 0) < 0) {
				unix_error("setpgid error");
			}
			if (execve(argv[0], argv, environ) < 0) {
				printf("%s: Command not found.\n", argv[0]);
				exit(EXIT_FAILURE);
			}
		}

        sigprocmask(SIG_BLOCK, &mask_all, NULL);
        if (addjob(jobs, pid, bg?BG:FG, cmdline) != 1)
            app_error("addjob error");
        sigprocmask(SIG_SETMASK, &prev_one, NULL); // Unblock SIGCHLD

        // Parent waits for foreground job to terminate
        if (!bg) {
            waitfg(pid); // 启动一个前台任务意味着阻塞shell
        } else {
            // background job
            printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
        }
    }

    return;
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
		buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
		buf++;
		delim = strchr(buf, '\'');
    }
    else {
		delim = strchr(buf, ' ');
    }

    while (delim) {
		argv[argc++] = buf;
		*delim = '\0';
		buf = delim + 1;
		while (*buf && (*buf == ' ')) /* ignore spaces */
			buf++;
	
		if (*buf == '\'') {
			buf++;
			delim = strchr(buf, '\'');
		}
		else {
			delim = strchr(buf, ' ');
		}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
		return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
		argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
    if (strcmp(argv[0], "quit") == 0) {
        exit(EXIT_SUCCESS);
    } else if (strcmp(argv[0], "jobs") == 0) {
        listjobs(jobs);
		return 1;
    } else if ((strcmp(argv[0], "fg")==0) || (strcmp(argv[0], "bg")==0)) {
		do_bgfg(argv);
        return 1;
    } else {
        return 0;     /* not a builtin command */
    }
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    struct job_t *job;
    if (argv[1]==NULL) {
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
		return;
    }
    if (*argv[1] != '%' && !isdigit(*argv[1])) {
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        return;
    }
    for (int i=1; i<strlen(argv[1]); i++) {
        if (!isdigit(argv[1][i])) {
            printf("%s: argument must be a PID or %%jobid\n", argv[0]);
            return;
		}
    }
    if (*argv[1] == '%') {
        if ((job=getjobjid(jobs, (int)strtol(argv[1]+1, NULL, 10))) == NULL) {
            printf("%s: No such job\n", argv[1]);
            return;
        }
    } else if ((job=getjobpid(jobs, (int)strtol(argv[1], NULL, 10))) == NULL) {
        printf("(%s): No such process\n", argv[1]);
		return;
    }
    kill(-job->pid, SIGCONT);
    if (strcmp(argv[0], "bg") == 0) {
        job->state = BG;
        printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
    } else if (strcmp(argv[0], "fg") == 0) {
        job->state = FG;
        waitfg(job->pid); // runs it foreground，即阻塞shell
    }
    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    // One of the tricky parts of the assignment is deciding on the allocation of work between the waitfg
    // and sigchld handler functions. We recommend the following approach:
    //   - In waitfg, use a busy loop around the sleep functionint status = 0;
    //   - In sigchld handler, use exactly one call to waitpid.
    // While other solutions are possible, such as calling waitpid in both waitfg and sigchld handler,
    // these can be very confusing. It is simpler to do all reaping in the handler.
    while (fgpid(jobs) == pid) {
        sleep(1);
    }
    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
    int status = 0;
    pid_t pid;
    struct job_t *job;
    sigset_t mask_all, prev_all;
    sigfillset(&mask_all);

    while ((pid=waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0) {
		sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
		if (WIFEXITED(status)) {
			if (deletejob(jobs, pid) != 1)
				app_error("deletejob error");
		} else if (WIFSTOPPED(status)) {
			// foreground job被ctrl-z/SIGSTOP(不是来自终端)/SIGTSTP(来自终端)信号暂停。
			if ((job=getjobpid(jobs, pid)) == NULL)
				app_error("getjobpid error");
			job->state = ST;
			printf("Job [%d] (%d) stopped by signal %d\n", job->jid, pid, WSTOPSIG(status));
			// XXX 在信号处理函数中使用C标准I/O是不安全的，这里偷懒就还是用了。
		} else if (WIFSIGNALED(status)) {
			printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, WTERMSIG(status));
			if (deletejob(jobs, pid) != 1)
				app_error("deletejob error");
		} if (WIFCONTINUED(status)) {
			// 子进程收到SIGCONT信号重新启动。
			// 暂时不知道怎么触发。
			printf("SIGCONT\n");
		}
		sigprocmask(SIG_SETMASK, &prev_all, NULL);
    }
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    pid_t pid;
    if ((pid=fgpid(jobs)) == 0) {
        // do nothing if there is no foreground job
        return;
    }
    // forward
    // If pid is less than -1, then sig is sent to every process in the process group whose ID is -pid.
    kill(-pid, sig);
    // 由sigchld_handler负责reap这个子进程并从jobs中删除对应的数据。
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    pid_t pid;
    struct job_t *job;
    sigset_t mask_all, prev_all;
    if ((pid=fgpid(jobs)) == 0) {
        // do nothing if there is no foreground job
        return;
    }
    sigfillset(&mask_all);
    sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    // 修改job的状态
    job = getjobpid(jobs, pid);
    job->state = ST;
    sigprocmask(SIG_SETMASK, &prev_all, NULL);
    // forward
    kill(-pid, sig); // 由于子进程execve后不再继承父进程设置的信号处理函数，所以直接转发SIGTSTP给子进程，子进程就会暂停运行。
    // printf("Job [%d] (%d) stopped by signal 20\n", job->jid, pid);
    // 不要当前shell进程的这里打印，而要在当前shell进程的sigchld_handler中打印，
    // 因为这里只处理键盘来的SIGTSTP信号，而子进程被暂停，OS发给当前shell进程的SIGCHLD信号
    // 包含了引起子进程暂停的来源，包括SIGTSTP, SIGSTOP等。
    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
        // printf("%d %d\n", jobs[i].pid, jobs[i].jid);
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
        // printf("XX %d %d\n", jobs[i].pid, jobs[i].jid);
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
// 只能有一个foreground job，而可以有多个background job。
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}


ssize_t sio_puts(char *s) {
	return write(STDOUT_FILENO, s, sio_strlen(s));
}

static size_t sio_strlen(char *s) {
	int i = 0;
	while (s[i]!='\0')
		++i;
	return i;
}
