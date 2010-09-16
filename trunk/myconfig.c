#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "logfile.h"
#include "mem.h"
#include "myconfig.h"
#include "zzmalloc.h"
#include "synclog.h"
#include "wrthread.h"
#include "server.h"

MyConfig *g_cf;

MyConfig*
myconfig_create(char *filename)
{
    MyConfig *mcf;

    mcf = (MyConfig*)zz_malloc(sizeof(MyConfig));
    if (NULL == mcf) {
        DERROR("malloc MyConfig error!\n");
        return NULL;
    }
    memset(mcf, 0, sizeof(MyConfig));
    mcf->block_data_count = 100;
    mcf->block_clean_cond = 0.5;
    mcf->dump_interval = 60;
    mcf->write_port = 11212;
    mcf->sync_port  = 11213;
    mcf->log_level  = 3; 
    mcf->timeout    = 30;

    snprintf(mcf->datadir, PATH_MAX, "data");

    FILE    *fp;
    char    filepath[PATH_MAX];

    // FIXME: must absolute path
    snprintf(filepath, PATH_MAX, "etc/%s", filename);

    fp = fopen(filepath, "r");
    if (NULL == fp) {
        DERROR("open config file error: %s\n", filepath);
    }

    char buffer[2048];
    while (1) {
        if (fgets(buffer, 2048, fp) == NULL) {
            break;
        }
        char *sp = strchr(buffer, '=');
        if (sp == NULL) {
            DERROR("config file error: %s\n", buffer);
            MEMLINK_EXIT;
        }

        char *end = sp - 1;
        while (end > buffer && isblank(*end)) {
            end--;
        }
        *(end + 1) = 0;

        char *start = sp + 1;
        while (isblank(*start)) {
            start++;
        }
        
        end = start;
        while (*end != 0) {
            if (*end == '\r' || *end == '\n') {
                end -= 1;
                while (end > start && isblank(*end)) {
                    end--;
                }
                *(end + 1) = 0;
                break;
            }
        }
        
        if (strcmp(buffer, "block_data_count") == 0) {
            mcf->block_data_count = atoi(start);
        }else if (strcmp(buffer, "dump_interval") == 0) {
            mcf->dump_interval = atoi(start);
        }else if (strcmp(buffer, "block_clean_cond") == 0) {
            mcf->block_clean_cond = atof(start);
        }else if (strcmp(buffer, "read_port") == 0) {
            mcf->read_port = atoi(start);
        }else if (strcmp(buffer, "write_port") == 0) {
            mcf->write_port = atoi(start);
        }else if (strcmp(buffer, "sync_port") == 0) {
            mcf->sync_port = atoi(start);
        }else if (strcmp(buffer, "data_dir") == 0) {
            snprintf(mcf->datadir, PATH_MAX, "%s", start);
        }else if (strcmp(buffer, "log_level") == 0) {
            mcf->log_level = atoi(start);
        }else if (strcmp(buffer, "timeout") == 0) {
            mcf->timeout = atoi(start);
        }else if (strcmp(buffer, "thread_num") == 0) {
            mcf->thread_num = atoi(start);
        }else if (strcmp(buffer, "max_conn") == 0) {
            mcf->max_conn = atoi(start);
        }else if (strcmp(buffer, "max_core") == 0) {
            mcf->max_core = atoi(start);
        }else if (strcmp(buffer, "is_daemon") == 0) {
            mcf->is_daemon = atoi(start);
        }
         
    }


    g_cf = mcf;

    DINFO("create MyConfig ok!\n");

    return mcf;
}

Runtime *g_runtime;

Runtime*
runtime_create(char *pgname)
{
    Runtime *rt;

    rt = (Runtime*)zz_malloc(sizeof(Runtime));
    if (NULL == rt) {
        DERROR("malloc Runtime error!\n");
        MEMLINK_EXIT;
        return NULL; 
    }
    memset(rt, 0, sizeof(Runtime));
    realpath(pgname, rt->home);
   
    int ret;
    ret = pthread_mutex_init(&rt->mutex, NULL);
    if (ret != 0) {
        DERROR("pthread_mutex_init error: %s\n", strerror(errno));
        MEMLINK_EXIT;
        return NULL;
    }

    rt->synclog = synclog_create("bin.log");
    if (NULL == rt->synclog) {
        DERROR("synclog_create error!\n");
        MEMLINK_EXIT;
        return NULL;
    }

    rt->mpool = mempool_create();
    if (NULL == rt->mpool) {
        DERROR("mempool create error!\n");
        MEMLINK_EXIT;
        return NULL;
    }

    rt->wrthread = wrthread_create();
    if (NULL == rt->wrthread) {
        DERROR("wrthread_create error!\n");
        MEMLINK_EXIT;
        return NULL;
    }

    rt->server = mainserver_create();
    if (NULL == rt->server) {
        DERROR("mainserver_create error!\n");
        MEMLINK_EXIT;
        return NULL;
    }

    g_runtime = rt;

    DINFO("create Runtime ok!\n");

    return rt;
}

void
runtime_destroy(Runtime *rt)
{
    if (NULL == rt)
        return;

    pthread_mutex_destroy(&rt->mutex);
    zz_free(rt);
}



