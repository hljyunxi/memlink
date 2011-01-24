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
#include "sslave.h"
#include "sthread.h"

// TODO is there a pre-defined const for this?
#define IP_ADDR_MAX_LEN 100

typedef struct _myconfig
{
    unsigned int block_data_count;
    unsigned int dump_interval;                       // in minutes
    float        block_clean_cond;
    int          block_clean_start;
    int          block_clean_num;
    int          read_port;
    int          write_port;
    int          sync_port;
    char         datadir[PATH_MAX];
    int          log_level;
    char         log_name[PATH_MAX];
	int			 write_binlog;
    int          timeout;
    int          thread_num;
    int          max_conn;                            // max connection
    int          max_core;                            // maximize core file limit
    int          is_daemon;                           // is run with daemon
    char         role;                                // 1 means master; 0 means slave
    char         master_sync_host[IP_ADDR_MAX_LEN];
    int          master_sync_port;
    unsigned int sync_interval;                       // in seconds
}MyConfig;


typedef struct _runtime
{
    char            home[PATH_MAX]; // programe home dir
    pthread_mutex_t mutex; // write lock
    unsigned int    dumpver; // dump file version
    unsigned int    dumplogver; // log version in dump file
    unsigned int    dumplogpos; // log position in dump file
    unsigned int    logver;  // synclog version
    SyncLog         *synclog;  // current synclog
    MemPool         *mpool; 
    HashTable       *ht;
	volatile int	inclean;
    //char			cleankey[512];
    WThread         *wthread;
    MainServer      *server;
    SSlave          *slave; // sync slave
    SThread         *sthread; // sync thread
    unsigned int    conn_num; // current conn count
}Runtime;

extern MyConfig *g_cf;
extern Runtime  *g_runtime;

MyConfig*   myconfig_create(char *filename);
Runtime*    runtime_create_master(char *pgname);
Runtime*    runtime_create_slave(char *pgname);
void        runtime_destroy(Runtime *rt);

#endif
