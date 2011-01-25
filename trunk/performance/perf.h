#ifndef MEMLINK_PERF
#define MEMLINK_PERF

#include <stdio.h>

#define THREAD_MAX_NUM  1000

typedef struct
{
    char    key[255];
    char    value[255];
    int     valuelen;
    int     valuesize; // create valuesize
    char    maskstr[255];
    int     from; // range from
    int     len;  // range len
    int     pos; // move pos
    int     popnum; // pop num
    unsigned int kind;  // range kind
    unsigned int tag; //tag
    int     testcount; // test count
    int     longconn; // is long conn
}TestArgs;

typedef int (*TestFunc)(TestArgs *args);

typedef struct
{
    int         threads;
    TestFunc    func;
    TestArgs    *args;
}ThreadArgs;

typedef struct
{
    char        *name;
    TestFunc    func;
}TestFuncLink;

typedef struct
{
    int         threads;
    char        action[255];
    TestFunc    func;
    TestArgs    args;
}TestConfig;

#endif
