#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "logfile.h"
#include "serial.h"
#include "utils.h"

/**
 * 把字符串形式的mask转换为数组形式
 */
int 
mask_string2array(char *mask, unsigned int *result)
{
    char *m = mask;
    int  i = 0;

    while (m != NULL) {
        char *fi  = strchr(m, ':');
        if (m[0] == ':') {
            result[i] = UINT_MAX;
        }else{
            result[i] = atoi(m); 
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

int
mask_array2binary(unsigned char *maskformat, unsigned int *maskarray, char masknum, char *mask)
{
    int i;
    int b   = 0; // 已经处理的bit
    int idx = 0;
    int n;
    unsigned int v;
    char    mf;
    // 前两位分别表示真实删除和标记删除，跳过
    // mask[idx] = mask[idx] & 0xfc;
    b += 2;

    for (i = 0; i < masknum; i++) {
        mf = maskformat[i];
        if (mf == 0) {
            DERROR("mask format error: %d\n", mf);
            return -1;
        }
        v  = maskarray[i]; 
        DINFO("idx:%d, b:%d, format:%d, maskvalue:%d, %x\n", idx, b, mf, v, v);

        if (v == UINT_MAX) {
            b += maskformat[i];
            continue;
        }
        
        unsigned char y = (b + mf) % 8;
        n = (b + mf) / 8 + (y>0 ? 1: 0);
        DINFO("y: %d, n: %d\n", y, n);
        v = v << b;
        //DINFO("v: %x %b\n", v, v); 
        unsigned char m = pow(2, b) - 1;
        unsigned char x = mask[idx] & m;
        DINFO("m: %d, %x, x: %d, %x\n", m, m, x, x);

        v = v | x;

        m = (pow(2, 8 - y) - 1);
        m = m << y;
        x = mask[idx + n - 1] & m;

        DINFO("copy idx:%d, v:%d, n:%d\n", idx, v, n);
        //printb((char *)&v, n);
        memcpy(&mask[idx], &v, n);

        idx += n - 1;
        
        mask[idx] = mask[idx] | x;

        b = y;

        DINFO("============================\n");
    }

    return idx + 1; 
}

int
mask_string2binary(unsigned char *maskformat, char *maskstr, char *mask)
{
    int masknum;
    unsigned int maskarray[HASHTABLE_MASK_MAX_LEN];

    masknum = mask_string2array(maskstr, maskarray);
    if (masknum <= 0)
        return 0;

    int ret = mask_array2binary(maskformat, maskarray, masknum, mask); 

    return ret;
}

static int 
unpack_string(char *s, char *v, unsigned char *vlen)
{
    unsigned char len;

    memcpy(&len, s, sizeof(char));
    memcpy(v, s + sizeof(char), len);

    if (vlen) {
        *vlen = len;
    }

    return len + sizeof(char);
}

static int
pack_string(char *s, char *v, unsigned char vlen)
{
    if (vlen <= 0) {
        vlen = strlen(v);
    }
    memcpy(s, &vlen, sizeof(char));
    memcpy(s + sizeof(char), v, vlen);

    return vlen + sizeof(char);
}

static int
unpack_mask(char *s, unsigned int *v, unsigned char *vlen)
{
    unsigned char len;
    
    memcpy(&len, s, sizeof(char));
    int clen = len * sizeof(int);
    //int i;

    printh(s, clen + sizeof(char));

    memcpy(v, s + sizeof(char), clen);

    if (vlen) {
        *vlen = len;
    }
    return clen + sizeof(char);
}

static int
pack_mask(char *s, unsigned int *v, unsigned char vlen)
{
    int clen = vlen * sizeof(int);

    if (vlen <= 0) {
        DERROR("vlen must not <= 0\n");
        MEMLINK_EXIT;
        return 0;
    }

    memcpy(s, &vlen, sizeof(char));
    memcpy(s + sizeof(char), v, clen);

    return clen + sizeof(char);
}

int 
cmd_dump_pack(char *data)
{
    unsigned short len = sizeof(char);
    unsigned char  cmd = CMD_DUMP;

    memcpy(data, &len, sizeof(short));
    memcpy(data + sizeof(short), &cmd, sizeof(char));

    return len + sizeof(short);
}

int 
cmd_dump_unpack(char *data)
{
    unsigned short len;
    //unsigned char cmd;

    memcpy(&len, data, sizeof(short));
    //cmd = *(data + sizeof(short)); 
    return 0; 
}

int 
cmd_clean_pack(char *data, char *key)
{
    unsigned char  cmd = CMD_CLEAN;
    unsigned short  len;
    int count = sizeof(short);
    int ret;

    memcpy(data + count, &cmd, sizeof(char)); 
    count += sizeof(char);

    ret = pack_string(data + count, key, 0);
    count += ret;
    len = count - sizeof(short);
    DINFO("clean len: %d, count: %d\n", len, count);
    memcpy(data, &len, sizeof(short));
    
    return count;
}

int 
cmd_clean_unpack(char *data, char *key)
{
    unsigned char keylen;

    unpack_string(data, key, &keylen);

    return 0;
}

int 
cmd_stat_pack(char *data, char *key)  //, HashTableStat  *stat)
{
    unsigned char  cmd = CMD_STAT;
    unsigned short count = sizeof(short);
    unsigned short len;

    memcpy(data + count, &cmd, sizeof(char)); 
    count += sizeof(char);

    count += pack_string(data + count, key, 0);
    //memcpy(data + count, stat, sizeof(HashTableStat)); 
    //count += sizeof(HashTableStat);

    len = count - sizeof(short);
    memcpy(data, &len, sizeof(short));
    
    return count;
}

int 
cmd_stat_unpack(char *data, char *key)  //, HashTableStat *stat)
{
    unsigned char keylen;
    int count;

    count = unpack_string(data, key, &keylen);
    //memcpy(stat, data + count, sizeof(HashTableStat)); 

    return 0;
}

int 
cmd_create_pack(char *data, char *key, unsigned char valuelen, unsigned char masknum, unsigned int *maskarray)
{
    unsigned char  cmd = CMD_CREATE;
    unsigned short count = sizeof(short);
    unsigned short len;

    memcpy(data + count, &cmd, sizeof(char)); 
    count += sizeof(char);
    count += pack_string(data + count, key, 0);
    memcpy(data + count, &valuelen, sizeof(char));
    count += sizeof(char);

    //count += pack_string(data + count, maskarray, masknum);
    count += pack_mask(data + count, maskarray, masknum);
    len = count - sizeof(short);

    memcpy(data, &len, sizeof(short));
    
    return count;
}

int 
cmd_create_unpack(char *data, char *key, unsigned char *valuelen, unsigned char *masknum, unsigned int *maskarray)
{
    int count = sizeof(short) + sizeof(char);
    unsigned char len;

    count += unpack_string(data + count, key, &len);
    memcpy(valuelen, data + count, sizeof(char));
    count += sizeof(char);
    //unpack_string(data + count, maskarray, masknum);
    unpack_mask(data + count, maskarray, masknum);

    return 0;
}

int 
cmd_del_pack(char *data, char *key, char *value, unsigned char valuelen)
{
    unsigned char cmd = CMD_DEL;
    unsigned short len;
    int count = sizeof(short);

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);

    count += pack_string(data + count, key, 0);
    count += pack_string(data + count, value, valuelen);

    len = count - sizeof(short);
    memcpy(data, &len, sizeof(short));

    return count;
}

int 
cmd_del_unpack(char *data, char *key, char *value, unsigned char *valuelen)
{
    int count = sizeof(short) + sizeof(char);
    unsigned char len;
    
    count += unpack_string(data + count, key, &len);
    unpack_string(data + count, value, valuelen);
    
    return 0;
}

int 
cmd_insert_pack(char *data, char *key, char *value, unsigned char valuelen, 
                unsigned char masknum, unsigned int *maskarray, unsigned int pos)
{
    unsigned char cmd = CMD_INSERT;
    unsigned short len;
    int count = sizeof(short);

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    count += pack_string(data + count, key, 0);  
    count += pack_string(data + count, value, valuelen);
    //count += pack_string(data + count, maskformat, masknum);
    count += pack_mask(data + count, maskarray, masknum);
    memcpy(data + count, &pos, sizeof(int));

    len = count - sizeof(short);
    memcpy(data, &len, sizeof(short));
    
    return count;
}

int 
cmd_insert_unpack(char *data, char *key, char *value, unsigned char *valuelen,
                  unsigned char *masknum, unsigned int *maskarray, unsigned int *pos)
{
    int count = sizeof(short) + sizeof(char);

    unsigned char vlen;
    count += unpack_string(data + count, key, NULL);
    count += unpack_string(data + count, value, &vlen);
    //count += unpack_string(data + count, maskformat, masknum);
    count += unpack_mask(data + count, maskarray, masknum);
    memcpy(pos, data + count, sizeof(int));

    *valuelen = vlen;

    return 0;
}


int 
cmd_update_pack(char *data, char *key, char *value, unsigned char valuelen, unsigned int pos)
{
    unsigned char cmd = CMD_UPDATE;
    unsigned short len;
    int count = sizeof(short);

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    count += pack_string(data + count, key, 0);
    count += pack_string(data + count, value, valuelen);
    memcpy(data + count, &pos, sizeof(int));
    count += sizeof(int);

    len = count - 1;
    memcpy(data, &len, sizeof(short)); 

    return count;
}

int 
cmd_update_unpack(char *data, char *key, char *value, unsigned char *valuelen, unsigned int *pos)
{
    int count = sizeof(short) + sizeof(char);
    unsigned char vlen;

    count += unpack_string(data + count, key, NULL);
    count += unpack_string(data + count, value, &vlen);
    memcpy(pos, data + count, sizeof(int));

    *valuelen = vlen;
    return 0;
}

int 
cmd_mask_pack(char *data, char *key, char *value, unsigned char valuelen, 
              unsigned char masknum, unsigned int *maskarray)
{
    unsigned char cmd = CMD_MASK;
    unsigned short len;
    int count = sizeof(short);

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    count += pack_string(data + count, key, 0);
    count += pack_string(data + count, value, valuelen);
    //count += pack_string(data + count, maskformat, masknum);
    count += pack_mask(data + count, maskarray, masknum);

    len = count - sizeof(short);
    
    memcpy(data + count, &len, sizeof(short));
    return count;
}

int 
cmd_mask_unpack(char *data, char *key, char *value, unsigned char *valuelen, 
                unsigned char *masknum, unsigned int *maskarray)
{
    int count = sizeof(short) + sizeof(char);
    unsigned char vlen; 

    count += unpack_string(data + count, key, NULL);
    count += unpack_string(data + count, value, &vlen);
    //count += unpack_string(data + count, maskformat, &mlen);
    count += unpack_mask(data + count, maskarray, masknum);

    *valuelen = vlen;
    //*masknum  = mlen;
    return 0;
}

int 
cmd_tag_pack(char *data, char *key, char *value, unsigned char valuelen, unsigned char tag)
{
    unsigned char cmd = CMD_TAG;
    unsigned short len;
    int count = sizeof(short);

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    count += pack_string(data + count, key, 0);
    count += pack_string(data + count, value, valuelen);
    memcpy(data + count, &tag, sizeof(char));
    count += sizeof(char);

    len = count - sizeof(short);
    memcpy(data, &len, sizeof(short));

    return count;
}

int 
cmd_tag_unpack(char *data, char *key, char *value, unsigned char *valuelen, unsigned char *tag)
{
    int count = sizeof(short) + sizeof(char);
    unsigned char vlen;
    count += unpack_string(data + count, key, NULL);
    count += unpack_string(data + count, value, &vlen);
    memcpy(tag, data + count, sizeof(char));
    *valuelen = vlen;
    return 0;
}

int 
cmd_range_pack(char *data, char *key, unsigned char masknum, unsigned int *maskarray, 
               unsigned int frompos, unsigned int rlen)
{
    unsigned char cmd = CMD_RANGE;
    unsigned short len;
    int count = sizeof(short);

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    count += pack_string(data + count, key, 0);
    //count += pack_string(data + count, maskformat, masknum);
    count += pack_mask(data + count, maskarray, masknum);
    memcpy(data + count, &frompos, sizeof(int));
    count += sizeof(int);
    memcpy(data + count, &rlen, sizeof(int));
    count += sizeof(int);

    len = count - sizeof(short);
    memcpy(data, &len, sizeof(short));
    return count;
}

int 
cmd_range_unpack(char *data, char *key, unsigned char *masknum, unsigned int *maskarray, 
                 unsigned int *frompos, unsigned int *len)
{
    int count = sizeof(short) + sizeof(char);
    //unsigned char mlen;
    count += unpack_string(data + count, key, NULL); 
    //count += unpack_string(data + count, maskformat, &mlen);
    count += unpack_mask(data + count, maskarray, masknum);
    memcpy(frompos, data + count, sizeof(int));
    count += sizeof(int);
    memcpy(len, data + count, sizeof(int));
    //*masknum = mlen;
    return 0;
}




