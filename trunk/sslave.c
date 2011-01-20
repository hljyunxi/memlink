#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h> 
#include <netinet/tcp.h> 
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>

#include "logfile.h"
#include "zzmalloc.h"
#include "utils.h"
#include "network.h"
#include "serial.h"
#include "synclog.h"
#include "common.h"
#include "dumpfile.h"
#include "sslave.h"

#define MAX_SYNC_PREV   100
#define SYNCPOS_LEN   (sizeof(int)+sizeof(int))

/**\
 * recv server push log
*/

#ifdef RECV_LOG_BY_PACKAGE 
static int
sslave_recv_package_log(SSlave *ss)
{
	char recvbuf[1024 * 1024];
	int  checksize = SYNCPOS_LEN + sizeof(int);
	int  ret;
	unsigned char first_check = 0;
	char *ptr = NULL;
	unsigned int package_len;
	unsigned int  rlen = 0;
    unsigned int    logver;
    unsigned int    logline;
	// send sync
    
    DINFO("slave recv log ...\n");
	while (1) {
		//读数据包长度
		ret = readn(ss->sock, &package_len, sizeof(int), 0);
		if (ret < sizeof(int)) {
			DERROR("read sync package_len too short: %d, close\n", ret);
			close(ss->sock);
			ss->sock = -1;
			return -1;
		}
		//读取数据包
		unsigned int need = package_len - sizeof(int);
		ret = readn(ss->sock, recvbuf, need, 0);
		if (ret < need) {
			DERROR("read sync command set too short: %d, close\n", ret);
			close(ss->sock);
			ss->sock = -1;
			return -1;
		}
		/*
		memcpy(&package_len, recvbuf, sizeof(int));
		if (ret < package_len) {
			DERROR("read sync command set too short: %d, close\n", ret);
			close(ss->sock);
			ss->sock = -1;
			return -1;
		}
		*/
		DINFO("package_len: %d\n", package_len);
		int count = 0;//统计读出命令的字节数
		ptr = recvbuf;
		while (count < package_len - sizeof(int)) {
			memcpy(&logver, ptr, sizeof(int));
			memcpy(&logline, ptr + sizeof(int), sizeof(int));
			memcpy(&rlen, ptr + SYNCPOS_LEN, sizeof(int));
			DINFO("logver: %d, logline: %d\n, rlen: %d", logver, logline, rlen);
			
			DINFO("ss->logver: %d, ss->logline: %d, index_pos: %d\n", ss->logver, ss->logline, g_runtime->synclog->index_pos);			
			//只需校验第一次接收到的命令
			if (first_check == 0) {
				if (logver == ss->logver && logline == ss->logline && g_runtime->synclog->index_pos != 0) {
					//int pos = g_runtime->synclog->index_pos;
					int *indxdata = (int *)(g_runtime->synclog->index + SYNCLOG_HEAD_LEN);
					DINFO("indxdata[logline]=%d\n", indxdata[logline]);		
					/*
					if (g_runtime->synclog->index_pos == SYNCLOG_INDEXNUM) {
						first_check = 1;
						count += SYNCPOS_LEN + sizeof(int) + rlen; 
						continue;
					}
					if (indxdata[pos] == 0 && indxdata[pos - 1] != 0) {
						first_check = 1;
						count += SYNCPOS_LEN + sizeof(int) + rlen;
						continue;
					}
					*/
					if (indxdata[logline] != 0) {
						first_check = 1;
						count += SYNCPOS_LEN + sizeof(int) + rlen;
						DINFO("package_len: %d, count: %d\n", package_len, count);
						DINFO("---------------------skip\n");
						ptr += SYNCPOS_LEN + sizeof(int) + rlen;
						continue;
					}
				}
			}
			unsigned int size = checksize + rlen;
			char cmd;
			memcpy(&cmd, ptr + SYNCPOS_LEN + sizeof(int), sizeof(char));
			pthread_mutex_lock(&g_runtime->mutex);
			ret = wdata_apply(ptr + SYNCPOS_LEN, rlen, MEMLINK_NO_LOG);
			DINFO("wdata_apply return: %d\n", ret);
			if (ret == 0) {
				DINFO("write to binlog\n");
				synclog_write(g_runtime->synclog, ptr, size);
				DINFO("after write to binlog\n");
			}
			pthread_mutex_unlock(&g_runtime->mutex);

			ss->logver = logver;
			ss->logline = logline;
			
			ptr += SYNCPOS_LEN + sizeof(int) + rlen;
			count += SYNCPOS_LEN + sizeof(int) + rlen;
		}

	}
	return 0;
}

