#include "csapp.h"
#include "cache.h"
#include <limits.h>

void cache_init(Cache *cache) {
    Sem_init(&cache->mutex, 0, 1);
    Sem_init(&cache->w, 0, 1);
    cache->nreader = 0;
    cache->sets = malloc(S * sizeof(Set));
    for (int i=0; i<S; i++) {
        cache->sets[i].lru = 0;
        cache->sets[i].valid = 0;
    }
}


void cache_read(Cache *cache, char *url, char **data, int *length, char **type) {
    P(&cache->mutex);
    cache->nreader++;
    if (cache->nreader == 1)
        P(&cache->w); // first in, 不让writer获得w锁。
    V(&cache->mutex);

    int max_lru = INT_MIN;
    int hit = 0;
    for (int i=0; i<S; i++) {
        if (cache->sets[i].lru > max_lru) {
            max_lru = cache->sets[i].lru;
        }
        if (cache->sets[i].valid && strcmp(cache->sets[i].url, url) == 0) {
            *data = cache->sets[i].data;
            *length = cache->sets[i].length;
            *type = cache->sets[i].type;
            hit = i;
        }
    }
    // 如果多个线程执行完了上面的循环由于调度同时要从这里开始，也就是得到了相同的max_lru，但访问的是不同的Set，
    // 就会导致lru错误更新。所以其实应该在加锁的区域中再遍历一次得到新的max_lru。
    if (hit) {
        P(&cache->mutex);
        for (int i=0; i<S; i++) {
            if (cache->sets[i].lru > max_lru) {
                max_lru = cache->sets[i].lru;
            }
        }
        cache->sets[hit].lru = max_lru + 1;
        V(&cache->mutex);
    }


    P(&cache->mutex);
    cache->nreader--;
    if (cache->nreader == 0)
        V(&cache->w); // last out
    V(&cache->mutex);
}

void cache_write(Cache *cache, char *url, char *data, int length, char *type) {
    if (length > MAX_OBJECT_SIZE) {
        return;
    }
    P(&cache->w);
    int min_lru = INT_MAX;
    int min_lru_i = -1;
    int max_lru = INT_MIN;
    int hit = 0;
    for (int i=0; i<S; i++) {
        if (cache->sets[i].lru < min_lru) {
            min_lru = cache->sets[i].lru;
            min_lru_i = i;
        }
        if (cache->sets[i].lru > max_lru) {
            max_lru = cache->sets[i].lru;
        }
        if (hit == 0 && cache->sets[i].valid == 0) {
            cache->sets[i].valid = 1;
            cache->sets[i].length = length;
            strcpy(cache->sets[i].url, url);
            strcpy(cache->sets[i].type, type);
            memcpy(cache->sets[i].data, data, length);
            hit = i;
        }
    }
    if (hit) {
        cache->sets[hit].lru = max_lru + 1;
    } else {
        // evict
        cache->sets[min_lru_i].length = length;
        strcpy(cache->sets[min_lru_i].url, url);
        strcpy(cache->sets[min_lru_i].type, type);
        memcpy(cache->sets[min_lru_i].data, data, length);
        cache->sets[min_lru_i].lru = max_lru + 1;
    }
    V(&cache->w);
}
