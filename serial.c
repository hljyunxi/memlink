/**
 * 数据包编码解码
 * @file serial.c
 * @author zhaowei
 * @ingroup memlink
 * @{
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "serial.h"
#include "myconfig.h"
#include "base/logfile.h"
#include "base/utils.h"
#include "base/zzmalloc.h"
#include "base/pack.h"
/**
 * 把字符串形式的attr转换为数组形式
 */
/**
 * Convert a attr string to an integer array. Digit characters delimited by : is 
 * converted into the correspoding integer. : is converted into UINT_MAX.
 *
 * @param attrstr attr string
 * @param result converted integer array
 * @return the count of the converted integers
 */
int
attr_string2array(char *attrstr, unsigned int *result)
{
    char *m = attrstr;
    int  i = 0;

    if (m[0] == 0) {
        return 0;
    }

    while (m != NULL) {
        char *fi  = strchr(m, ':');
        if (i >= HASHTABLE_ATTR_MAX_ITEM)
            return -1;
        if (m[0] == ':' || m[0] == 0) {
            result[i] = UINT_MAX;
        }else{
        //modifed by wyx 12.27
            unsigned long num = strtoul(m, NULL, 10);
            if (num >= UINT_MAX){
                //DERROR("num:%lu\n", num);
                return MEMLINK_ERR_PARAM;
            }
            result[i] = num; 
        }
        i++;
        if (fi != NULL) {
            m = fi + 1;
        }else{
            m = NULL;
        }
    }
    return i;
}

/**
 * attr必须先初始化为0
 */
int
attr_array2binary(unsigned char *attrformat, unsigned int *attrarray, char attrnum, char *attr)
{
    int i;
    int b   = 0; // 已经处理的bit
    int idx = 0;
    int n;
    unsigned int v, flow = 0;
    char    mf;
    // 前两位分别表示真实删除和标记删除，跳过
    // attr[idx] = attr[idx] & 0xfc;
    //memset(attr, 0, attrnum);
    
    attr[0] = 0x01;  // 默认设置数据有效，非标记删除
    b += 2;

    for (i = 0; i < attrnum; i++) {
        mf = attrformat[i];
        if (mf == 0) {
            DERROR("attr format error: %d\n", mf);
            return -1;
        }
        v  = attrarray[i]; 
        //DINFO("idx:%d, b:%d, format:%d, attrvalue:%d, %x\n", idx, b, mf, v, v);

        if (v == UINT_MAX) {
            v = 0;
        }
        
        unsigned char y = (b + mf) % 8;
        n = (b + mf) / 8 + (y>0 ? 1: 0);
        if (n > 4) {
            flow = v >> (32 - b);
        }
        v = v << b;
        unsigned char m = 0xffffffff >> (32 - b);
        unsigned char x = attr[idx] & m;

        v = v | x;
        if (n > 4) {
            memcpy(&attr[idx], &v, sizeof(int));
        } else {
            memcpy(&attr[idx], &v, n);
        }

        if (y > 0) {
            idx += n - 1;
        }else{
            idx += n;
        }
        if (n > 4) {
            attr[idx] = attr[idx] | flow;
        }

        b = y;

        //DINFO("============================\n");
    }
    n = b / 8;
    if (n > 0) {
        idx += n;
        b = b % 8;
    }

    if (b > 0) {
        //attr[idx] = attr[idx] & (char)(pow(2, b) - 1);
        attr[idx] = attr[idx] & (char)(0xff >> (8 - b));
    }


    return idx + 1; 
}

/**
 * attr必须先初始化为0
 */
int
attr_string2binary(unsigned char *attrformat, char *attrstr, char *attr)
{
    int attrnum;
    unsigned int attrarray[HASHTABLE_ATTR_MAX_ITEM] = {0};

    attrnum = attr_string2array(attrstr, attrarray);
    if (attrnum <= 0)
        return 0;

    int ret = attr_array2binary(attrformat, attrarray, attrnum, attr); 

    return ret;
}