#else
static int
sslave_recv_log(SSlave *ss)
{
    char recvbuf[1024];
    int  checksize = SYNCPOS_LEN + sizeof(int);
    int  ret;
    unsigned int  rlen = 0;
    unsigned int    logver;
    unsigned int    logline;
    // send sync

    DINFO("slave recv log ...\n");
    while (1) {
        ret = readn(ss->sock, recvbuf, checksize, 0);
        if (ret < checksize) {
            DERROR("read sync head too short: %d, close\n", ret);
            close(ss->sock);
            ss->sock = -1;
            return -1;
        }
        //DINFO("readn return:%d\n", ret);
        memcpy(&rlen, recvbuf + SYNCPOS_LEN, sizeof(int));
        ret = readn(ss->sock, recvbuf + checksize, rlen, 0);
        if (ret < rlen) {
            DERROR("read sync too short: %d, close\n", ret);
            close(ss->sock);
            ss->sock = -1;
            return -1;
        }
        //DINFO("recv log len:%d\n", rlen);
        memcpy(&logver, recvbuf, sizeof(int));
        memcpy(&logline, recvbuf + sizeof(int), sizeof(int));
        DINFO("logver:%d, logline:%d\n", logver, logline);
        // after getdump, must not skip first one
        if (logver == ss->logver && logline == ss->logline && g_runtime->synclog->index_pos != 0) {
            //skip first one
            //continue;
            //int pos = g_runtime->synclog->index_pos;
            int *indxdata = (int *)(g_runtime->synclog->index + SYNCLOG_HEAD_LEN);

            //上一条已经记录了
            if (indxdata[logline] != 0) {
                continue;
            }
        }

        unsigned int size = checksize + rlen;
        struct timeval start, end;
        char cmd;

		        memcpy(&cmd, recvbuf + SYNCPOS_LEN + sizeof(int), sizeof(char));
        pthread_mutex_lock(&g_runtime->mutex);
        gettimeofday(&start, NULL);
        ret = wdata_apply(recvbuf + SYNCPOS_LEN, rlen, 0);
        DINFO("wdata_apply return:%d\n", ret);
        if (ret == 0) {
            //DINFO("synclog index_pos:%d, pos:%d\n", g_runtime->synclog->index_pos, g_runtime->synclog->pos);
            synclog_write(g_runtime->synclog, recvbuf, size);
        }
        gettimeofday(&end, NULL);
        DNOTE("cmd:%d %d %u us\n", cmd, ret, timediff(&start, &end));
        pthread_mutex_unlock(&g_runtime->mutex);

        ss->logver  = logver;
        ss->logline = logline;
    }

    return 0;
}
#endif

/**
 * load master dump info
 */
