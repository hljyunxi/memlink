/**
 * 配置和初始化
 * @file myconfig.c
 * @author zhaowei
 * @ingroup memlink
 * @{
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
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
#include "common.h"
#include "utils.h"
#include "sslave.h"
#include "runtime.h"

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

    // set default value
    mcf->block_data_count[0]    = 50;
    mcf->block_data_count[1]    = 20;
    mcf->block_data_count[2]    = 10;
    mcf->block_data_count[3]    = 5;
    mcf->block_data_count[4]    = 1;
    mcf->block_data_count_items = 5;
    mcf->block_clean_cond  = 0.5;
    mcf->block_clean_start = 3;
    mcf->block_clean_num   = 100;
    mcf->dump_interval = 60;
    mcf->sync_interval = 60;
    mcf->write_port = 11002;
    mcf->sync_port  = 11003;
    mcf->log_level  = 3; 
    mcf->timeout    = 30;
    mcf->max_conn   = 1000;

    snprintf(mcf->datadir, PATH_MAX, "data");

    FILE    *fp;
    //char    filepath[PATH_MAX];
    // FIXME: must absolute path
    //snprintf(filepath, PATH_MAX, "etc/%s", filename);

	int lineno = 0;
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
		lineno ++;
        //DINFO("buffer: %s\n", buffer);
        if (buffer[0] == '#' || buffer[0] == '\r' || buffer[0] == '\n') { // skip comment
            continue;
        }
        char *sp = strchr(buffer, '=');
        if (sp == NULL) {
            DERROR("config file error at line %d: %s\n", lineno, buffer);
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
            char *ptr = start;
            int  i = 0;
            while (start) {
                //DINFO("start:%s\n", start);
                while (isblank(*start)) {
                    start++;
                }
                ptr = strchr(start, ',');
                if (ptr) {
                    *ptr = 0;
                    ptr++;
                }
                
                mcf->block_data_count[i] = atoi(start);
                start = ptr;
                //DINFO("i: %d, %d\n", i, mcf->block_data_count[i]);
                i++;
                if (i >= BLOCK_DATA_COUNT_MAX) {
                    DERROR("config error: block_data_count have too many items.\n");
                    MEMLINK_EXIT;
                }
            }
            mcf->block_data_count_items = i;
            qsort(mcf->block_data_count, i, sizeof(int), compare_int);

            //for (i = 0; i < BLOCK_DATA_COUNT_MAX; i++) {
            //    DINFO("block_data_count: %d, %d\n", i, mcf->block_data_count[i]);
            //}
        }else if (strcmp(buffer, "block_data_reduce") == 0) {
            mcf->block_data_reduce = atof(start);
        }else if (strcmp(buffer, "dump_interval") == 0) {
            mcf->dump_interval = atoi(start);
        }else if (strcmp(buffer, "sync_interval") == 0) {
            mcf->sync_interval = atoi(start); 
        }else if (strcmp(buffer, "block_clean_cond") == 0) {
            mcf->block_clean_cond = atof(start);
        }else if (strcmp(buffer, "block_clean_start") == 0) {
            mcf->block_clean_start = atoi(start);
        }else if (strcmp(buffer, "block_clean_num") == 0) {
            mcf->block_clean_num = atoi(start);
        }else if (strcmp(buffer, "ip") == 0) {
            snprintf(mcf->ip, IP_ADDR_MAX_LEN, "%s", start);
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
            int tn = atoi(start);
            mcf->thread_num = tn > MEMLINK_MAX_THREADS ? MEMLINK_MAX_THREADS : tn;
        }else if (strcmp(buffer, "write_binlog") == 0) {
            if (strcmp(start, "yes") == 0) {
                mcf->write_binlog = 1;
            }else if (strcmp(start, "no") == 0){
                mcf->write_binlog = 0;
            }else{
                DERROR("write_binlog must set to yes/no !\n");
                MEMLINK_EXIT;
            }
        }else if (strcmp(buffer, "max_conn") == 0) {
            mcf->max_conn = atoi(start);
            if (mcf->max_conn < 0) {
                DERROR("max_conn must not smaller than 0.\n");
                MEMLINK_EXIT;
            }
        }else if (strcmp(buffer, "max_read_conn") == 0) {
            mcf->max_read_conn = atoi(start);
            if (mcf->max_read_conn < 0) {
                DERROR("max_read_conn must >= 0 and <= max_conn\n");
                MEMLINK_EXIT;
            }
        }else if (strcmp(buffer, "max_write_conn") == 0) {
            mcf->max_write_conn = atoi(start);
            if (mcf->max_write_conn < 0) {
                DERROR("max_write_conn must >= 0 and <= max_conn\n");
                MEMLINK_EXIT;
            }
        }else if (strcmp(buffer, "max_sync_conn") == 0) {
            mcf->max_sync_conn = atoi(start);
            if (mcf->max_sync_conn < 0) {
                DERROR("max_sync_conn must >= 0 and <= max_conn\n");
                MEMLINK_EXIT;
            }
        }else if (strcmp(buffer, "max_core") == 0) {
            mcf->max_core = atoi(start);
        }else if (strcmp(buffer, "is_daemon") == 0) {
            if (strcmp(start, "yes") == 0) {
                mcf->is_daemon = 1;
            }else if (strcmp(start, "no") == 0){
                mcf->is_daemon = 0;
            }else{
                DERROR("is_daemon must set to yes/no !\n");
                MEMLINK_EXIT;
            }
        }else if (strcmp(buffer, "role") == 0) {
            if (strcmp(start, "master") == 0) {
                mcf->role = ROLE_MASTER;
            }else if (strcmp(start, "backup") == 0) {
                mcf->role = ROLE_BACKUP;
            }else if (strcmp(start, "slave") == 0) {
                mcf->role = ROLE_SLAVE;
            }else{
                DERROR("role must set to master/backup/slave!\n");
                MEMLINK_EXIT;
            }
        }else if (strcmp(buffer, "master_sync_host") == 0) {
            snprintf(mcf->master_sync_host, IP_ADDR_MAX_LEN, "%s", start);
		}else if (strcmp(buffer, "master_sync_port") == 0) {
			mcf->master_sync_port = atoi(start);
        }else if (strcmp(buffer, "sync_interval") == 0) {
            mcf->sync_interval = atoi(start);
        }else if (strcmp(buffer, "user") == 0) {
            snprintf(mcf->user, 128, "%s", start);
        }
    }

    fclose(fp);

    // check config 
    if (mcf->max_conn <= 0 || mcf->max_conn < mcf->max_read_conn || 
        mcf->max_conn < mcf->max_write_conn || mcf->max_conn < mcf->max_sync_conn) {
        DERROR("config max_conn must >= 0 and >= max_read_conn, >= max_write_conn, >= max_sync_conn.\n");
        MEMLINK_EXIT;
    }

    if (mcf->max_read_conn == 0) {
        mcf->max_read_conn = mcf->max_conn;
    }
    if (mcf->max_write_conn == 0) {
        mcf->max_write_conn = mcf->max_conn;
    }
    if (mcf->max_sync_conn == 0) {
        mcf->max_sync_conn = mcf->max_conn;
    }


    g_cf = mcf;
    //DINFO("create MyConfig ok!\n");

    return mcf;
}

int
myconfig_change()
{
    char *conffile = g_runtime->conffile;
    char conffile_tmp[PATH_MAX];

    DINFO("change config file...\n");

    snprintf(conffile_tmp, PATH_MAX, "%s.tmp", conffile);

    FILE *fp = fopen64(conffile_tmp, "wb");
    char line[512] = {0};

    snprintf(line, 512, "block_data_count = %d,%d,%d,%d\n", g_cf->block_data_count[0],
        g_cf->block_data_count[1], g_cf->block_data_count[2], g_cf->block_data_count[3]);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "block_data_reduce = %0.1f\n", g_cf->block_data_reduce);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "dump_interval = %d\n", g_cf->dump_interval);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "block_clean_cond = %0.1f\n", g_cf->block_clean_cond);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "block_clean_start = %d\n", g_cf->block_clean_start);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "block_clean_num = %d\n", g_cf->block_clean_num);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "read_port = %d\n", g_cf->read_port);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "write_port = %d\n", g_cf->write_port);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "sync_port = %d\n", g_cf->sync_port);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "data_dir = %s\n", g_cf->datadir);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "log_level = %d\n", g_cf->log_level);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "log_name = %s\n", g_cf->log_name);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "timeout = %d\n", g_cf->timeout);
    ffwrite(line, strlen(line), 1, fp);
    
    snprintf(line, 512, "thread_num = %d\n", g_cf->thread_num);
    ffwrite(line, strlen(line), 1, fp);
    
    if (g_cf->write_binlog == 1)
        snprintf(line, 512, "write_binlog = %s\n", "yes");
    else
        snprintf(line, 512, "write_binlog = %s\n", "no");
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "max_conn = %d\n", g_cf->max_conn);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "max_read_conn = %d\n", g_cf->max_read_conn);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "max_write_conn = %d\n", g_cf->max_write_conn);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "max_core = %d\n", g_cf->max_core);
    ffwrite(line, strlen(line), 1, fp);

    if (g_cf->is_daemon == 1)
        snprintf(line, 512, "is_daemon = %s\n", "yes");
    else
        snprintf(line, 512, "is_daemon = %s\n", "no");
    ffwrite(line, strlen(line), 1, fp);
    
    snprintf(line, 512, "%s\n", "# master/backup/slave");
    ffwrite(line, strlen(line), 1, fp);

    if (g_cf->role == ROLE_MASTER)
        snprintf(line, 512, "role = %s\n", "master");
    else if (g_cf->role == ROLE_BACKUP)
        snprintf(line, 512, "role = %s\n", "backup");
    else if (g_cf->role == ROLE_SLAVE)
        snprintf(line, 512, "role = %s\n", "slave");
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "master_sync_host = %s\n", g_cf->master_sync_host);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "master_sync_port = %d\n", g_cf->master_sync_port);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "sync_interval = %d\n", g_cf->sync_interval);
    ffwrite(line, strlen(line), 1, fp);

    snprintf(line, 512, "user = %s\n", g_cf->user);
    ffwrite(line, strlen(line), 1, fp);
    
    int ret;
    ret = rename(conffile_tmp, conffile);

    if (ret < 0) {
        DERROR("conffile rename error %s\n", strerror(errno));
    }
    fclose(fp);
    return 1;
}

/*
Runtime *g_runtime;

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
	
	rt->memlink_start = time(NULL);
    if (realpath(pgname, rt->home) == NULL) {
		DERROR("realpath error: %s\n", strerror(errno));
		MEMLINK_EXIT;
	}
    char *last = strrchr(rt->home, '/');  
    if (last != NULL) {
        *last = 0;
    }
    DINFO("home: %s\n", rt->home);

    int ret;
	// create data and log dir
	if (!isdir(g_cf->datadir)) {
		ret = mkdir(g_cf->datadir, 0744);
		if (ret == -1) {
			DERROR("create dir %s error! %s\n", g_cf->datadir, strerror(errno));
			MEMLINK_EXIT;
		}
	}

    ret = pthread_mutex_init(&rt->mutex, NULL);
    if (ret != 0) {
        DERROR("pthread_mutex_init error: %s\n", strerror(errno));
        MEMLINK_EXIT;
        return NULL;
    }
    DINFO("mutex init ok!\n");

    ret = pthread_mutex_init(&rt->mutex_mem, NULL);
    if (ret != 0) {
        DERROR("pthread_mutex_init error: %s\n", strerror(errno));
        MEMLINK_EXIT;
        return NULL;
    }
    DINFO("mutex_mem init ok!\n");

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
	DINFO("synclog index_pos:%d, pos:%d\n", g_runtime->synclog->index_pos, g_runtime->synclog->pos);

    return rt;
}

Runtime* 
runtime_create_slave(char *pgname, char *conffile) 
{
    Runtime *rt;

    rt = runtime_create_common(pgname);
    if (NULL == rt) {
        return rt;
    }
    snprintf(rt->conffile, PATH_MAX, "%s", conffile); 
    rt->slave = sslave_create();
    if (NULL == rt->slave) {
        DERROR("sslave_create error!\n");
        MEMLINK_EXIT;
    }
    DINFO("sslave thread create ok!\n");
    
    rt->sthread = sthread_create();
    if (NULL == rt->sthread) {
     DERROR("sthread_create error!\n");
     MEMLINK_EXIT;
     return NULL;
    }
    DINFO("sync thread create ok!\n");


	int ret = load_data_slave();
    if (ret < 0) {
        DERROR("load_data error: %d\n", ret);
        MEMLINK_EXIT;
        return NULL;
    }
    DINFO("load_data ok!\n");
	
	rt->wthread = wthread_create();
    if (NULL == rt->wthread) {
        DERROR("wthread_create error!\n");
        MEMLINK_EXIT;
        return NULL;
    }
    DINFO("write thread create ok!\n");
 
	sslave_go(rt->slave);
	
	DNOTE("create slave runtime ok!\n");
    return rt;
}

Runtime*
runtime_create_master(char *pgname, char *conffile)
{
    Runtime* rt;// = runtime_init(pgname);

    rt = runtime_create_common(pgname);
    if (NULL == rt) {
        return rt;
    }
    snprintf(rt->conffile, PATH_MAX, "%s", conffile);
	int ret = load_data();
    if (ret < 0) {
        DERROR("load_data error: %d\n", ret);
        MEMLINK_EXIT;
        return NULL;
    }
    DINFO("load_data ok!\n");


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
	

    DNOTE("create master Runtime ok!\n");
    return rt;
}

void
runtime_destroy(Runtime *rt)
{
    if (NULL == rt)
        return;

    pthread_mutex_destroy(&rt->mutex);
    zz_free(rt);
}*/

int	
mem_used_inc(long long size)
{
    pthread_mutex_lock(&g_runtime->mutex_mem);
    g_runtime->mem_used += size;
    pthread_mutex_unlock(&g_runtime->mutex_mem);
    return 0;
}

int	
mem_used_dec(long long size)
{
    pthread_mutex_lock(&g_runtime->mutex_mem);
    g_runtime->mem_used -= size;
    pthread_mutex_unlock(&g_runtime->mutex_mem);
    return 0;
}



/**
 * @}
 */
