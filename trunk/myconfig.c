#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "logfile.h"
#include "mem.h"
#include "myconfig.h"
#include "zzmalloc.h"
#include "synclog.h"
#include "server.h"
#include "dumpfile.h"
#include "wthread.h"

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
    mcf->sync_interval = 60;
    mcf->write_port = 11212;
    mcf->sync_port  = 11213;
    mcf->log_level  = 3; 
    mcf->timeout    = 30;

    snprintf(mcf->datadir, PATH_MAX, "data");

    FILE    *fp;
    //char    filepath[PATH_MAX];

    // FIXME: must absolute path
    //snprintf(filepath, PATH_MAX, "etc/%s", filename);

    //fp = fopen(filepath, "r");
    fp = fopen(filename, "r");
    if (NULL == fp) {
        DERROR("open config file error: %s\n", filename);
        MEMLINK_EXIT;
    }

    char buffer[2048];
    while (1) {
        if (fgets(buffer, 2048, fp) == NULL) {
            //DINFO("config file read complete!\n");
            break;
        }
        //DINFO("buffer: %s\n", buffer);
        
        if (buffer[0] == '#') { // skip comment
            continue;
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
            end += 1;
        }
        
        if (strcmp(buffer, "block_data_count") == 0) {
            mcf->block_data_count = atoi(start);
        }else if (strcmp(buffer, "dump_interval") == 0) {
            mcf->dump_interval = atoi(start);
        }else if (strcmp(buffer, "sync_interval") == 0) {
            mcf->sync_interval = atoi(start); 
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
        }else if (strcmp(buffer, "log_name") == 0) {
            snprintf(mcf->log_name, PATH_MAX, "%s", start);    
        }else if (strcmp(buffer, "timeout") == 0) {
            mcf->timeout = atoi(start);
        }else if (strcmp(buffer, "thread_num") == 0) {
            int t_n = atoi(start);
            mcf->thread_num = t_n > MEMLINK_MAX_THREADS ? MEMLINK_MAX_THREADS : t_n;
        }else if (strcmp(buffer, "max_conn") == 0) {
            mcf->max_conn = atoi(start);
        }else if (strcmp(buffer, "max_core") == 0) {
            mcf->max_core = atoi(start);
        }else if (strcmp(buffer, "is_daemon") == 0) {
            mcf->is_daemon = atoi(start);
        }else if (strcmp(buffer, "role") == 0) {
            mcf->role = atoi(start);
        }else if (strcmp(buffer, "master_sync_host") == 0) {
            snprintf(mcf->master_sync_host, IP_ADDR_MAX_LEN, "%s", start);
		}else if (strcmp(buffer, "master_sync_port") == 0) {
			mcf->master_sync_port = atoi(start);
        }else if (strcmp(buffer, "sync_interval") == 0) {
            mcf->sync_interval = atoi(start);
        }
    }

    fclose(fp);

    g_cf = mcf;
    //DINFO("create MyConfig ok!\n");

    return mcf;
}

int 
compare_int ( const void *a , const void *b ) 
{ 
    return *(int *)a - *(int *)b; 
} 

/**
 * Apply log records to hash table.
 *
 * @param logname sync log file pathname
 */
