/** 
 * 读操作线程
 * @file rthread.c
 * @author zhaowei
 * @ingroup memlink
 * @{
 */
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "logfile.h"
#include "rthread.h"
#include "hashtable.h"
#include "serial.h"
#include "myconfig.h"
#include "wthread.h"
#include "utils.h"
#include "common.h"
#include "info.h"
#include "runtime.h"

/**
 * Execute the read command and send response.
 *
 * @param conn connection
 * @param data command data
 * @param datalen the length of data parameter
 */
int
rdata_ready(Conn *conn, char *data, int datalen)
{
    char key[512] = {0}; 
    //char value[512] = {0};
    char cmd;
    int  ret = 0;
    //char *msg = NULL;
    char *retdata = NULL;
    int  retlen = 0;
    //unsigned char   valuelen;
    unsigned char   masknum;
    unsigned int    maskarray[HASHTABLE_MASK_MAX_ITEM] = {0};
    int    frompos, len;
    struct timeval start, end;
	ThreadServer *st = (ThreadServer *)conn->thread;
	RwConnInfo *conninfo = NULL; 


	int i;
	for (i = 0; i <= g_cf->max_conn; i++) {
		conninfo = &(st->rw_conn_info[i]);
		if (conninfo->fd == conn->sock)
			break;
	}

    gettimeofday(&start, NULL);
    memcpy(&cmd, data + sizeof(int), sizeof(char));
    char buf[256] = {0};
    DINFO("data ready cmd: %d, data: %s\n", cmd, formath(data, datalen, buf, 256));

    switch(cmd) {
		case CMD_PING: {
			ret = MEMLINK_OK;
            if (conninfo)
                conninfo->cmd_count++;
            goto rdata_ready_error;
			break;
		}
        case CMD_RANGE: {
            DINFO("<<< cmd RANGE >>>\n");
			unsigned char kind;
            ret = cmd_range_unpack(data, key, &kind, &masknum, maskarray, &frompos, &len);
            DINFO("unpack range return:%d, key:%s, masknum:%d, frompos:%d, len:%d\n", ret, key, masknum, frompos, len);

			if (frompos < 0 || len <= 0) {
				DERROR("from or len small than 0. from:%d, len:%d\n", frompos, len);
				ret = MEMLINK_ERR_RANGE_SIZE;
				goto rdata_ready_error;
			}

			if (key[0] == 0) {
				ret = MEMLINK_ERR_PARAM;
				goto rdata_ready_error;
			}
            // len(4B) + retcode(2B) + valuesize(1B) + masksize(1B) + masknum(1B) + maskformat(masknum B) + value.mask * len
            //int wlen = sizeof(int) + sizeof(short) + sizeof(char) + sizeof(char) + sizeof(char) + 
			//		   masknum * sizeof(int) + (HASHTABLE_VALUE_MAX + (HASHTABLE_MASK_MAX_BIT/8 + 2) * masknum) * len;
            ret = hashtable_range(g_runtime->ht, key, kind, maskarray, masknum, frompos, len, conn); 
            DINFO("hashtable_range return: %d\n", ret);
            //ret = data_reply(conn, ret, retrec, retlen);
            ret = data_reply_direct(conn);
            DINFO("data_reply return: %d\n", ret);
            if (conninfo)
                conninfo->cmd_count++;

            //hashtable_print(g_runtime->ht, key);

            break;
        }
        case CMD_SL_RANGE: {
            DINFO("<<< cmd SL_RANGE >>>\n");
			unsigned char kind;
            char valmin[512] = {0};
            char valmax[512] = {0};
            unsigned char vminlen = 0, vmaxlen = 0;

            ret = cmd_sortlist_range_unpack(data, key, &kind, &masknum, maskarray, 
                                            valmin, &vminlen, valmax, &vmaxlen);
            DINFO("unpack range return:%d, key:%s, masknum:%d, vmin:%s,%d, vmax:%s,%d, len:%d\n", 
                            ret, key, masknum, valmin, vminlen, valmax, vmaxlen, len);

            /*
			if (frompos < 0 || len <= 0) {
				DERROR("from or len small than 0. from:%d, len:%d\n", frompos, len);
				ret = MEMLINK_ERR_RANGE_SIZE;
				goto rdata_ready_error;
			}*/

			if (key[0] == 0) {
				ret = MEMLINK_ERR_PARAM;
				goto rdata_ready_error;
			}
            // len(4B) + retcode(2B) + valuesize(1B) + masksize(1B) + masknum(1B) + maskformat(masknum B) + value.mask * len
            //int wlen = sizeof(int) + sizeof(short) + sizeof(char) + sizeof(char) + sizeof(char) + 
			//		   masknum * sizeof(int) + (HASHTABLE_VALUE_MAX + (HASHTABLE_MASK_MAX_BIT/8 + 2) * masknum) * len;

            //hashtable_print(g_runtime->ht, key);

            ret = hashtable_sortlist_range(g_runtime->ht, key, kind, maskarray, masknum, 
                                            valmin, valmax, conn); 
            DINFO("hashtable_range return: %d\n", ret);
            //ret = data_reply(conn, ret, retrec, retlen);
            ret = data_reply_direct(conn);
            DINFO("data_reply return: %d\n", ret);
            if (conninfo)
                conninfo->cmd_count++;
            break;
        }
        case CMD_STAT: {
            DINFO("<<< cmd STAT >>>\n");
            HashTableStat   stat;
            ret = cmd_stat_unpack(data, key);
            DINFO("unpack stat return: %d, key: %s\n", ret, key);

			if (key[0] == 0) {
				ret = MEMLINK_ERR_PARAM;
				goto rdata_ready_error;
			}

            ret = hashtable_stat(g_runtime->ht, key, &stat);
            DINFO("hashtable stat: %d\n", ret);
            DINFO("stat blocks: %d\n", stat.blocks);
	
            //hashtable_print(g_runtime->ht, key); 
            retdata = (char*)&stat;
            retlen  = sizeof(HashTableStat);

            ret = data_reply(conn, ret, retdata, retlen);
            DINFO("data_reply return: %d\n", ret);
            if (conninfo)
                conninfo->cmd_count++;
            break;
        }
		case CMD_STAT_SYS: {
            DINFO("<<< cmd STAT_SYS >>>\n");
            //HashTableStatSys   stat;
			MemLinkStatSys stat;

            //ret = hashtable_stat_sys(g_runtime->ht, &stat);
			ret = info_sys_stat(&stat);
            DINFO("hashtable stat sys: %d\n", ret);
    
            retdata = (char*)&stat;
            retlen  = sizeof(HashTableStatSys);

            ret = data_reply(conn, ret, retdata, retlen);
            DINFO("data_reply return: %d\n", ret);
            if (conninfo)
                conninfo->cmd_count++;
            break;
        }
        case CMD_COUNT: {
            DINFO("<<< cmd COUNT >>>\n");
			unsigned char masknum;
			//unsigned int  maskarray[HASHTABLE_MASK_MAX_ITEM * sizeof(int)];

            ret = cmd_count_unpack(data, key, &masknum, maskarray);
            DINFO("unpack count return: %d, key: %s, mask:%d:%d:%d\n", ret, key, maskarray[0], maskarray[1], maskarray[2]);
           
			if (key[0] == 0) {
				ret = MEMLINK_ERR_PARAM;
				goto rdata_ready_error;
			}

            int vcount = 0, mcount = 0;
            retlen = sizeof(int) * 2;
            char retrec[retlen];

            ret = hashtable_count(g_runtime->ht, key, maskarray, masknum, &vcount, &mcount);
            DINFO("hashtable count, ret:%d, visible_count:%d, tagdel_count:%d\n", ret, vcount, mcount);
            memcpy(retrec, &vcount, sizeof(int));
            memcpy(retrec + sizeof(int), &mcount, sizeof(int));
            //retlen = sizeof(int) + sizeof(int);

            ret = data_reply(conn, ret, retrec, retlen);
            DINFO("data_reply return: %d\n", ret);
            if (conninfo)
                conninfo->cmd_count++;
            break;
        }
        case CMD_SL_COUNT: {
            DINFO("<<< cmd SL_COUNT >>>\n");
            char valmin[512] = {0};
            char valmax[512] = {0};
            unsigned char vminlen = 0, vmaxlen = 0;

            ret = cmd_sortlist_count_unpack(data, key, &masknum, maskarray, 
                                            valmin, &vminlen, valmax, &vmaxlen);
            DINFO("unpack range return:%d, key:%s, masknum:%d, vmin:%s,%d, vmax:%s,%d, len:%d\n", 
                            ret, key, masknum, valmin, vminlen, valmax, vmaxlen, len);

			if (key[0] == 0) {
				ret = MEMLINK_ERR_PARAM;
				goto rdata_ready_error;
			}
            int vcount = 0, mcount = 0;
            retlen = sizeof(int) * 2;
            char retrec[retlen];

            ret = hashtable_sortlist_count(g_runtime->ht, key, maskarray, masknum, 
                                            valmin, valmax, &vcount, &mcount); 
            DINFO("hashtable sortlist count, ret:%d, visible_count:%d, tagdel_count:%d\n", ret, vcount, mcount);
            memcpy(retrec, &vcount, sizeof(int));
            memcpy(retrec + sizeof(int), &mcount, sizeof(int));
            //retlen = sizeof(int) + sizeof(int);
            ret = data_reply(conn, ret, retrec, retlen);
            DINFO("data_reply return: %d\n", ret);
            if (conninfo)
                conninfo->cmd_count++;
            break;
        }
		case CMD_READ_CONN_INFO: {
			DINFO("<<< cmd READ_CONN_INFO >>>\n");
            if (conninfo)
                conninfo->cmd_count++;
            ret = info_read_conn(conn);
			ret = data_reply_direct(conn);
			DINFO("data_reply return: %d\n", ret);
			break;

		}
		case CMD_WRITE_CONN_INFO: {
			DINFO("<<< cmd WRITE_CONN_INFO >>>\n");
            if (conninfo)
                conninfo->cmd_count++;
            ret = info_write_conn(conn);
			DINFO("write_conn_info return: %d\n", ret);
			ret = data_reply_direct(conn);
			DINFO("data_reply return: %d\n", ret);
			break;
		}
		case CMD_SYNC_CONN_INFO: {
			DINFO("<<< cmd SYNC_CONN_INFO >>>\n");
            if (conninfo)
                conninfo->cmd_count++;
            ret = info_sync_conn(conn);
			DINFO("sync_conn_info return: %d\n", ret);
			ret = data_reply_direct(conn);
			DINFO("data_reply return: %d\n", ret);
			break;
		}
        case CMD_CONFIG_INFO: {
            DINFO("<<< cmd CMD_CONFIG_INFO >>>\n");
            ret = info_sys_config(conn);
            ret = data_reply_direct(conn);
            DINFO("data_reply return: %d\n", ret);
            break;
        }
        default: {
            ret = MEMLINK_ERR_CLIENT_CMD;

            ret = data_reply(conn, ret, retdata, retlen);
            DINFO("data_reply return: %d\n", ret);

        }
    }

    gettimeofday(&end, NULL);
    DNOTE("%s:%d cmd:%d sendret:%d use %u us\n", conn->client_ip, conn->client_port, cmd,ret,timediff(&start, &end));

    return 0;

rdata_ready_error:
	ret = data_reply(conn, ret, NULL, 0);
    gettimeofday(&end, NULL);
    DNOTE("%s:%d cmd:%d sendret:%d use %u us\n", conn->client_ip, conn->client_port, cmd,ret,timediff(&start, &end));

    return 0;
}
/**
 * @}
 */
