#ifndef __CACHE_H__
#define __CACHE_H__
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000 // 1 MB
#define MAX_OBJECT_SIZE 102400 // 100 KB
#define S 10
#define E 10

typedef struct {
    //  In either case, the fact that you don’t have to
    //  implement a strictly LRU eviction policy will give you some flexibility in supporting multiple readers.
    int lru;
    int valid;
    int length; // content_length
    char type[MAXLINE]; // content_type
    char url[MAXLINE];
    char data[MAX_CACHE_SIZE];
} Line;

typedef Line Set; // 为了简单，一个Set只包含一个Line。

typedef struct {
    int nreader;
    sem_t mutex, w; // writer要写，必须取得w锁，而mutex保护reader和writer对nreader, lru的写。
    Set* sets; // 对一个url分配一个Set/Line。
} Cache;

void cache_init(Cache *cache);
void cache_read(Cache *cache, char *url, char **data, int *length, char **type);
void cache_write(Cache *cache, char *url, char *data, int length, char *type);

#endif /* __CACHE_H__ */