int
sslave_load_master_dump_info(SSlave *ss, char *dumpfile, long long *filesize, long long *dumpsize, unsigned int *dumpver, unsigned int *logver)
{
	//char dumpfile[PATH_MAX];	
	int  ret;
	long long fsize;
	long long dsize;
	unsigned int mylogver;
	//snprintf(dumpfile, PATH_MAX, "%s/dump.master.dat", g_cf->datadir);
	
	if (!isfile(dumpfile)) {
		return -1;
	}

	FILE	*dumpf;	
	//dumpf = fopen(dumpfile, "r");
	dumpf = (FILE*)fopen64(dumpfile, "r");
	if (dumpf == NULL) {
		DERROR("open file %s error! %s\n", dumpfile, strerror(errno));
		return -1;
	}

	fseek(dumpf, 0, SEEK_END);
	fsize = ftell(dumpf);

	if (filesize) {
		*filesize = fsize;
	}

	if (fsize >= DUMP_HEAD_LEN) {
		fseek(dumpf, DUMP_HEAD_LEN - sizeof(long long), SEEK_SET);
		ret = fread(&dsize, 1, sizeof(long long), dumpf);
		if (ret != sizeof(long long)) {
			DERROR("fread error, ret:%d, %s\n", ret, strerror(errno));
			fclose(dumpf);
			return -1;
			//goto read_dump_over;
		}
		if (dumpsize) {
			*dumpsize = dsize;
		}

		//int pos = sizeof(short) + sizeof(int);
		int pos = sizeof(short);
		fseek(dumpf, pos, SEEK_SET);

		ret = fread(&mylogver, 1, sizeof(int), dumpf);
		if (ret != sizeof(int)) {
			DERROR("fread error: %s\n", strerror(errno));
			fclose(dumpf);
			return -1;
			//goto read_dump_over;
		}
		if (dumpver) {
			*dumpver = mylogver;
		}
		ret = fread(&mylogver, 1, sizeof(int), dumpf);
		if (ret != sizeof(int)) {
			DERROR("fread error: %s\n", strerror(errno));
			fclose(dumpf);
			return -1;
			//goto read_dump_over;
		}
		if (logver) {
			*logver = mylogver;
		}


	}
	fclose(dumpf);

	return 0;
}

/* 
 * find previous binlog position for sync
 */
/*
static int 
sslave_prev_sync_pos(SSlave *ss)
{
	char filepath[PATH_MAX];
	//int bver = ss->binlog_ver;
	int ret;
    int i;

    for (i = ss->trycount; i < MAX_SYNC_PREV; i++) {
	    ss->trycount += 1;		

        if (ss->binlog_index == 0) {
            if (ss->binlog) {
                synclog_destroy(ss->binlog);
                ss->binlog = NULL;
            }
            ret = synclog_prevlog(ss->binlog_ver);		
            if (ret <= 0) {
                DERROR("synclog_prevlog error: %d\n", ret);
                return -1;
            }
			DINFO("prev binlog: %d, current binlog: %d\n", ret, ss->binlog_ver);
            ss->binlog_ver   = ret;
            ss->binlog_index = 0;

            if (ss->binlog_ver != g_runtime->logver) {
                snprintf(filepath, PATH_MAX, "%s/bin.log.%d", g_cf->datadir, ss->binlog_ver);
            }else{
                snprintf(filepath, PATH_MAX, "%s/bin.log", g_cf->datadir);
            }
            DINFO("try open synclog: %s\n", filepath);
            ss->binlog = synclog_open(filepath);
            if (NULL == ss->binlog) {
                DERROR("open synclog error: %s\n", filepath);
                return -1;
            }

            ss->binlog_index = ss->binlog->index_pos;
        }
		
		if (NULL == ss->binlog) {
			if (ss->binlog_ver != g_runtime->logver) {
                snprintf(filepath, PATH_MAX, "%s/bin.log.%d", g_cf->datadir, ss->binlog_ver);
            }else{
                snprintf(filepath, PATH_MAX, "%s/bin.log", g_cf->datadir);
            }
			
			DINFO("open synclog: %s\n", filepath);
            ss->binlog = synclog_open(filepath);
            if (NULL == ss->binlog) {
                DERROR("open synclog error: %s\n", filepath);
                return -1;
            }
			ss->binlog_index = ss->binlog->index_pos;
		}

		DINFO("synclog index_pos:%d, pos:%d, len:%d\n", ss->binlog->index_pos, ss->binlog->pos, ss->binlog->len);
        ss->binlog_index -= 1;
        int *idxdata = (int*)(ss->binlog->index + SYNCLOG_HEAD_LEN);
        int pos      = idxdata[ss->binlog_index];
        
        DINFO("=== binlog_index:%d, pos: %d ===\n", ss->binlog_index, pos);

        lseek(ss->binlog->fd, pos, SEEK_SET);
        unsigned int logver, logline;
        ret = readn(ss->binlog->fd, &logver, sizeof(int), 0);
        if (ret != sizeof(int)) {
            DERROR("read logver error! %s\n", strerror(errno));
            continue;
            //return -1;
        }
        ret = readn(ss->binlog->fd, &logline, sizeof(int), 0);
        if (ret != sizeof(int)) {
            DERROR("read logline error! %s\n", strerror(errno));
            continue;
            //return -1;
        }
		
		DINFO("read logver:%d, logline:%d\n", logver, logline);
        ss->logver  = logver;
        ss->logline = logline;

        break;
    }
	if (ss->trycount >= MAX_SYNC_PREV)
		return -1;

	return 0;
}
*/


