/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the 
 *     GET method to serve static and dynamic content.
 */
#include "csapp.h"
#include "sbuf.h"
#include "cache.h"

#define NTHREADS  4
#define SBUFSIZE  16

#define min(a,b)    (((a) < (b)) ? (a) : (b))

// TODO 将本程序中使用的csapp.c中的包装函数替换一下，使得出现错误不要exit，而是调用clienterror返回客户端错误信息。

sbuf_t sbuf; /* Shared buffer of connected descriptors */
Cache cache;

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void doit(int fd);
void read_forward_response(rio_t *rio2, int fd1, char *url);
void response_to_client(int fd, char *url, char *data, int length, char *type);
void read_forward_requesthdrs(rio_t *rio1, int fd2, char *host);
int parse_url(char *url, char *host, char *port, char *uri);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void *thread(void *vargp);

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    pthread_t tid;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // 如果尝试两次发送数据到一个已经被对方关闭的 socket 上时，内核会发送一个 SIGPIPE 信号给程序。
    // 在默认情况下，会终止当前程序，显然不是我们想要的，所以要忽略它。
    Signal(SIGPIPE, SIG_IGN);

    cache_init(&cache);

    listenfd = Open_listenfd(argv[1]);

    sbuf_init(&sbuf, SBUFSIZE);
    for (int i = 0; i < NTHREADS; i++)
        Pthread_create(&tid, NULL, thread, NULL);

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        sbuf_insert(&sbuf, connfd); /* Insert connfd in buffer */
    }
}

void *thread(void *vargp) 
{  
    // 分离线程，使得当线程执行完后可以被OS回收，否则如果进程自己没有回收且开启的线程
    // 不是分离的话，这些执行完的线程就会一直占用内存资源。
    Pthread_detach(pthread_self()); 
    while (1) { 
        int connfd = sbuf_remove(&sbuf); /* Remove connfd from buffer */
        doit(connfd);
        Close(connfd);
    }
}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd1) 
{
    // When an end user enters a URL such as http://www.cmu.edu/hub/index.html into the address
    // bar of a web browser, the browser will send an HTTP request to the proxy that begins with a line that might
    // resemble the following:
    // GET http://www.cmu.edu/hub/index.html HTTP/1.1
    // In that case, the proxy should parse the request into at least the following fields: the hostname, www.cmu.edu;
    // and the path or query and everything following it, /hub/index.html. That way, the proxy can determine that
    // it should open a connection to www.cmu.edu and send an HTTP request of its own starting with
    // a line of the following form:
    // GET /hub/index.html HTTP/1.0
    // Modern web browsers will generate HTTP/1.1 requests, but
    // your proxy should handle them and forward them as HTTP/1.0 requests.
    char buf[MAXLINE], method[MAXLINE], url[MAXLINE], uri[MAXLINE], host[MAXLINE], port[32];
    rio_t rio1, rio2;
    int fd2;

    Rio_readinitb(&rio1, fd1);
    if (!Rio_readlineb(&rio1, buf, MAXLINE)) // read request line
        return;
    printf("request line:\n%s", buf);
    sscanf(buf, "%s %s %*s", method, url);

    char *data = NULL;
    char *type = NULL;
    int length = 0;
    cache_read(&cache, url, &data, &length, &type);
    if (data != NULL) {
        response_to_client(fd1, url, data, length, type);
        return;
    }

    char url2[MAXLINE];
    strcpy(url2, url);
    int have_port = parse_url(url, host, port, uri);
    printf("host: %s\nport: %s\nuri: %s\n", host, port, uri);
    if (have_port) {
        fd2 = Open_clientfd(host, port);
    } else {
        fd2 = Open_clientfd(host, "80");
    }
    Rio_readinitb(&rio2, fd2);
    sprintf(buf, "%s %s HTTP/1.0\r\n", method, uri);
    Rio_writen(fd2, buf, strlen(buf)); // write request line

    read_forward_requesthdrs(&rio1, fd2, host);

    // rio_readlineb底层是对socket fd调用read系统调用，当socket fd不可读时（在该socket fd上未有数据到达），
    // read系统调用会一直阻塞，即这里也会一直阻塞。
    read_forward_response(&rio2, fd1, url2);

    Close(fd2);
}