int 
attr_binary2string(unsigned char *attrformat, int attrnum, char *attr, int attrlen, char *attrstr)
{
    int n    = 2;
    int idx = 0;
    unsigned int val;
    int i;
    int widx = 0;
   
    for (i = 0; i < attrnum; i++) {
        int fs = attrformat[i];
        int offset = (fs + n) % 8;
        int yu = offset > 0 ? 1: 0;
        int cs = (fs + n) / 8 + yu;

        //DINFO("i:%d, fs:%d, cs:%d, offset:%d, n:%d, widx:%d\n", i, fs, cs, offset, n, widx);
        val = 0;
        if (cs <= 4) {
            //DINFO("copy:%d\n", cs);
            memcpy(&val, &attr[idx], cs);
            val <<= 32 - fs - n;
            val >>= 32 - fs;
        }else{
            //DINFO("copy:%d\n", sizeof(int));
            memcpy(&val, &attr[idx], sizeof(int));
            val >>= n;
            unsigned int val2 = 0;
            
            //DINFO("copy:%d\n", cs - sizeof(int));
            memcpy(&val2, &attr[idx + sizeof(int)], cs - sizeof(int));
            val2 <<= 32 - n;
            val = val | val2;
        }

        if (widx != 0) {
            attrstr[widx] = ':';    
            widx ++;
        }
        int len = int2string(&attrstr[widx], val);        
        widx += len;

        idx += cs - 1;
        if (yu == 0) {
            idx += 1;
        }
        n = offset;
    }
    attrstr[widx] = 0;

    return widx;
}

int 
attr_binary2array(unsigned char *attrformat, int attrnum, char *attr, unsigned int *attrarray)
{
    int n    = 2;
    int idx = 0;
    unsigned int val;
    int i;
    int widx = 0;
   
    for (i = 0; i < attrnum; i++) {
        int fs = attrformat[i];
        int offset = (fs + n) % 8;
        int yu = offset > 0 ? 1: 0;
        int cs = (fs + n) / 8 + yu;

        //DINFO("i:%d, fs:%d, cs:%d, offset:%d, n:%d, widx:%d\n", i, fs, cs, offset, n, widx);
        val = 0;
        if (cs <= 4) {
            //DINFO("copy:%d\n", cs);
            memcpy(&val, &attr[idx], cs);
            val <<= 32 - fs - n;
            val >>= 32 - fs;
        }else{
            //DINFO("copy:%d\n", sizeof(int));
            memcpy(&val, &attr[idx], sizeof(int));
            val >>= n;
            unsigned int val2 = 0;
            
            //DINFO("copy:%d\n", cs - sizeof(int));
            memcpy(&val2, &attr[idx + sizeof(int)], cs - sizeof(int));
            val2 <<= 32 - n;
            val = val | val2;
        }

        attrarray[widx] = val;
        widx ++;

        idx += cs - 1;
        if (yu == 0) {
            idx += 1;
        }
        n = offset;
    }
    attrarray[widx] = 0;

    return widx;
}


/**
 * attr必须先初始化为0
 */
int 
attr_array2flag(unsigned char *attrformat, unsigned int *attrarray, char attrnum, char *attr)
{
    int i, j;
    int idx = 0;
    int mf; // attrformat item value
    int m;  // attrarray item value
    int b = 0;
    int xlen;
    unsigned int v;

    attr[0] = 0x03;
    b = 2;
    v = 0;
    for (i = 0; i < attrnum; i++) {
        mf = attrformat[i];
        m  = attrarray[i];
        
        //DINFO("i:%d, b:%d, idx:%d, format:%d, array:%d, xlen:%d\n", i, b, idx, mf, m, (b + mf) / 8 + ((b + mf) % 8 > 0 ? 1:0));
        if (m == UINT_MAX) { // set to 1
            xlen = (b + mf) / 8 + ((b + mf) % 8 > 0 ? 1:0);
            v = 0xffffffff >> (32 - mf);
            v = v << b;

            char *cpdata = (char *)&v;
            if (xlen <= 4) {
                for (j = 0; j < xlen; j++) {
                    attr[idx + j] |= cpdata[j];
                }
            }else{
                for (j = 0; j < 4; j++) {
                    attr[idx + j] |= cpdata[j];
                }
                unsigned char v2 = 0xff >> (8 - (mf + b) - 32);
                attr[idx + j] |= v2;
            }
            //idx += xlen - 1;
            idx += (b + mf) / 8;
            b   = (b + mf) % 8;
        }else{ // set to 0
            b += mf;
            while (b >= 8) {
                idx ++;
                b = b - 8;
            }
        }
    }

    if (b > 0) {
        attr[idx] = attr[idx] & (char)(UCHAR_MAX >> (8 - b));
    }
    
    //DINFO("return:%d\n", idx+1);
    return idx + 1;
}



