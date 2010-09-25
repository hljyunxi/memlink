#ifndef MEMLINK_MYCONFIG_H
#define MEMLINK_MYCONFIG_H

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include "synclog.h"
#include "hashtable.h"
#include "mem.h"
#include "wthread.h"
#include "server.h"

typedef struct _myconfig
{
    unsigned int block_data_count;
    unsigned int dump_interval;
    float        block_clean_cond;
    int          read_port;
    int          write_port;
    int          sync_port;
    char         datadir[PATH_MAX];
    int          log_level;
    int          timeout;
    int          thread_num; 
    int          max_conn;  // max connection
    int          max_core;  // maximize core file limit
    int          is_daemon; // is run with daemon
    int          role;
    int          master_addr;
    int          sync_interval;
}MyConfig;


typedef struct _runtime
{
    char            home[PATH_MAX]; // programe home dir
    pthread_mutex_t mutex; // write lock
    unsigned int    dumpver; // dump file version
    unsigned int    dumplogver; // log version in dump file
    unsigned int    logver;  // synclog version
    SyncLog         *synclog;  // current synclog
    MemPool         *mpool; 
    HashTable       *ht;
    volatile int    indump;
    WThread         *wthread;
    MainServer      *server;
}Runtime;

extern MyConfig *g_cf;
extern Runtime  *g_runtime;

MyConfig*   myconfig_create(char *filename);
Runtime*    master_runtime_create(char *pgname);
Runtime*    slave_runtime_create(char *pgname);
void        runtime_destroy(Runtime *rt);

#endif
