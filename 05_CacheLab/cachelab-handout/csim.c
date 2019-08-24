#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>

typedef struct {
    int s;   // Number of set index bits (S = 2^s is the number of sets)
    int E;   // Associativity (number of lines per set)
    int b;   // Number of block bits (B = 2^b is the block size)
    int v;   // verbose flag
    char *t; // Name of the valgrind trace to replay
} Args;

typedef struct {
    int nhits;
    int nmisses;
    int nevictions;
} Statistics;

typedef struct {
    char valid;	// 'y' or 'n'
    int tag;
    // char* data; // 因为只是统计cache访问情况，所以不需要这个字段，如果需要，我会把类型设置为char*，因为char是一个字节，然后根据b动态分配数组，赋值的话就用memcpy等。
    int lru; // lru counter
    // 当一个行被载入B字节块时，初始化为当前set中最大的lru值加一。
    // 当这个行被访问（读/写）时，增加计数器，错误，应该设置计数器为该set中最大的计数器值加一。evict时则逐出set中计数器最小的行。
} Line;

typedef Line* Set; // Set是一个Line数组。

typedef struct {
    int S;
    int E;
    Set* sets; // sets[i][j]，i为第i个Set，j为该Set中第j行。
} Cache;

void help() {
    printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n\tOptions:\n");
    printf("-h         Print this help message.\n");
    printf("-v         Optional verbose flag.\n");
    printf("-s <num>   Number of set index bits.\n");
    printf("-E <num>   Number of lines per set.\n");
    printf("-b <num>   Number of block offset bits.\n");
    printf("-t <file>  Trace file.\n\n");
    printf("Examples:\n");
    printf("linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void initArgs(int argc, char* *argv, Args* args) {
    char *s;
    if (argc!=2 && argc!=9 && argc!=10) {
        printf("Missing required command line argument\n");
        help();
        exit(EXIT_FAILURE);
    }
    args->v = 0; // 可选的flag先初始化，而其它flag必然会被设置，所以不初始化也行。
    for (int i=1; i<argc; i++) {
        s = argv[i];
        if (*s!='-') {
            printf("invalid option -- %s\n", s);
            help();
            exit(EXIT_FAILURE);
        }
        switch (*++s) {
        case 'h': help(); exit(EXIT_SUCCESS);
        case 'v': args->v=1; break;
        case 's':
        case 'E':
        case 'b':
        case 't':
            if (++i==argc) {
                printf("option requires an argument -- %s\n", s);
                help();
                exit(EXIT_FAILURE);
	        }
            switch (*s) {
            // 这里则假设参数是合法的数字和文件路径，没有检查。
            case 's': args->s = strtol(argv[i], NULL, 10); break;
            case 'E': args->E = strtol(argv[i], NULL, 10); break;
            case 'b': args->b = strtol(argv[i], NULL, 10); break;
            case 't': args->t = argv[i]; break;
	        }
            break;
        }
    }
    // printf("%d, %d, %d, %d, %s\n", args->v, args->s, args->E, args->b, args->t);
}

void initCache(Cache *cache, Args *args) {
    Set *set;
    cache->S = pow(2, args->s);
    cache->E = args->E;
    cache->sets = (Set*)malloc(cache->S * sizeof(Set));
    for (int i=0; i<cache->S; i++) {
        cache->sets[i] = (Line*)malloc(cache->E * sizeof(Line));
	    for (int j=0; j<cache->E; j++) {
            cache->sets[i][j].valid = 'n';
            cache->sets[i][j].tag = 0;
            cache->sets[i][j].lru = 0;
        }
    }
    // printf("%c, %d, %d\n", cache->sets[1][0].valid, cache->sets[1][0].tag, cache->sets[1][0].lru);
}

void initStatistics(Statistics *st) {
    st->nhits = 0;
    st->nmisses = 0;
    st->nevictions = 0;
}

// 像这种简单的函数，编译器会帮我们inline掉，所以就不要顾虑调用开销了。
int getSetIndex(Args *args, long addr) {
    return (addr>>args->b) & ((1<<args->s)-1);
}

int getTag(Args *args, long addr) {
    return addr>>(args->b+args->s);
}

// 在模拟统计的角度下，L和S所做的操作是一样的，M需要额外一点操作即可。
void accessCache(Args *args, Cache *cache, Statistics *st, long addr, char *line, char op) {
    int setIndex = getSetIndex(args, addr);
    int tag = getTag(args, addr);
    Set set = cache->sets[setIndex];
    int minLRU = INT_MAX; // 利用遍历过程记录一些信息，之后evict时才不用再次遍历。
    int minLRULine = -1;
    int maxLRU = INT_MIN;
    int idleLine = -1; // 记录遍历中出现的最后一个空闲行，以备替换。
    int hit = -1;
    // printf("%d, %d\t", setIndex, tag);
    for (int i=0; i<cache->E; i++) {
        if (set[i].valid=='y' && set[i].tag==tag) {
            st->nhits++;
            // set[i].lru++;
            if (args->v) strcat(line, " hit");
	        if (op=='M') {
                st->nhits++;
                // set[i].lru++;
                if (args->v) strcat(line, " hit");
            }
            if (args->v) printf("%s\n", line+1);
	        hit = i;
        }
        if (set[i].lru<minLRU) {
            minLRU = set[i].lru;
	        minLRULine = i;
        }
        if (set[i].lru>maxLRU) maxLRU=set[i].lru;
        if (set[i].valid=='n') idleLine = i;
    }
    if (hit!=-1) {
        set[hit].lru = maxLRU+1;
        return;
        // 这里是对bug的修复，考虑这样一种情况(2, 2, 3)，第一列是setIndex，第二列是tag，第三列是lru计数器：
        // 0 196693 0 hmiss
        // 0 1073217564 0 miss
        // 0 1073217564 1 hit
        // 0 1073217564 2 hit
        // 0 196689 3 miss, eviction
        // 0 1073217564 3 hit
        // 0 196691 4 miss, eviction // 就在这里，set0中的两个行的lru计数器都为3，事实上这时应该淘汰196689，但却错误地淘汰了1073217564。
        // 0 1073217564 miss, eviction
        // 所以应该修改算法，当hit/设置新的B字节块后，不应该使行的计数器自增1，而应该将行的计数器设置为当前集合中最大的计数器值加一。
    }
    // 从内存中获取。
    st->nmisses++;
    if (args->v) strcat(line, " miss");
    if (idleLine==-1) {
        set[minLRULine].tag = tag;
        set[minLRULine].lru = maxLRU+1;
        st->nevictions++;
        if (args->v) strcat(line, " eviction");
    } else {
        set[idleLine].valid = 'y';
        set[idleLine].tag = tag;
        set[idleLine].lru = maxLRU+1;
    }
    if (op=='M') {
        st->nhits++;
        // if (idleLine==-1) set[minLRULine].lru++;
        // else set[idleLine].lru++;
        if (args->v) strcat(line, " hit");
    }
    if (args->v) printf("%s\n", line+1);
}

void statistic(Args *args, Cache *cache, Statistics *st) {
    size_t len = 512;
    char *line = malloc(len);
    char op; // L, M, S
    long addr;
    FILE *fp = fopen(args->t, "r");
    if (fp==NULL) {
        printf("%s: No such file or directory", args->t);
        exit(EXIT_FAILURE);
    }
    // If *lineptr is set to NULL and *n is set 0 before the call, then getline() will allocate a buffer for storing the line.
    // This buffer should be freed by the user program even if getline() failed.
    while (getline(&line, &len, fp) != -1) {
        if (*line!=' ') continue;
        sscanf(line, " %c %lx", &op, &addr);
        if (args->v) line[strlen(line)-1] = '\0';
        // printf("%c %lx, %d %d\n", op, addr, getSetIndex(args, addr), getTag(args, addr));
        accessCache(args, cache, st, addr, line, op);
    }
    fclose(fp);
    free(line);
}

int main(int argc, char* *argv) {
    Args args;
    initArgs(argc, argv, &args);
    Cache cache;
    initCache(&cache, &args);
    Statistics st;
    initStatistics(&st);
    statistic(&args, &cache, &st);
    printSummary(st.nhits, st.nmisses, st.nevictions);
    return 0;
}