int 
cmd_dump_pack(char *data)
{
    return pack(data, 0, "$4c", CMD_DUMP);
}

int 
cmd_dump_unpack(char *data)
{
    return 0; 
}

int 
cmd_clean_pack(char *data, char *key)
{
    return pack(data, 0, "$4cs", CMD_CLEAN, key);
}

int 
cmd_clean_unpack(char *data, char *key)
{
    return unpack(data + CMD_REQ_HEAD_LEN, 0, "s", key);
}

int 
cmd_rmkey_pack(char *data, char *key)
{
    return pack(data, 0, "$4cs", CMD_RMKEY, key);
}

int 
cmd_rmkey_unpack(char *data, char *key)
{
    return unpack(data + CMD_REQ_HEAD_LEN, 0, "s", key);
}
int 
cmd_rmtable_pack(char *data, char *key)
{
    return pack(data, 0, "$4cs", CMD_RMTABLE, key);
}

int 
cmd_rmtable_unpack(char *data, char *key)
{
    return unpack(data + CMD_REQ_HEAD_LEN, 0, "s", key);
}

int 
cmd_count_pack(char *data, char *key, unsigned char attrnum, unsigned int *attrarray)
{
    return pack(data, 0, "$4csI", CMD_COUNT, key, attrnum, attrarray);
}

int 
cmd_count_unpack(char *data, char *key, unsigned char *attrnum, unsigned int *attrarray)
{
    return unpack(data + CMD_REQ_HEAD_LEN, 0, "sI", key, attrnum, attrarray);
}

int 
cmd_sortlist_count_pack(char *data, char *key, unsigned char attrnum, unsigned int *attrarray,
                        void *valmin, unsigned char vminlen, void *valmax, unsigned char vmaxlen)
{
    return pack(data, 0, "$4csICC", CMD_SL_COUNT, key, attrnum, attrarray, 
                vminlen, valmin, vmaxlen, valmax);
}

int 
cmd_sortlist_count_unpack(char *data, char *key, unsigned char *attrnum, unsigned int *attrarray,
                    void *valmin, unsigned char *vminlen, void *valmax, unsigned char *vmaxlen)
{
    return unpack(data + CMD_REQ_HEAD_LEN, 0, "sICC", key, attrnum, attrarray, 
            vminlen, valmin, vmaxlen, valmax);
}

int 
cmd_stat_pack(char *data, char *key)  //, HashTableStat  *stat)
{
    return pack(data, 0, "$4cs", CMD_STAT, key);
}

int 
cmd_stat_unpack(char *data, char *key)  //, HashTableStat *stat)
{
    return unpack(data + CMD_REQ_HEAD_LEN, 0, "s", key);
}

int 
cmd_stat_sys_pack(char *data)
{
    return pack(data, 0, "$4c", CMD_STAT_SYS);
}

int 
cmd_stat_sys_unpack(char *data)
{
    return 0; 
}


/**
 * Pack a CMD_CREATE command. Command format:
 * ------------------------------------------------------------------------
 * | command length (2 bytes) | cmd number (1 byte) | key length (1 byte) |
 * ------------------------------------------------------------------------
 * | key | value length (1 byte) | attr length (1 byte) | attr |
 * -------------------------------------------------------------
 * command length is the count of bytes following it. attr length is the length 
 * of attr integer array.
 * 
 * @param data destination byte array
 * @param key hash key
 * @param valuelen the data value length
 * @param attrnum the length of the attr array
 * @param attrarray attr array
 * @param return the length of the whole command.
 */
