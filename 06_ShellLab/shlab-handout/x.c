#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>

extern char **environ;

void sigint_handler(int sig) {
    // printf("SIGINT\n"); // unsafe
	sio_puts("SIGINT\n");
    exit(EXIT_SUCCESS);
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

int main(int argc, char **argv) {
    pid_t pid;

    signal(SIGINT, sigint_handler);

    if ((pid = fork()) == 0) {
		pause();
    }
    sleep(2);
    kill(pid, SIGINT);
    // 输出SIGINT

    if ((pid = fork()) == 0) {
		argv[0] = "./myspin";
		if (execve(argv[0], argv, environ) < 0)
			printf("%d", errno);
    }
    sleep(2);
    kill(pid, SIGINT);
    // 没有输出SIGINT
    sleep(10);
}