/**
 * send command and recv response
 */
int 
sslave_do_cmd(SSlave *ss, char *sendbuf, int buflen, char *recvbuf, int recvlen)
{
    int ret;

    //DINFO("send buflen: %d\n", buflen);
    ret = writen(ss->sock, sendbuf, buflen, ss->timeout);
    if (ret != buflen) {
        sslave_close(ss);
        DINFO("cmd writen error: %d, close.\n", ret);
        return -1;
    }
    
    ret = readn(ss->sock, recvbuf, sizeof(int), ss->timeout);
    if (ret != sizeof(int)) {
        sslave_close(ss);
        DINFO("cmd readn error: %d, close.\n", ret);
        return -2;
    }
    
    unsigned int len;
    memcpy(&len, recvbuf, sizeof(int));
    //DINFO("cmd recv len:%d\n", len);
    
    if (len + sizeof(int) > recvlen) {
        sslave_close(ss);
        DERROR("recv data too long: %d\n", (int)(len + sizeof(short)));
        return -3;
    }

    ret = readn(ss->sock, recvbuf + sizeof(int), len, ss->timeout);
    if (ret != len) {
        sslave_close(ss);
        DINFO("cmd readn error: %d, close.\n", ret);
        return -4;
    }

    return len + sizeof(int);
}

/**
 * get dump.dat from master
 */
int
sslave_do_getdump(SSlave *ss)
{
	char    dumpfile[PATH_MAX];
    char    dumpfile_tmp[PATH_MAX];

    snprintf(dumpfile, PATH_MAX, "%s/dump.master.dat", g_cf->datadir);
    snprintf(dumpfile_tmp, PATH_MAX, "%s/dump.master.dat.tmp", g_cf->datadir);

	long long	 tmpsize  = 0;
	long long	 dumpsize = 0;
	unsigned int dumpver  = 0;
	unsigned int logver   = 0;
	int			 ret;

	sslave_load_master_dump_info(ss, dumpfile_tmp, &tmpsize, &dumpsize, &dumpver, &logver);

	char sndbuf[1024];
    int  sndlen;
    char recvbuf[1024];

	// do getdump
    DINFO("try getdump, dumpver:%d, dumpsize:%lld, filesize:%lld\n", dumpver, dumpsize, tmpsize);
    sndlen = cmd_getdump_pack(sndbuf, dumpver, tmpsize); 
    ret = sslave_do_cmd(ss, sndbuf, sndlen, recvbuf, 1024); 
    if (ret < 0) {
        DERROR("cmd getdump error: %d\n", ret);
        return -1;
    }
    unsigned short retcode;
    unsigned long long size, rlen = 0;

    memcpy(&retcode, recvbuf + sizeof(int), sizeof(short));
    memcpy(&dumpver, recvbuf + CMD_REPLY_HEAD_LEN, sizeof(int));
    memcpy(&size, recvbuf + CMD_REPLY_HEAD_LEN + sizeof(int), sizeof(long long));
    DINFO("recvlen:%d, retcode:%d, dumpver:%d, dump size: %lld\n", ret, retcode, dumpver, size);

    char    dumpbuf[8192];
    int     buflen = 0;
    int     fd = 0;
	int		oflag; 

	if (retcode == CMD_GETDUMP_OK) {
		oflag = O_CREAT|O_WRONLY|O_APPEND;
	}else{
		oflag = O_CREAT|O_WRONLY|O_TRUNC;
	}

    fd = open(dumpfile_tmp, oflag, 0644);
    if (fd == -1) {
        DERROR("open dumpfile %s error! %s\n", dumpfile_tmp, strerror(errno));
        MEMLINK_EXIT;
    }

    int rsize = 8192;
    while (rlen < size) {
        rsize = size - rlen;
        if (rsize > 8192) {
            rsize = 8192;
        }
        ret = readn(ss->sock, dumpbuf, rsize, ss->timeout);    
        if (ret < 0) {
            DERROR("read dump error: %d\n", ret);
            goto sslave_do_getdump_error;
        }
        if (ret == 0) {
            DINFO("read eof! close conn:%d\n", ss->sock);
            goto sslave_do_getdump_error;
        }
        buflen = ret;
        ret = writen(fd, dumpbuf, buflen, 0);
        if (ret < 0) {
            DERROR("write dump error: %d, %s\n", ret, strerror(errno));
            goto sslave_do_getdump_error;
        }

        rlen += ret;
        DINFO("recv dump size:%lld\n", rlen);
    }

    close(fd);
    
    ret = rename(dumpfile_tmp, dumpfile);
    if (ret == -1) {
        DERROR("dump file rename error: %s\n", strerror(errno));
        MEMLINK_EXIT; 
    }
	return 0;

sslave_do_getdump_error:
    close(ss->sock);
    ss->sock = -1;
    close(fd);
    return -1;
}