int 
cmd_create_table_pack(char *data, char *name, unsigned char valuelen, 
                unsigned char attrnum, unsigned int *attrarray,
                unsigned char listtype, unsigned char valuetype)
{
    return pack(data, 0, "$4cscIcc", CMD_CREATE_TABLE, name, valuelen, 
                attrnum, attrarray, listtype, valuetype);
}

int 
cmd_create_table_unpack(char *data, char *name, unsigned char *valuelen, 
                  unsigned char *attrnum, unsigned int *attrarray,
                  unsigned char *listtype, unsigned char *valuetype)
{
    return unpack(data + CMD_REQ_HEAD_LEN, 0, "scIcc", name, valuelen, 
                attrnum, attrarray, listtype, valuetype);
}

int 
cmd_create_node_pack(char *data, char *key)
{
    return pack(data, 0, "$4cs", CMD_CREATE_NODE, key);
}

int 
cmd_create_node_unpack(char *data, char *key)
{
    return unpack(data+CMD_REQ_HEAD_LEN, 0, "s", key);
}

int 
cmd_del_pack(char *data, char *key, char *value, unsigned char valuelen)
{
    return pack(data, 0, "$4csC", CMD_DEL, key, valuelen, value);
}

int 
cmd_del_unpack(char *data, char *key, char *value, unsigned char *valuelen)
{
    return unpack(data + CMD_REQ_HEAD_LEN, 0, "sC", key, valuelen, value);
}

int 
cmd_sortlist_del_pack(char *data, char *key, unsigned char kind, char *valmin, unsigned char vminlen, 
            char *valmax, unsigned char vmaxlen, unsigned char attrnum, unsigned int *attrarray)
{
    return pack(data, 0, "$4ccsCCI", CMD_SL_DEL, kind, key, vminlen, valmin,
                    vmaxlen, valmax, attrnum, attrarray);
}

int 
cmd_sortlist_del_unpack(char *data, char *key, unsigned char *kind, 
                        char *valmin, unsigned char *vminlen,
                        char *valmax, unsigned char *vmaxlen, 
                        unsigned char *attrnum, unsigned int *attrarray)
{
    return unpack(data+CMD_REQ_HEAD_LEN, 0, "csCCI", kind, key, vminlen, valmin, vmaxlen, valmax, 
                attrnum, attrarray);
}

int 
cmd_insert_pack(char *data, char *key, char *value, unsigned char valuelen, 
                unsigned char attrnum, unsigned int *attrarray, int pos)
{
    return pack(data, 0, "$4csCIi", CMD_INSERT, key, valuelen, value, attrnum, attrarray, pos);
}

int 
cmd_insert_unpack(char *data, char *key, char *value, unsigned char *valuelen,
                  unsigned char *attrnum, unsigned int *attrarray, int *pos)
{
    return unpack(data+CMD_REQ_HEAD_LEN, 0, "sCIi", key, valuelen, value, attrnum, attrarray, pos);
}


int 
cmd_move_pack(char *data, char *key, char *value, unsigned char valuelen, int pos)
{
    return pack(data, 0, "$4csCi", CMD_MOVE, key, valuelen, value, pos);
}

int 
cmd_move_unpack(char *data, char *key, char *value, unsigned char *valuelen, int *pos)
{
    return unpack(data+CMD_REQ_HEAD_LEN, 0, "sCi", key, valuelen, value, pos);
}

int 
cmd_attr_pack(char *data, char *key, char *value, unsigned char valuelen, 
              unsigned char attrnum, unsigned int *attrarray)
{
    return pack(data, 0, "$4csCI", CMD_ATTR, key, valuelen, value, attrnum, attrarray);
}

int 
cmd_attr_unpack(char *data, char *key, char *value, unsigned char *valuelen, 
                unsigned char *attrnum, unsigned int *attrarray)
{
    return unpack(data + CMD_REQ_HEAD_LEN, 0, "sCI", key, valuelen, value, attrnum, attrarray);
}

