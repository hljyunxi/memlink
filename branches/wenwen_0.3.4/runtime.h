#ifndef MEMLINK_RUNTIME_H
#define MEMLINK_RUNTIME_H

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

typedef struct _runtime
{
    char            home[PATH_MAX]; // programe home dir
    char            conffile[PATH_MAX];
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
	time_t          last_dump;
	unsigned int    memlink_start;

	pthread_mutex_t	mutex_mem;
	long long		mem_used;
}Runtime;

extern Runtime  *g_runtime;

Runtime*    runtime_create_master(char *pgname, char *conffile);
Runtime*    runtime_create_slave(char *pgname, char *conffile);
void        runtime_destroy(Runtime *rt);


#endif
