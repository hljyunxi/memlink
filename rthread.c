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
    //char value[1024];
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

    gettimeofday(&start, NULL);
    memcpy(&cmd, data + sizeof(int), sizeof(char));
    char buf[256] = {0};
    DINFO("data ready cmd: %d, data: %s\n", cmd, formath(data, datalen, buf, 256));

    switch(cmd) {
		case CMD_PING: {
			ret = MEMLINK_OK;
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

            break;
        }
		case CMD_STAT_SYS: {
            DINFO("<<< cmd STAT_SYS >>>\n");
            HashTableStatSys   stat;

            ret = hashtable_stat_sys(g_runtime->ht, &stat);
            DINFO("hashtable stat sys: %d\n", ret);
    
            retdata = (char*)&stat;
            retlen  = sizeof(HashTableStatSys);

            ret = data_reply(conn, ret, retdata, retlen);
            DINFO("data_reply return: %d\n", ret);
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
            break;
        }
        default: {
            ret = MEMLINK_ERR_CLIENT_CMD;

            ret = data_reply(conn, ret, retdata, retlen);
            DINFO("data_reply return: %d\n", ret);

        }
    }

    gettimeofday(&end, NULL);
    DNOTE("%s:%d cmd:%d use %u us\n", conn->client_ip, conn->client_port, cmd, timediff(&start, &end));

    return 0;

rdata_ready_error:
	ret = data_reply(conn, ret, NULL, 0);
    gettimeofday(&end, NULL);
    DNOTE("%s:%d cmd:%d use %u us\n", conn->client_ip, conn->client_port, cmd, timediff(&start, &end));
	DINFO("data_reply return: %d\n", ret);

    return 0;
}