int 
cmd_tag_pack(char *data, char *key, char *value, unsigned char valuelen, unsigned char tag)
{
    return pack(data, 0, "$4csCc", CMD_TAG, key, valuelen, value, tag);
}

int 
cmd_tag_unpack(char *data, char *key, char *value, unsigned char *valuelen, unsigned char *tag)
{
    return unpack(data + CMD_REQ_HEAD_LEN, 0, "sCc", key, valuelen, value, tag);
}

int 
cmd_range_pack(char *data, char *key, unsigned char kind, 
               unsigned char attrnum, unsigned int *attrarray, 
               int frompos, int rlen)
{
    return pack(data, 0, "$4cscIii", CMD_RANGE, key, kind, attrnum, attrarray, frompos, rlen);
}

int 
cmd_range_unpack(char *data, char *key, unsigned char *kind, unsigned char *attrnum, unsigned int *attrarray, 
                 int *frompos, int *len)
{
    return unpack(data + CMD_REQ_HEAD_LEN, 0, "scIii", key, kind, attrnum, attrarray, frompos, len);
}

int 
cmd_sortlist_range_pack(char *data, char *key, unsigned char kind, 
                unsigned char attrnum, unsigned int *attrarray, 
                void *valmin, unsigned char vminlen, void *valmax, unsigned char vmaxlen)
{
    return pack(data, 0, "$4cscICC", CMD_SL_RANGE, key, kind, attrnum, attrarray,
                    vminlen, valmin, vmaxlen, valmax);
}

int 
cmd_sortlist_range_unpack(char *data, char *key, unsigned char *kind, 
                    unsigned char *attrnum, unsigned int *attrarray, 
                    void *valmin, unsigned char *vminlen, void *valmax, unsigned char *vmaxlen)
{
    return unpack(data + CMD_REQ_HEAD_LEN, 0, "scICC", key, kind, attrnum, attrarray,
                vminlen, valmin, vmaxlen, valmax);
}

int 
cmd_ping_pack(char *data)
{
    return pack(data, 0, "$4c", CMD_PING);
}
int 
cmd_ping_unpack(char *data)
{
    return 0;
}

int 
cmd_push_pack(char *data, unsigned char cmd, char *key, char *value, unsigned char valuelen, 
                unsigned char attrnum, unsigned *attrarray)
{
    return pack(data, 0, "$4csCI", cmd, key, valuelen, value, attrnum, attrarray);
}


int
cmd_push_unpack(char *data, char *key, char *value, unsigned char *valuelen,
            unsigned char *attrnum, unsigned int *attrarray)
{
    return unpack(data + CMD_REQ_HEAD_LEN, 0, "sCI", key, valuelen, value, attrnum, attrarray);
}

int 
cmd_lpush_pack(char *data, char *key, char *value, unsigned char valuelen, 
                unsigned char attrnum, unsigned *attrarray)
{
    return cmd_push_pack(data, CMD_LPUSH, key, value, valuelen, attrnum, attrarray);
}

int 
cmd_rpush_pack(char *data, char *key, char *value, unsigned char valuelen, 
                unsigned char attrnum, unsigned *attrarray)
{
    return cmd_push_pack(data, CMD_RPUSH, key, value, valuelen, attrnum, attrarray);
}


int
cmd_pop_pack(char *data, unsigned char cmd, char *key, int num)
{
    return pack(data, 0, "$4csi", cmd, key, num);
}

int
cmd_lpop_pack(char *data, char *key, int num)
{
    return cmd_pop_pack(data, CMD_LPOP, key, num);
}

int
cmd_rpop_pack(char *data, char *key, int num)
{
    return cmd_pop_pack(data, CMD_RPOP, key, num);
}

int 
cmd_pop_unpack(char *data, char *key, int *num)
{
    return unpack(data + CMD_REQ_HEAD_LEN, 0, "si", key, num);
}

