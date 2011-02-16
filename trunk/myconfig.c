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
#include "common.h"
#include "utils.h"
#include "sslave.h"

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
    mcf->block_clean_start = 3;
    mcf->block_clean_num = 100;
    mcf->dump_interval = 60;
    mcf->sync_interval = 60;
    mcf->write_port = 11002;
    mcf->sync_port  = 11003;
    mcf->log_level  = 3; 
    mcf->timeout    = 30;

    snprintf(mcf->datadir, PATH_MAX, "data");

    FILE    *fp;
    //char    filepath[PATH_MAX];

    // FIXME: must absolute path
    //snprintf(filepath, PATH_MAX, "etc/%s", filename);

    //fp = fopen(filepath, "r");
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
        
        if (buffer[0] == '#') { // skip comment
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
            mcf->block_data_count = atoi(start);
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

/**
 * Apply log records to hash table.
 *
 * @param logname sync log file pathname
 */
static int 
load_synclog(char *logname, unsigned int dumplogver, unsigned int dumplogpos)
{
    int ret;
	unsigned int binlogver;
	int have_key = 0;
	
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

	unsigned int indexlen = *(unsigned int*)(addr + SYNCLOG_HEAD_LEN - sizeof(int));
	char *data    = addr + SYNCLOG_HEAD_LEN + indexlen * sizeof(int);
	char *enddata = addr + len;

	binlogver = *(unsigned int *)(addr + sizeof(short));
	DINFO("binlogver: %d, dumplogver: %d\n", binlogver, dumplogver);
	if (binlogver == dumplogver) {
		int *indxdata = (int *)(addr + SYNCLOG_HEAD_LEN);
		int pos;

		if (dumplogpos < SYNCLOG_INDEXNUM) {
			pos = indxdata[dumplogpos];
		} else {
			pos = 0;
		}
		DINFO("dumplogpos: %d, pos: %d\n", dumplogpos, pos);
		if (pos == 0 && dumplogpos != 0) {
			DINFO("indxdata[dumplogpos - 1]=%d\n", indxdata[dumplogpos - 1]);
			if (indxdata[dumplogpos - 1] != 0) {
				data = addr + indxdata[dumplogpos - 1];
				have_key = 1;
			}else{
				g_runtime->slave->logver  = dumplogver;
				g_runtime->slave->logline = dumplogpos;
				munmap(addr, len);

				close(ffd);
				return dumplogpos;
			}
		}else{
			if (pos != 0) {
				data = addr + pos;
			}
		}
	}

	unsigned int blen; 
	unsigned int logver = 0, logline = 0, count = 0;
	while (data < enddata) {
		//blen = *(unsigned short*)(data + SYNCPOS_LEN);
		blen = *(unsigned int*)(data + SYNCPOS_LEN);
			

		memcpy(&logver, data, sizeof(int));
		memcpy(&logline, data + sizeof(int), sizeof(int));
		
		DINFO("logver: %d, logline: %d\n", logver, logline);
		if (enddata < data + SYNCPOS_LEN + blen + sizeof(int)) {
			DERROR("synclog end error: %s, skip\n", logname);
			//MEMLINK_EXIT;
			break;
		}
		DINFO("command, len:%d\n", blen);
		if (have_key == 0) {
			ret = wdata_apply(data + SYNCPOS_LEN, blen + sizeof(int), MEMLINK_NO_LOG, NULL);
			if (ret != 0) {
				DERROR("wdata_apply log error: %d\n", ret);
				MEMLINK_EXIT;
			}
			count ++;
		}

		data += SYNCPOS_LEN + blen + sizeof(int); 
	}

	if (g_cf->role == ROLE_SLAVE) {
		g_runtime->slave->logver  = logver;
		g_runtime->slave->logline = logline;
		if (dumplogver == binlogver) {
			g_runtime->slave->binlog_index = count + dumplogpos;
		}else{
			g_runtime->slave->binlog_index = count;
		}
	}

	munmap(addr, len);

	close(ffd);

	return count;
}

static int
load_data()
{
    int    ret;
    int    havedump = 0;
    char   filename[PATH_MAX];
	char   dumpfileok[PATH_MAX];

    snprintf(filename, PATH_MAX, "%s/dump.dat", g_cf->datadir);
    // check dumpfile exist
	/*
    ret = stat(filename, &stbuf);
    if (ret == -1 && errno == ENOENT) {
        DINFO("not found dumpfile: %s\n", filename);
    }*/
 	ret = isfile(filename);
	if (ret == 0) {
		DINFO("node fount dumpfile: %s\n", filename);
		snprintf(dumpfileok, PATH_MAX, "%s/dump.dat.ok", g_cf->datadir);
		if (!isfile(dumpfileok)) {
			ret = 0;
		} else {
			rename(dumpfileok, filename);
			ret = 1;
		}
	}	
  
    // have dumpfile, load
    if (ret == 1) {
        havedump = 1;
    
        DINFO("try load dumpfile ...\n");
        ret = dumpfile_load(g_runtime->ht, filename, 1);
        if (ret < 0) {
            DERROR("dumpfile_load error: %d\n", ret);
            MEMLINK_EXIT;
            return -1;
        }
    }

    int n;
	int i;
    char logname[PATH_MAX];
    int  logids[10000] = {0};
		
	DINFO("havedump: %d\n", havedump);
	n = synclog_scan_binlog(logids, 10000);
	if (n < 0) {
		DERROR("get binlog error! %d\n", n);
		return -1;
	}
	DINFO("dumplogver: %d, n: %d\n", g_runtime->dumplogver, n);
	//没有dump.dat也没有dump.dat.ok,且本地binlog不是从1开始记录的
	if (havedump == 0) {
		/*
		if (logids[0] == 0 && g_runtime->logver != 1) {//本地只有bin.log，且bin.log的logver不为1
			DERROR("no dump.dat, binlog version must start from 1. logver: %d\n", g_runtime->logver);
			MEMLINK_EXIT;
		} else {
			if (logids[0] != 1 ) {//本地有一系列的bin.log.xxx，但是logver不是从1开始的
				DERROR("no dump.dat, binlog version must start from 1. binlog version start: %d\n", logids[0]);
				MEMLINK_EXIT;
			}
		}	
		*/
		if (!(logids[0] == 1 || g_runtime->logver == 1)) {
			DERROR("no dump.dat, binlog version must start from 1. binlog version start: %d\n", logids[0]);
			MEMLINK_EXIT;
		}
	}

    unsigned int count = 0;
    for (i = 0; i < n; i++) {
		if (logids[i] < g_runtime->dumplogver) {
			continue;
		}
        //snprintf(logname, PATH_MAX, "%s/data/bin.log.%d", g_runtime->home, logids[i]);
        snprintf(logname, PATH_MAX, "%s/bin.log.%d", g_cf->datadir, logids[i]);
        DINFO("load synclog: %s\n", logname);
        count += load_synclog(logname, g_runtime->dumplogver, g_runtime->dumplogpos);
    }

    //snprintf(logname, PATH_MAX, "%s/data/bin.log", g_runtime->home);
    snprintf(logname, PATH_MAX, "%s/bin.log", g_cf->datadir);
    DINFO("load synclog: %s\n", logname);
    count += load_synclog(logname, g_runtime->dumplogver, g_runtime->dumplogpos);
	
    if (havedump == 0) {
        dumpfile(g_runtime->ht);
    }
    DNOTE("load binlog %u ok!\n", count);
    return 0;
}

static int
load_data_slave()
{
    int    ret;
    //struct stat stbuf;
    int    havedump = 0;
	int    load_master_dump = 0;
    char   dump_filename[PATH_MAX];
	char   master_filename[PATH_MAX];
	char   dumpfileok[PATH_MAX];

    snprintf(dump_filename, PATH_MAX, "%s/dump.dat", g_cf->datadir);
    snprintf(master_filename, PATH_MAX, "%s/dump.master.dat", g_cf->datadir);
	if (!isfile(dump_filename)) {
		snprintf(dumpfileok, PATH_MAX, "%s/dump.dat.ok", g_cf->datadir);
		if(isfile(dumpfileok)) {
			rename(dumpfileok, dump_filename);
		}
	}
    // check dumpfile exist
	if (isfile(dump_filename) == 0) {
		if (isfile(master_filename)) {
			// load dump.master.dat
			DINFO("try load master dumpfile ...\n");
			ret = dumpfile_load(g_runtime->ht, master_filename, 0);
			if (ret < 0) {
				DERROR("dumpfile_load error: %d\n", ret);
				MEMLINK_EXIT;
				return -1;
			}
			SSlave	*slave = g_runtime->slave;

            dumpfile_logver(master_filename, &slave->logver, &slave->logline);
			load_master_dump = 1;
			//如果要加载dump.master.dat，清空本地binlog
			synclog_clean(slave->logver, slave->logline);
		}
	}else{
		// have dumpfile, load
        havedump = 1;
    
        DINFO("try load dumpfile ...\n");
        ret = dumpfile_load(g_runtime->ht, dump_filename, 1);
        if (ret < 0) {
            DERROR("dumpfile_load error: %d\n", ret);
            MEMLINK_EXIT;
            return -1;
        }

    }


    int n;
    char logname[PATH_MAX];
    int  logids[10000] = {0};
    unsigned int count = 0;
		
	n = synclog_scan_binlog(logids, 10000);
	if (n < 0) {
		DERROR("get binlog error! %d\n", n);
		return -1;
	}
    DINFO("dumplogver: %d, n: %d\n", g_runtime->dumplogver, n);

	if (load_master_dump == 0) {
		//没有dump.dat也没有dump.dat.ok,且本地binlog不是从1开始记录的
		if (havedump == 0) {
			/*
			if (logids[0] == 0 && g_runtime->logver != 1) {//本地只有bin.log，且bin.log的logver不为1
				DERROR("no dump.dat, binlog version must start from 1. logver: %d\n", g_runtime->logver);
				MEMLINK_EXIT;
			} else {
				if (logids[0] != 1 ) {//本地有一系列的bin.log.xxx，但是logver不是从1开始的
					DERROR("no dump.dat, binlog version must start from 1. binlog version start: %d\n", logids[0]);
					MEMLINK_EXIT;
				}
			}	
			*/
			if (!(logids[0] == 1 || g_runtime->logver == 1)) {
				DERROR("no dump.dat, binlog version must start from 1. binlog version start: %d\n", logids[0]);
				MEMLINK_EXIT;
			}

		}
		int i;
		DINFO("load binlog ...\n");
		for (i = 0; i < n; i++) {
			if (logids[i] < g_runtime->dumplogver) {
				continue;
			}
			snprintf(logname, PATH_MAX, "%s/bin.log.%d", g_cf->datadir, logids[i]);
			DINFO("load synclog: %s\n", logname);
			ret = load_synclog(logname, g_runtime->dumplogver, g_runtime->dumplogpos);
			if (ret > 0 && g_cf->role == ROLE_SLAVE) {
				g_runtime->slave->binlog_ver = logids[i];
			}
            count += ret;
		}
		snprintf(logname, PATH_MAX, "%s/bin.log", g_cf->datadir);
		DINFO("load synclog: %s\n", logname);
		ret = load_synclog(logname, g_runtime->dumplogver, g_runtime->dumplogpos);
		if (ret > 0 && g_cf->role == ROLE_SLAVE) {
			g_runtime->slave->binlog_ver = g_runtime->synclog->version;
		}
        count += ret;
	}

    if (havedump == 0) {
        dumpfile(g_runtime->ht);
    }

    if (load_master_dump == 1) {
        g_runtime->slave->binlog_ver = g_runtime->logver;
    }

	DINFO("======== slave binlog_ver:%d, binlog_index:%d, logver:%d, logline:%d, dump_logver:%d, dumpsize:%lld, dumpfile_size:%lld\n", g_runtime->slave->binlog_ver, g_runtime->slave->binlog_index, g_runtime->slave->logver, g_runtime->slave->logline, g_runtime->slave->dump_logver, g_runtime->slave->dumpsize, g_runtime->slave->dumpfile_size);

    DNOTE("load binlog %u ok!\n", count);

    return 0;
}

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
runtime_create_slave(char *pgname) 
{
    Runtime *rt;

    rt = runtime_create_common(pgname);
    if (NULL == rt) {
        return rt;
    }
    
    rt->slave = sslave_create();
    if (NULL == rt->slave) {
        DERROR("sslave_create error!\n");
        MEMLINK_EXIT;
    }
    DINFO("sslave thread create ok!\n");

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
runtime_create_master(char *pgname)
{
    Runtime* rt;// = runtime_init(pgname);

    rt = runtime_create_common(pgname);
    if (NULL == rt) {
        return rt;
    }

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
}

/**
 * @}
 */