void read_forward_requesthdrs(rio_t *rio1, int fd2, char *host) 
{
    char buf1[MAXLINE];
    char buf2[MAXBUF];
    int host_hdr_forwarded = 0;

    // 因为读完请求行后，至少有一个\r\n，所以可以直接先读。
    do {
        Rio_readlineb(rio1, buf1, MAXLINE);
        if (strncmp("Host", buf1, 4) == 0) {
            // The Host header describes the hostname of the end server.
            sprintf(buf2, "%s", buf1);
            host_hdr_forwarded = 1;
        }
    } while(strcmp(buf1, "\r\n"));
    if (host_hdr_forwarded == 0) {
        sprintf(buf2, "%sHost: %s\r\n", buf2, host);
    }
    // The User-Agent header identifies the client (in terms of parameters such as the operating system
    // and browser), and web servers often use the identifying information to manipulate the content they serve.
    // The Connection and Proxy-Connection headers are used to specify whether a connection
    // will be kept alive after the first request/response exchange is completed. 
    // 注意以\r\n分隔请求头和请求体或以\r\n标识整个请求文本（如果没有body的话）已结束。
    sprintf(buf2, "%s%sConnection: close\r\nProxy-Connection: close\r\n\r\n", buf2, user_agent_hdr);
    Rio_writen(fd2, buf2, strlen(buf2));
    return;
}

void read_forward_response(rio_t *rio2, int fd1, char *url) {
    char buf2[MAXLINE];
    char buf1[MAXBUF];
    char body[MAXBUF];
    char content_type[MAXLINE];
    int content_length = 0;
    *buf1 = '\0';
    do {
        Rio_readlineb(rio2, buf2, MAXLINE);
        if (strncmp("Content-length", buf2, 14) == 0) {
            sscanf(buf2, "Content-length: %d", &content_length);
        } else if (strncmp("Content-type", buf2, 12) == 0) {
            sscanf(buf2, "Content-type: %s\r\n", content_type);
        }
        sprintf(buf1, "%s%s", buf1, buf2);
    } while(strcmp(buf2, "\r\n"));
    // sprintf(buf1, "%s\r\n", buf1); // 加一个\r\n结束header，也可能是结束整个请求文本，如果没有body的话。
    // 不需要再添加\r\n了，因为上面循环已经加了，再加就重复了，而多出来的\r\n就会“挤掉”body的内容，使得客户端以为多出来的
    // \r\n是响应体，最终如果用vim打开请求的文本文件，就会在第一行看到^M，vim会用^M替换无法显示的\r。
    Rio_writen(fd1, buf1, strlen(buf1)); // header+body可能超过MAXBUF，所以分两次写入。

    char bufc[MAX_OBJECT_SIZE];
    int i = 0;
    
    ssize_t n;
    while (content_length > 0) {
        // 因为响应体可能是二进制内容而不是文本内容，所以就不一行一行读了。
        // ▪ rio_readn returns short count only if it encounters EOF
        // ▪ Only use it when you know how many bytes to read
        // ▪ rio_writen never returns a short count
        // 注意不能混用带缓存的读和不带缓存的读，因为可能"abcde"中"ab"已经被读入rio结构体中的缓冲区了，
        // 这时再用不带缓存的读去读，就只会读到"cde"而无法读到"ab"。因此这里要用rio_readn的缓存版本rio_readnb。
        //
        // 注意还要考虑content_length大于MAXBUF的情况，这时如果直接读content_length个字节，body缓冲区就会溢出了。
        n = Rio_readnb(rio2, body, min(content_length, MAXBUF));
        Rio_writen(fd1, body, n);
        content_length -= n;
        memcpy(bufc+i, body, n);
        i += n;
    }
    if (content_length <= MAX_OBJECT_SIZE) {
        cache_write(&cache, url, bufc, i, content_type);
    }
}

int parse_url(char *url, char *host, char *port, char *uri) {
    int have_port = 0;
    if (strstr(url, "http://") != NULL) {
        url += 7;
    }
    char *p = strchr(url, '/');
    strcpy(uri, p);
    *p = '\0';
    if ((p=strchr(url, ':')) != NULL) {
        strcpy(port, p+1);
        *p = '\0';
        have_port = 1;
    }
    strcpy(host, url);
    return have_port;
}

void response_to_client(int fd, char *url, char *data, int length, char *type) {
    char buf[MAXLINE];

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: %s\r\n", type);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", length);
    Rio_writen(fd, buf, strlen(buf));
    if (data != NULL) {
        Rio_writen(fd, data, length);
    }
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