int 
cmd_sync_pack(char *data, unsigned int logver, unsigned int logpos, int bcount, char *md5)
{
    return pack(data, 0, "$4ciiis", CMD_SYNC, logver, logpos, bcount, md5);
}

int 
cmd_sync_unpack(char *data, unsigned int *logver, unsigned int *logpos, int *bcount, char *md5)
{
    return unpack(data +CMD_REQ_HEAD_LEN, 0, "iiis", logver, logpos, bcount, md5);
}

int 
cmd_getdump_pack(char *data, unsigned int dumpver, uint64_t size)
{
    return pack(data, 0, "$4cil", CMD_GETDUMP, dumpver, size);
}

int 
cmd_getdump_unpack(char *data, unsigned int *dumpver, uint64_t *size)
{
    return unpack(data+CMD_REQ_HEAD_LEN, 0, "il", dumpver, size);
}

//add by lanwenhong
int
cmd_del_by_attr_pack(char *data, char *key, unsigned int *attrarray, unsigned char attrnum)
{
    return pack(data, 0, "$4csI", CMD_DEL_BY_ATTR, key, attrnum, attrarray);
}

int
cmd_del_by_attr_unpack(char *data, char *key, unsigned int *attrarray, unsigned char *attrnum)
{
    return unpack(data+CMD_REQ_HEAD_LEN, 0, "sI", key, attrnum, attrarray);
}

int
cmd_insert_mkv_pack(char *data, MemLinkInsertMkv *mkv)
{
    unsigned char cmd = CMD_INSERT_MKV;
    unsigned int len;
    int ret;
    int count = CMD_REQ_SIZE_LEN;
    MemLinkInsertKey *keyitem;
    MemLinkInsertVal *valitem;

    ret = pack(data + count, 0, "c", cmd);
    count += ret;

    keyitem = mkv->keylist;
    while(keyitem != NULL) {
        valitem = keyitem->vallist;

        count += pack(data + count, 0, "si", keyitem->key, keyitem->valnum);
        while (valitem != NULL) {
            count += pack(data + count, 0, "CIi", valitem->valuelen, valitem->value, 
                            valitem->attrnum, valitem->attrarray, valitem->pos);
            valitem = valitem->next;
        }
        keyitem = keyitem->next;
    }

    len = count - CMD_REQ_SIZE_LEN;
    pack(data, 0, "i", len);
    
    return count;
}

int
cmd_insert_mkv_unpack_packagelen(char *data, unsigned int *package_len)
{
    return unpack(data, 0, "i", package_len);
}

int
cmd_insert_mkv_unpack_keycount(char *data, unsigned int *keycount)
{
    return unpack(data, 0, "i", keycount);
}

int
cmd_insert_mkv_unpack_key(char *data, char *key, unsigned int *valcount, char **countstart)
{
    int ret = unpack(data, 0, "si", key, valcount);
    *countstart = data + ret;
    return ret;
}

int
cmd_insert_mkv_unpack_val(char *data, char *value, unsigned char *valuelen,
    unsigned char *attrnum, unsigned int *attrarray, int *pos)
{
    return unpack(data, 0, "CIi", valuelen, value, attrnum, attrarray, pos);
}

int
cmd_read_conn_info_pack(char *data)
{
    return pack(data, 0 ,"$4c", CMD_READ_CONN_INFO);
}
int
cmd_read_conn_info_unpack(char *data)
{
    return 0;
}


int
cmd_write_conn_info_pack(char *data)
{
    return pack(data, 0 ,"$4c", CMD_WRITE_CONN_INFO);
}
int
cmd_write_conn_info_unpack(char *data)
{
    return 0;
}

int
cmd_sync_conn_info_pack(char *data)
{
    return pack(data, 0 ,"$4c", CMD_SYNC_CONN_INFO);
}

int
cmd_sync_conn_info_unpack(char *data)
{
    return 0;
}

