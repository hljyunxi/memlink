#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "logfile.h"
#include "rthread.h"
#include "hashtable.h"
#include "serial.h"
#include "myconfig.h"
#include "wthread.h"

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
    DINFO("data ready cmd: %d\n", cmd);
    
    switch(cmd) {
        case CMD_RANGE: {
            DINFO("<<< cmd RANGE >>>\n");
            ret = cmd_range_unpack(data, key, &masknum, maskarray, &frompos, &len);
            DINFO("unpack range return: %d\n", ret);
            break;
        }
        case CMD_STAT: {
            DINFO("<<< cmd STAT >>>\n");
            HashTableStat   stat;
            ret = cmd_stat_unpack(data, key);
            DINFO("unpack stat return: %d\n", ret);

            ret = hashtable_stat(g_runtime->ht, key, &stat);
            DINFO("hashtable stat: %d\n", ret);
            DINFO("stat blocks: %d\n", stat.blocks);

            retdata = (char*)&stat;
            retlen  = sizeof(HashTableStat);

            DINFO("data_reply return: %d\n", ret);

            break;
        }
        default: {
            ret = 300;

            goto rdata_ready_over;
        }
    }

    if (ret < 0) {
        ret = 500;
    }else{
        ret = 200;
    }

rdata_ready_over:
    ret = data_reply(conn, ret, msg, retdata, retlen);
    DINFO("data_reply return: %d\n", ret);

    return 0;
}