/**
 * apply sync,getdump commands before recv server push log
 */
int
sslave_conn_init(SSlave *ss)
{
    char sndbuf[1024];
    int  sndlen;
    char recvbuf[1024];
    //int  recvlen;
    int  ret;
    //unsigned int  local_logver_start;
    //unsigned int  local_logpos_start;
    unsigned int  dumpver;
    //long long     dumpsize;
	char    mdumpfile[PATH_MAX];
	char    mdumpfile_tmp[PATH_MAX];

    snprintf(mdumpfile, PATH_MAX, "%s/dump.master.dat", g_cf->datadir);
    snprintf(mdumpfile_tmp, PATH_MAX, "%s/dump.master.dat.tmp", g_cf->datadir);

    DINFO("slave init ...\n");
    DINFO("ss->dumpsize:%lld, ss->dumpfile_size:%lld, ss->dump_logver:%d\n", ss->dumpsize, ss->dumpfile_size, ss->dump_logver);

	//if (ss->dump_logver == 0) {
	//	sslave_load_master_dump_info(ss, mdumpfile, &ss->dumpfile_size, &ss->dumpsize, &ss->dump_logver);
	//}

	sslave_load_master_dump_info(ss, mdumpfile, &ss->dumpfile_size, &ss->dumpsize, &dumpver, &ss->dump_logver);

	if (ss->logver == 0) {
		ss->logver  = ss->dump_logver;
		ss->logline = 0;
	}

    //local_logver_start = ss->binlog_ver;
    //local_logpos_start = ss->binlog_index;

    if (ss->dumpsize == ss->dumpfile_size) { // have master dumpfile, and size ok or not have master dumpfile
        // send sync command 
		int is_getdump = 0;
        while (1) {
            //logver  = ss->logver;
            //logline = ss->logline;
			DINFO("binlog ver:%d, binlog index:%d\n", ss->binlog_ver, ss->binlog_index);
			DINFO("dump logver:%d, dumpsize:%lld, dumpfilesize:%lld\n", 
                    ss->dump_logver, ss->dumpsize, ss->dumpfile_size);
			DINFO("sync pack logver: %u, logline: %u\n", ss->logver, ss->logline);

            sndlen = cmd_sync_pack(sndbuf, ss->logver, ss->logline); 
            ret = sslave_do_cmd(ss, sndbuf, sndlen, recvbuf, 1024);
            if (ret < 0) {
                DINFO("cmd sync error: %d\n", ret);
                return -1;
            }
            //DINFO("sync cmd return len:%d\n", ret);
            char syncret; 
            int  i = sizeof(int);

            syncret = recvbuf[i];
            DINFO("sync return code:%d\n", syncret);
            if (syncret == CMD_SYNC_OK) { // ok, recv synclog
                DINFO("sync ok, logver:%d, logline:%d\n", ss->binlog_ver, ss->binlog_index);
                //if ((ss->binlog_ver < local_logver_start || ss->binlog_index < local_logpos_start) && is_getdump != 1) {
                    //synclog_resize(ss->binlog_ver, ss->binlog_index);
                //}
                DINFO("sync ok! try recv push message.\n");
                return 0;
            }else if (syncret == CMD_SYNC_FAILED && \
					  is_getdump == 1 && ss->logline == 0) {
				sleep(1);
				continue;
			}

            // try find the last sync position
			DINFO("try find the last sync position .\n");
			//ret = sslave_prev_sync_pos(ss);
			DINFO("no prev log, try getdump.\n");
			//dumpsize = 0;
			//break;
			if (syncret == CMD_SYNC_FAILED) {
				if (sslave_do_getdump(ss) == 0) {
					DINFO("load dump ...\n");
					hashtable_clear_all(g_runtime->ht);

					dumpfile_load(g_runtime->ht, mdumpfile, 1);
					g_runtime->synclog->index_pos = g_runtime->dumplogpos;

					sslave_load_master_dump_info(ss, mdumpfile, NULL, NULL, &dumpver, &ss->logver);
					//add by lanwenhong
					g_runtime->synclog->version = ss->logver;
					g_runtime->logver = ss->logver;
					
					dumpfile(g_runtime->ht);
					DINFO("ss->logver: %d, dumplogpos: %d\n", ss->logver, g_runtime->dumplogpos);
					memcpy((g_runtime->synclog->index + sizeof(short)), &(ss->logver), sizeof(int));

					//ss->logline = 0;
					ss->logline = g_runtime->dumplogpos;
					is_getdump = 1;
					synclog_clean(ss->logver, g_runtime->dumplogpos);
				}else{
					return -1;
				}
			}
		} 
	}

	return 0;
}