int 
pack_config_struct(char *data, MyConfig *config)
{
    return pack(data, 0, "I:4ififiiiiisisiiiiiicsii", 
                                    config->block_data_count_items, 
                                    config->block_data_count,
                                    config->block_data_count_items,
                                    config->block_data_reduce,
                                    config->dump_interval,
                                    config->block_clean_cond,
                                    config->block_clean_start,
                                    config->block_clean_num,
                                    config->read_port,
                                    config->write_port,
                                    config->sync_port,
                                    config->datadir,
                                    config->log_level, 
                                    config->log_name,
                                    config->write_binlog,
                                    config->timeout,
                                    config->thread_num,
                                    config->max_conn,
                                    config->max_core,
                                    config->is_daemon,
                                    config->role,
                                    config->master_sync_host,
                                    config->master_sync_port,
                                    config->sync_check_interval
                                    );
}

int 
unpack_config_struct(char *data, MyConfig *config)
{
    return unpack(data, 0, "I:4ififiiiiisisiiiiiicsii", 
                                    &config->block_data_count_items, 
                                    config->block_data_count,
                                    &config->block_data_count_items,
                                    &config->block_data_reduce,
                                    &config->dump_interval,
                                    &config->block_clean_cond,
                                    &config->block_clean_start,
                                    &config->block_clean_num,
                                    &config->read_port,
                                    &config->write_port,
                                    &config->sync_port,
                                    &config->datadir,
                                    &config->log_level, 
                                    config->log_name,
                                    &config->write_binlog,
                                    &config->timeout,
                                    &config->thread_num,
                                    &config->max_conn,
                                    &config->max_core,
                                    &config->is_daemon,
                                    &config->role,
                                    config->master_sync_host,
                                    &config->master_sync_port,
                                    &config->sync_check_interval
                                    );

}

int
cmd_config_info_pack(char *data)
{
    return pack(data, 0, "$4c", CMD_CONFIG_INFO);
}

int
cmd_config_info_unpack(char *data)
{
    return 0;
}

int
cmd_set_config_dynamic_pack(char *data, char *key, char *value)
{
    return pack(data, 0, "$4css", CMD_SET_CONFIG, key, value);
}

int
cmd_set_config_dynamic_unpack(char *data, char *key, char *value)
{
    return unpack(data + CMD_REQ_HEAD_LEN, 0, "ss", key, value);
}

int
cmd_clean_all_pack(char *data)
{
    return pack(data, 0, "$4c", CMD_CLEAN_ALL);
}

int
cmd_clean_all_unpack(char *data)
{
    return 0;
}

int
cmd_heartbeat_pack(char *data, int port)
{
    /*
    unsigned char cmd = CMD_HEARTBEAT;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    memcpy(data + count, &port, sizeof(int));
    count += sizeof(int);
    int len = count - CMD_REQ_SIZE_LEN;

    memcpy(data,&len, sizeof(int));
    return count;
    */
    return pack(data, 0, "$4ci", CMD_HEARTBEAT, port);
}
int
cmd_heartbeat_unpack(char *data, int *port)
{
    /*
    char *ptr = data + CMD_REQ_SIZE_LEN + sizeof(char);

    memcpy(port, ptr, sizeof(int));

    return 0;
    */
    return unpack(data + CMD_REQ_HEAD_LEN, 0, "i", port);
}

int
cmd_backup_ack_pack(char *data, char state)
{
    return pack(data, 0, "$4cc", CMD_BACKUP_ACK, state);
}

int
cmd_backup_ack_unpack(char *data, unsigned char *cmd, short *ret, int *logver, int *logline)
{
    return unpack(data + CMD_REQ_SIZE_LEN, 0, "chii", cmd, ret, logver, logline);
}

int
cmd_vote_pack(char *data, uint64_t id, unsigned char result, uint64_t voteid, unsigned short port)
{
    return pack(data, 0, "$4clclh", CMD_VOTE, id, result, voteid, port);
}

int
unpack_votehost(char *buf, char *ip, uint16_t *port)
{
    return unpack(buf, 0, "sh", ip, port);
}

int
unpack_voteid(char *buf, uint64_t *vote_id)
{
    return unpack(buf, 0, "l", vote_id);
}

/**
 * @}
 */