static int 
load_synclog(char *logname)
{
    int ret;

    int ffd = open(logname, O_RDONLY);
    if (-1 == ffd) {
        DERROR("open file %s error! %s\n", logname, strerror(errno));
        MEMLINK_EXIT;
    }
    int len = lseek(ffd, 0, SEEK_END);

    char *addr = mmap(NULL, len, PROT_READ, MAP_SHARED, ffd, 0);
    if (addr == MAP_FAILED) {
        DERROR("synclog mmap error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }   
    unsigned int indexlen = *(unsigned int*)(addr + sizeof(short) + sizeof(int));
    //unsigned int *index = (unsigned int*)(addr + sizeof(short) + sizeof(int) * 2);
    char *data    = addr + sizeof(short) + sizeof(int) * 2 + indexlen * sizeof(int);
    char *enddata = addr + len;

    unsigned short blen; 
    while (data < enddata) {
        blen = *(unsigned short*)data;  

        if (enddata < data + blen + sizeof(short)) {
            DERROR("synclog end error: %s, skip\n", logname);
            //MEMLINK_EXIT;
            break;
        }
        DINFO("command, len:%d\n", blen);
        ret = wdata_apply(data, blen + sizeof(short), 0);       
        if (ret != 0) {
            DERROR("wdata_apply log error: %d\n", ret);
            MEMLINK_EXIT;
        }

        data += blen + sizeof(short); 
    }

    munmap(addr, len);

    close(ffd);

    return 0;
}

static int
load_data()
{
    int    ret;
    struct stat stbuf;
    int    havedump = 0;
    //char   *filename = "data/dump.dat";
    char   filename[PATH_MAX];

    snprintf(filename, PATH_MAX, "%s/dump.dat", g_cf->datadir);
    // check dumpfile exist
    ret = stat(filename, &stbuf);
    if (ret == -1 && errno == ENOENT) {
        DINFO("not found dumpfile: %s\n", filename);
    }   
  
    // have dumpfile, load
    if (ret == 0) {
        havedump = 1;
    
        DINFO("try load dumpfile ...\n");
        ret = loaddump(g_runtime->ht, filename);
        if (ret < 0) {
            DERROR("loaddump error: %d\n", ret);
            MEMLINK_EXIT;
            return -1;
        }
    }


    // get all synclog
    DIR     *mydir; 
    struct  dirent *nodes;
    int     minid = g_runtime->dumplogver;
    int     logids[10000] = {0};
    int     i = 0;

    DINFO("try get synclog ...\n");
    mydir = opendir(g_cf->datadir);
    if (NULL == mydir) {
        DERROR("opendir %s error: %s\n", g_cf->datadir, strerror(errno));
        return -2;
    }   
    while ((nodes = readdir(mydir)) != NULL) {
        DINFO("name: %s\n", nodes->d_name);
        if (strncmp(nodes->d_name, "bin.log.", 8) == 0) {
            int binid = atoi(&nodes->d_name[8]);
            if (binid > minid) {
                logids[i] = binid;
                i++;
            }   
        }   
    }   
    closedir(mydir);    

    qsort(logids, i, sizeof(int), compare_int);

    int n = i;
    //int ffd, len;
    char logname[PATH_MAX];

    DINFO("dumplogver: %d, n: %d\n", g_runtime->dumplogver, n);
    for (i = g_runtime->dumplogver; i < n; i++) {
        snprintf(logname, PATH_MAX, "%s/data/bin.log.%d", g_runtime->home, logids[i]);
        DINFO("load synclog: %s\n", logname);

        load_synclog(logname);

        /* 
        ffd = open(logname, O_RDONLY);
        if (-1 == ffd) {
            DERROR("open file %s error! %s\n", logname, strerror(errno));
            MEMLINK_EXIT;
        }
        len = lseek(ffd, 0, SEEK_END);
        
        char *addr = mmap(NULL, len, PROT_READ, MAP_SHARED, ffd, 0);
        if (addr == MAP_FAILED) {
            DERROR("synclog mmap error: %s\n", strerror(errno));
            MEMLINK_EXIT;
        }   
        unsigned int indexlen = *(unsigned int*)(addr + sizeof(short) + sizeof(int));
        //unsigned int *index = (unsigned int*)(addr + sizeof(short) + sizeof(int) * 2);
        char *data    = addr + sizeof(short) + sizeof(int) * 2 + indexlen * sizeof(int);
        char *enddata = addr + len;
       
        unsigned short blen; 
        while (data < enddata) {
            blen = *(unsigned short*)data;  
           
            if (enddata < data + blen + sizeof(short)) {
                DERROR("synclog end error: %s, skip\n", logname);
                //MEMLINK_EXIT;
                break;
            }
            DINFO("command, len:%d\n", blen);
            ret = wdata_apply(data, blen + sizeof(short), 0);       
            if (ret != 0) {
                DERROR("wdata_apply log error: %d\n", ret);
                MEMLINK_EXIT;
            }

            data += blen + sizeof(short); 
        }
    
        munmap(addr, len);

        close(ffd);
        */
    }

    snprintf(logname, PATH_MAX, "%s/data/bin.log", g_runtime->home);
    DINFO("load synclog: %s\n", logname);
    load_synclog(logname);

    if (havedump == 0) {
        dumpfile(g_runtime->ht);
    }

    return 0;
}


Runtime *g_runtime;

static Runtime* 
runtime_init(char *pgname) 
{
    Runtime *rt = (Runtime*)zz_malloc(sizeof(Runtime));
    if (NULL == rt) {
        DERROR("malloc Runtime error!\n");
        MEMLINK_EXIT;
        return NULL; 
    }
    memset(rt, 0, sizeof(Runtime));
    g_runtime = rt;

    realpath(pgname, rt->home);
    return rt;
}

int mainserver_init(Runtime *rt) 
{
    rt->server = mainserver_create();
    if (NULL == rt->server) {
        DERROR("mainserver_create error!\n");
        MEMLINK_EXIT;
        return -1;
    }
    DINFO("main thread create ok!\n");
    return 0;
}

int hashtable_init(Runtime* rt) 
{
    rt->ht = hashtable_create();
    if (NULL == rt->ht) {
        DERROR("hashtable_create error!\n");
        MEMLINK_EXIT;
        return -1;
    }
    DINFO("hashtable create ok!\n");
    return 0;
}


int mempool_init(Runtime* rt) 
{
    rt->mpool = mempool_create();
    if (NULL == rt->mpool) {
        DERROR("mempool create error!\n");
        MEMLINK_EXIT;
        return -1;
    }
    DINFO("mempool create ok!\n");
    return 0;
}

Runtime*
runtime_create_common(char *pgname)
{
    Runtime *rt = (Runtime*)zz_malloc(sizeof(Runtime));
    if (NULL == rt) {
        DERROR("malloc Runtime error!\n");
        MEMLINK_EXIT;
        return NULL; 
    }
    memset(rt, 0, sizeof(Runtime));
    g_runtime = rt;

    realpath(pgname, rt->home);
    char *last = strrchr(rt->home, '/');  
    if (last != NULL) {
        *last = 0;
    }
    DINFO("home: %s\n", rt->home);

    int ret;
    ret = pthread_mutex_init(&rt->mutex, NULL);
    if (ret != 0) {
        DERROR("pthread_mutex_init error: %s\n", strerror(errno));
        MEMLINK_EXIT;
        return NULL;
    }
    DINFO("mutex init ok!\n");

    rt->mpool = mempool_create();
    if (NULL == rt->mpool) {
        DERROR("mempool create error!\n");
        MEMLINK_EXIT;
        return NULL;
    }
    DINFO("mempool create ok!\n");

    rt->ht = hashtable_create();
    if (NULL == rt->ht) {
        DERROR("hashtable_create error!\n");
        MEMLINK_EXIT;
        return NULL;
    }
    DINFO("hashtable create ok!\n");

    rt->server = mainserver_create();
    if (NULL == rt->server) {
        DERROR("mainserver_create error!\n");
        MEMLINK_EXIT;
        return NULL;
    }
    DINFO("main thread create ok!\n");

    rt->synclog = synclog_create();
    if (NULL == rt->synclog) {
        DERROR("synclog_create error!\n");
        MEMLINK_EXIT;
        return NULL;
    }
    DINFO("synclog open ok!\n");

    ret = load_data();
    if (ret < 0) {
        DERROR("load_data error: %d\n", ret);
        MEMLINK_EXIT;
        return NULL;
    }
    DINFO("load_data ok!\n");

    return rt;
}

Runtime* 
slave_runtime_create(char *pgname) 
{
    Runtime * rt = runtime_init(pgname);

    if (hashtable_init(rt) != 0)
      return NULL;
    if (mempool_init(rt) != 0)
        return NULL;
    if (mainserver_init(rt) != 0)
        return NULL;

    /*rt->sslave = sslave_create();*/
    /*if (NULL == rt->wthread) {*/
    /*   DERROR("sync slave creation error!\n");*/
    /*   MEMLINK_EXIT;*/
    /*   return NULL;*/
    /*}*/
    /*DINFO("sync slave creation ok!\n");*/

    DINFO("create slave runtime ok!\n");
    return rt;
}

Runtime*
runtime_create_master(char *pgname)
{
    Runtime* rt;// = runtime_init(pgname);

    rt = runtime_create_common(pgname);
    if (NULL == rt) {
        return rt;
    }

    rt->wthread = wthread_create();
    if (NULL == rt->wthread) {
        DERROR("wthread_create error!\n");
        MEMLINK_EXIT;
        return NULL;
    }
    DINFO("write thread create ok!\n");
    
    rt->sthread = sthread_create();
    if (NULL == rt->sthread) {
     DERROR("sthread_create error!\n");
     MEMLINK_EXIT;
     return NULL;
    }
    DINFO("sync thread create ok!\n");

    DINFO("create master Runtime ok!\n");
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


