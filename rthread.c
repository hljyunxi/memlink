#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "logfile.h"
#include "rthread.h"
#include "hashtable.h"
#include "serial.h"
#include "myconfig.h"
#include "wthread.h"
#include "utils.h"

int
rdata_ready(Conn *conn, char *data, int datalen)
{
    char key[1024]; 
    //char value[1024];
    char cmd;
    int  ret = 0;
    char *msg = NULL;
    char *retdata = NULL;
    int  retlen = 0;

    //unsigned char   valuelen;
    unsigned char   masknum;
    unsigned int    maskarray[HASHTABLE_MASK_MAX_LEN];
    unsigned int    frompos, len;

    memcpy(&cmd, data + sizeof(short), sizeof(char));
    char buf[256] = {0};
    DINFO("data ready cmd: %d, data: %s\n", cmd, formath(data, datalen, buf, 256));

    switch(cmd) {
        case CMD_RANGE: {
            DINFO("<<< cmd RANGE >>>\n");
            ret = cmd_range_unpack(data, key, &masknum, maskarray, &frompos, &len);
            DINFO("unpack range return:%d, key:%s, masknum:%d, frompos:%d, len:%d\n", ret, key, masknum, frompos, len);
            unsigned char masksize = 0, valuesize = 0;
            int rlen = sizeof(char) * 2 + 256 * len + HASHTABLE_MASK_MAX_LEN * sizeof(int) * len;
            DINFO("ret buffer len: %d\n", rlen);
            char retrec[rlen];

            ret = hashtable_range(g_runtime->ht, key, maskarray, masknum, frompos, len, 
                                retrec + sizeof(char)*2, &retlen, &valuesize, &masksize); 
            DINFO("hashtable_range return: %d, retlen:%d, valuesize:%d, masksize:%d\n", ret, retlen, valuesize, masksize);
            /*
            if (retlen > 0) {
                printh(retrec + sizeof(char) * 2, retlen);
            }*/
            memcpy(retrec, &valuesize, sizeof(char));
            memcpy(retrec + sizeof(char), &masksize, sizeof(char));
           
            retlen += sizeof(char) * 2;

            if (ret < 0) {
                ret = 500;
            }else{
                ret = 200;
            }
            
            ret = data_reply(conn, ret, msg, retrec, retlen);
            DINFO("data_reply return: %d\n", ret);

            break;
        }
        case CMD_STAT: {
            DINFO("<<< cmd STAT >>>\n");
            HashTableStat   stat;
            ret = cmd_stat_unpack(data, key);
            DINFO("unpack stat return: %d, key: %s\n", ret, key);

            ret = hashtable_stat(g_runtime->ht, key, &stat);
            DINFO("hashtable stat: %d\n", ret);
            DINFO("stat blocks: %d\n", stat.blocks);

            retdata = (char*)&stat;
            retlen  = sizeof(HashTableStat);

            DINFO("data_reply return: %d\n", ret);

            if (ret < 0) {
                ret = 500;
            }else{
                ret = 200;
            }

            ret = data_reply(conn, ret, msg, retdata, retlen);
            DINFO("data_reply return: %d\n", ret);

            break;
        }
        default: {
            ret = 300;

            ret = data_reply(conn, ret, msg, retdata, retlen);
            DINFO("data_reply return: %d\n", ret);

        }
    }

    /*if (ret < 0) {
        ret = 500;
    }else{
        ret = 200;
    }

rdata_ready_over:
    ret = data_reply(conn, ret, msg, retdata, retlen);
    DINFO("data_reply return: %d\n", ret);
    */

    return 0;
}