/**
 * slave sync thread
 * 1.find local sync logver/logline 
 * 2.send sync command to server or get dump
 * 3.recv server push log
 */
void*
sslave_run(void *args)
{
	SSlave	*ss = (SSlave*)args;
	int		ret;

    // do not start immediately, wait for some initialize
	while (ss->isrunning == 0) {
		sleep(1);
	}

    DINFO("sslave run ...\n");
	while (1) {
		if (ss->sock <= 0) {
            DINFO("connect to master %s:%d timeout:%d\n", g_cf->master_sync_host, g_cf->master_sync_port, ss->timeout);
			ret = tcp_socket_connect(g_cf->master_sync_host, g_cf->master_sync_port, ss->timeout);
			if (ret <= 0) {
				DERROR("connect error! ret:%d, continue.\n", ret);
				sleep(1);
				continue;
			}
            ss->sock = ret;

            DINFO("connect to master ok!\n");
			// do sync, getdump
			ret = sslave_conn_init(ss);
			if (ret < 0) {
                DINFO("slave conn init error!\n");
                sleep(1);
				continue;
			}
			//break;
		}
		// recv sync log from master
#ifdef RECV_LOG_BY_PACKAGE
		ret = sslave_recv_package_log(ss);
#else
		ret = sslave_recv_log(ss);
#endif
	}

	return NULL;
}

SSlave*
sslave_create() 
{
    SSlave *ss;

    ss = (SSlave*)zz_malloc(sizeof(SSlave));
    if (ss == NULL) {
        DERROR("sslave malloc error.\n");
        MEMLINK_EXIT;
    }
    memset(ss, 0, sizeof(SSlave));

	ss->timeout = g_cf->timeout;

    pthread_t threadid;
    pthread_attr_t attr;
    int ret;

    ret = pthread_attr_init(&attr);
    if (ret != 0) {
        DERROR("pthread_attr_init error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }

    ret = pthread_create(&threadid, &attr, sslave_run, ss);
    if (ret != 0) {
        DERROR("pthread_create error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }
    
    ret = pthread_detach(threadid);
    if (ret != 0) {
        DERROR("pthread_detach error; %s\n", strerror(errno));
        MEMLINK_EXIT;
    }

    DINFO("create sync slave thread ok!\n");

    return ss;
}

void
sslave_go(SSlave *ss)
{
	ss->isrunning = 1;
}

void
sslave_close(SSlave *ss)
{
    if (ss->sock) {
        close(ss->sock);
        ss->sock = -1;
    }
    
    ss->logver  = 0;
    ss->logline = 0;
}

void 
sslave_destroy(SSlave *ss) 
{
    if (ss->sock) {
        close(ss->sock);
        ss->sock = -1;
    }
    zz_free(ss);
}


