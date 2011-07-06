/**
 * 数据包编码解码
 * @file serial.c
 * @author zhaowei
 * @ingroup memlink
 * @{
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "logfile.h"
#include "serial.h"
#include "utils.h"
#include "zzmalloc.h"
#include "myconfig.h"
/**
 * 把字符串形式的mask转换为数组形式
 */
/**
 * Convert a mask string to an integer array. Digit characters delimited by : is 
 * converted into the correspoding integer. : is converted into UINT_MAX.
 *
 * @param maskstr mask string
 * @param result converted integer array
 * @return the count of the converted integers
 */
int
mask_string2array(char *maskstr, unsigned int *result)
{
    char *m = maskstr;
    int  i = 0;

    if (m[0] == 0) {
        return 0;
    }

    while (m != NULL) {
        char *fi  = strchr(m, ':');
        if (i >= HASHTABLE_MASK_MAX_ITEM)
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
 * mask必须先初始化为0
 */
int
mask_array2binary(unsigned char *maskformat, unsigned int *maskarray, char masknum, char *mask)
{
    int i;
    int b   = 0; // 已经处理的bit
    int idx = 0;
    int n;
    unsigned int v, flow = 0;
    char    mf;
    // 前两位分别表示真实删除和标记删除，跳过
    // mask[idx] = mask[idx] & 0xfc;
    //memset(mask, 0, masknum);
    
    mask[0] = 0x01;  // 默认设置数据有效，非标记删除
    b += 2;

    for (i = 0; i < masknum; i++) {
        mf = maskformat[i];
        if (mf == 0) {
            DERROR("mask format error: %d\n", mf);
            return -1;
        }
        v  = maskarray[i]; 
        //DINFO("idx:%d, b:%d, format:%d, maskvalue:%d, %x\n", idx, b, mf, v, v);

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
        unsigned char x = mask[idx] & m;

        v = v | x;
		if (n > 4) {
        	memcpy(&mask[idx], &v, sizeof(int));
		} else {
			memcpy(&mask[idx], &v, n);
		}

        if (y > 0) {
            idx += n - 1;
        }else{
            idx += n;
        }
		if (n > 4) {
			mask[idx] = mask[idx] | flow;
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
        //mask[idx] = mask[idx] & (char)(pow(2, b) - 1);
        mask[idx] = mask[idx] & (char)(0xff >> (8 - b));
    }


    return idx + 1; 
}

/**
 * mask必须先初始化为0
 */
int
mask_string2binary(unsigned char *maskformat, char *maskstr, char *mask)
{
    int masknum;
    unsigned int maskarray[HASHTABLE_MASK_MAX_ITEM] = {0};

    masknum = mask_string2array(maskstr, maskarray);
    if (masknum <= 0)
        return 0;

    int ret = mask_array2binary(maskformat, maskarray, masknum, mask); 

    return ret;
}

static int 
int2string(char *s, unsigned int val)
{
    unsigned int v = val;
    int yu;
    int ret, i = 0, j = 0;
    char ss[32];

	if (v == 0) {
		s[0] = '0';
		return 1;
	}

	if (v == UINT_MAX)
		return 0;

    while (v > 0) {
        yu = v % 10;
        v  = v / 10;

        ss[i] = yu + 48;
        i++;
    }
    ret = i;

    for (i = i - 1; i >= 0; i--) {
        s[j] = ss[i];
        j++;
    }

    return ret;
}

int 
mask_binary2string(unsigned char *maskformat, int masknum, char *mask, int masklen, char *maskstr)
{
	int n	= 2;
	int idx = 0;
	unsigned int val;
	int i;
	int widx = 0;
   
	for (i = 0; i < masknum; i++) {
		int fs = maskformat[i];
		int offset = (fs + n) % 8;
		int yu = offset > 0 ? 1: 0;
		int cs = (fs + n) / 8 + yu;

        //DINFO("i:%d, fs:%d, cs:%d, offset:%d, n:%d, widx:%d\n", i, fs, cs, offset, n, widx);
		val = 0;
        if (cs <= 4) {
            //DINFO("copy:%d\n", cs);
            memcpy(&val, &mask[idx], cs);
            val <<= 32 - fs - n;
            val >>= 32 - fs;
        }else{
            //DINFO("copy:%d\n", sizeof(int));
            memcpy(&val, &mask[idx], sizeof(int));
            val >>= n;
            unsigned int val2 = 0;
            
            //DINFO("copy:%d\n", cs - sizeof(int));
            memcpy(&val2, &mask[idx + sizeof(int)], cs - sizeof(int));
            val2 <<= 32 - n;
            val = val | val2;
        }

		if (widx != 0) {
			maskstr[widx] = ':';	
			widx ++;
		}
		int len = int2string(&maskstr[widx], val);		
		widx += len;

		idx += cs - 1;
		if (yu == 0) {
			idx += 1;
		}
	    n = offset;
	}
    maskstr[widx] = 0;

	return widx;
}

int 
mask_array2flag(unsigned char *maskformat, unsigned int *maskarray, char masknum, char *mask)
{
    int i, j;
    int idx = 0;
    int mf; // maskformat item value
    int m;  // maskarray item value
    int b = 0;
    int xlen;
    unsigned int v;

    mask[0] = 0x03;
    b = 2;
    v = 0;
    for (i = 0; i < masknum; i++) {
        mf = maskformat[i];
        m  = maskarray[i];
        
        //DINFO("i:%d, b:%d, idx:%d, format:%d, array:%d, xlen:%d\n", i, b, idx, mf, m, (b + mf) / 8 + ((b + mf) % 8 > 0 ? 1:0));
        if (m == UINT_MAX) { // set to 1
            xlen = (b + mf) / 8 + ((b + mf) % 8 > 0 ? 1:0);
            v = 0xffffffff >> (32 - mf);
            v = v << b;

            char *cpdata = (char *)&v;
			if (xlen <= 4) {
				for (j = 0; j < xlen; j++) {
					mask[idx + j] |= cpdata[j];
				}
			}else{
				for (j = 0; j < 4; j++) {
					mask[idx + j] |= cpdata[j];
				}
				unsigned char v2 = 0xff >> (8 - (mf + b) - 32);
				mask[idx + j] |= v2;
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
        mask[idx] = mask[idx] & (char)(UCHAR_MAX >> (8 - b));
    }
    
    DINFO("return:%d\n", idx+1);
    return idx + 1;
}


static int 
unpack_string(char *s, char *v, unsigned char *vlen)
{
    unsigned char len;

    memcpy(&len, s, sizeof(char));
    if (len > 0) {
        memcpy(v, s + sizeof(char), len);
    }
    v[len] = 0;

    if (vlen) {
        *vlen = len;
    }
    return len + sizeof(char);
}

/**
 * Pack a string into a byte array. First, a byte for the count of the bytes to 
 * be put is put into the destination array.  Then, the vlen bytes from the 
 * source string are put into the destination array.
 * 
 * @param s destination byte array
 * @param v source string
 * @param vlen length to be put into the destination array. If zero, the whole 
 * string is put.
 * @return the count of bytes which have been put into the destination array
 */
static int
pack_string(char *s, char *v, unsigned char vlen)
{
    if (vlen <= 0) {
        vlen = strlen(v);
    }
    memcpy(s, &vlen, sizeof(char));
    if (vlen > 0) {
        memcpy(s + sizeof(char), v, vlen);
    }

    return vlen + sizeof(char);
}

static int
unpack_mask(char *s, unsigned int *v, unsigned char *vlen)
{
    unsigned char len;
    
    memcpy(&len, s, sizeof(char));
    int clen = len * sizeof(int);

    //printh(s, clen + sizeof(char));
    
    if (len > 0) {
        memcpy(v, s + sizeof(char), clen);
    }

    if (vlen) {
        *vlen = len;
    }
    return clen + sizeof(char);
}

static int
pack_mask(char *s, unsigned int *v, unsigned char vlen)
{
    int clen = vlen * sizeof(int);

    /*
    if (vlen < 0) {
        DERROR("vlen must not <= 0\n");
        MEMLINK_EXIT;
        return 0;
    }*/

    memcpy(s, &vlen, sizeof(char));
    if (vlen) {
        memcpy(s + sizeof(char), v, clen);
    }

    return clen + sizeof(char);
}

int 
cmd_dump_pack(char *data)
{
    unsigned int len = sizeof(char);
    unsigned char  cmd = CMD_DUMP;

    memcpy(data, &len, CMD_REQ_SIZE_LEN);
    memcpy(data + CMD_REQ_SIZE_LEN, &cmd, sizeof(char));

    return len + CMD_REQ_SIZE_LEN;
}

int 
cmd_dump_unpack(char *data)
{
    return 0; 
}

int 
cmd_clean_pack(char *data, char *key)
{
    unsigned char  cmd = CMD_CLEAN;
    unsigned int  len;
    int count = CMD_REQ_SIZE_LEN;
    int ret;

    memcpy(data + count, &cmd, sizeof(char)); 
    count += sizeof(char);

    ret = pack_string(data + count, key, 0);
    count += ret;
    len = count - sizeof(int);
    //DINFO("clean len: %d, count: %d\n", len, count);
    memcpy(data, &len, sizeof(int));
    
    return count;
}

int 
cmd_clean_unpack(char *data, char *key)
{
    unsigned char keylen;
    int count = CMD_REQ_HEAD_LEN;

    unpack_string(data + count, key, &keylen);

    return 0;
}

int 
cmd_rmkey_pack(char *data, char *key)
{
    unsigned char  cmd = CMD_RMKEY;
    unsigned int  len;
    int count = CMD_REQ_SIZE_LEN;
    int ret;

    memcpy(data + count, &cmd, sizeof(char)); 
    count += sizeof(char);

    ret = pack_string(data + count, key, 0);
    count += ret;
    len = count - CMD_REQ_SIZE_LEN;
    //DINFO("clean len: %d, count: %d\n", len, count);
    memcpy(data, &len, CMD_REQ_SIZE_LEN);
    
    return count;
}

int 
cmd_rmkey_unpack(char *data, char *key)
{
    unsigned char keylen;
    int count = CMD_REQ_HEAD_LEN;

    unpack_string(data + count, key, &keylen);

    return 0;
}


int 
cmd_count_pack(char *data, char *key, unsigned char masknum, unsigned int *maskarray)
{
    unsigned char  cmd = CMD_COUNT;
    unsigned int  len;
    int count = CMD_REQ_SIZE_LEN;
    int ret;

    memcpy(data + count, &cmd, sizeof(char)); 
    count += sizeof(char);

    ret = pack_string(data + count, key, 0);
    count += ret;
	count += pack_mask(data + count, maskarray, masknum);

    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);
    
    return count;
}

int 
cmd_count_unpack(char *data, char *key, unsigned char *masknum, unsigned int *maskarray)
{
    unsigned char keylen;
    int count = CMD_REQ_HEAD_LEN;

    count += unpack_string(data + count, key, &keylen);
	unpack_mask(data + count, maskarray, masknum);

    return 0;
}

int 
cmd_sortlist_count_pack(char *data, char *key, unsigned char masknum, unsigned int *maskarray,
                        void *valmin, unsigned char vminlen, void *valmax, unsigned char vmaxlen)
{
    unsigned char  cmd = CMD_SL_COUNT;
    unsigned int  len;
    int count = CMD_REQ_SIZE_LEN;
    int ret;

    memcpy(data + count, &cmd, sizeof(char)); 
    count += sizeof(char);

    ret = pack_string(data + count, key, 0);
    count += ret;
	count += pack_mask(data + count, maskarray, masknum);
    count += pack_string(data + count, valmin, vminlen);
    count += pack_string(data + count, valmax, vmaxlen);

    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);
    
    return count;
}

int 
cmd_sortlist_count_unpack(char *data, char *key, unsigned char *masknum, unsigned int *maskarray,
                    void *valmin, unsigned char *vminlen, void *valmax, unsigned char *vmaxlen)
{
    unsigned char keylen;
    int count = CMD_REQ_HEAD_LEN;

    count += unpack_string(data + count, key, &keylen);
	count += unpack_mask(data + count, maskarray, masknum);
    count += unpack_string(data + count, valmin, vminlen);
    unpack_string(data + count, valmax, vmaxlen);

    return 0;
}

int 
cmd_stat_pack(char *data, char *key)  //, HashTableStat  *stat)
{
    unsigned char  cmd = CMD_STAT;
    unsigned short count = CMD_REQ_SIZE_LEN;
    unsigned int len;

    memcpy(data + count, &cmd, sizeof(char)); 
    count += sizeof(char);

    count += pack_string(data + count, key, 0);
    //memcpy(data + count, stat, sizeof(HashTableStat)); 
    //count += sizeof(HashTableStat);

    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);
    
    return count;
}

int 
cmd_stat_unpack(char *data, char *key)  //, HashTableStat *stat)
{
    unsigned char keylen;
    int count = CMD_REQ_HEAD_LEN;

    count = unpack_string(data + count, key, &keylen);
    //memcpy(stat, data + count, sizeof(HashTableStat)); 

    return 0;
}

int 
cmd_stat_sys_pack(char *data)
{
    unsigned int len = sizeof(char);
    unsigned char  cmd = CMD_STAT_SYS;

    memcpy(data, &len, CMD_REQ_SIZE_LEN);
    memcpy(data + CMD_REQ_SIZE_LEN, &cmd, sizeof(char));

    return len + CMD_REQ_SIZE_LEN;
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
 * | key | value length (1 byte) | mask length (1 byte) | mask |
 * -------------------------------------------------------------
 * command length is the count of bytes following it. mask length is the length 
 * of mask integer array.
 * 
 * @param data destination byte array
 * @param key hash key
 * @param valuelen the data value length
 * @param masknum the length of the mask array
 * @param maskarray mask array
 * @param return the length of the whole command.
 */
int 
cmd_create_pack(char *data, char *key, unsigned char valuelen, 
                unsigned char masknum, unsigned int *maskarray,
                unsigned char listtype, unsigned char valuetype)
{
    unsigned char  cmd = CMD_CREATE;
    unsigned short count = CMD_REQ_SIZE_LEN;
    unsigned int len;

    memcpy(data + count, &cmd, sizeof(char)); 
    count += sizeof(char);
    count += pack_string(data + count, key, 0);
    memcpy(data + count, &valuelen, sizeof(char));
    count += sizeof(char);

    //count += pack_string(data + count, maskarray, masknum);
    count += pack_mask(data + count, maskarray, masknum);
    memcpy(data + count, &listtype, sizeof(char));
    count += sizeof(char);
    memcpy(data + count, &valuetype, sizeof(char));
    count += sizeof(char);
    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);
    
    return count;
}

int 
cmd_create_unpack(char *data, char *key, unsigned char *valuelen, 
                  unsigned char *masknum, unsigned int *maskarray,
                  unsigned char *listtype, unsigned char *valuetype)
{
    int count = CMD_REQ_HEAD_LEN;
    unsigned char len;

    count += unpack_string(data + count, key, &len);
    memcpy(valuelen, data + count, sizeof(char));
    count += sizeof(char);
    count += unpack_mask(data + count, maskarray, masknum);
    memcpy(listtype, data + count, sizeof(char));
    count += sizeof(char);
    memcpy(valuetype, data + count, sizeof(char));

    return 0;
}

int 
cmd_del_pack(char *data, char *key, char *value, unsigned char valuelen)
{
    unsigned char cmd = CMD_DEL;
    unsigned int len;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);

    count += pack_string(data + count, key, 0);
    count += pack_string(data + count, value, valuelen);

    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);

    return count;
}

int 
cmd_del_unpack(char *data, char *key, char *value, unsigned char *valuelen)
{
    int count = CMD_REQ_HEAD_LEN;
    unsigned char len;
    
    count += unpack_string(data + count, key, &len);
    unpack_string(data + count, value, valuelen);
    
    return 0;
}

int 
cmd_sortlist_del_pack(char *data, char *key, unsigned char kind, char *valmin, unsigned char vminlen, 
            char *valmax, unsigned char vmaxlen, unsigned char masknum, unsigned int *maskarray)
{
    unsigned char cmd = CMD_SL_DEL;
    unsigned int len;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    
    memcpy(data + count, &kind, sizeof(char));
    count += sizeof(char);
    
    count += pack_string(data + count, key, 0);
    count += pack_string(data + count, valmin, vminlen);
    count += pack_string(data + count, valmax, vmaxlen);
    count += pack_mask(data + count, maskarray, masknum);

    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);

    return count;
}

int 
cmd_sortlist_del_unpack(char *data, char *key, unsigned char *kind, char *valmin, unsigned char *vminlen,
                        char *valmax, unsigned char *vmaxlen, unsigned char *masknum, unsigned int *maskarray)
{
    int count = CMD_REQ_HEAD_LEN;
    unsigned char len;
   
    memcpy(kind, data + count, sizeof(char));
    count += sizeof(char);
    count += unpack_string(data + count, key, &len);
    count += unpack_string(data + count, valmin, vminlen);
    count += unpack_string(data + count, valmax, vmaxlen);
    unpack_mask(data + count, maskarray, masknum);
    
    return 0;
}

int 
cmd_insert_pack(char *data, char *key, char *value, unsigned char valuelen, 
                unsigned char masknum, unsigned int *maskarray, int pos)
{
    unsigned char cmd = CMD_INSERT;
    unsigned int len;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    count += pack_string(data + count, key, 0);  
    count += pack_string(data + count, value, valuelen);
    //count += pack_string(data + count, maskformat, masknum);
    count += pack_mask(data + count, maskarray, masknum);
    memcpy(data + count, &pos, sizeof(int));
    count += sizeof(int);

    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);
    
    return count;
}

int 
cmd_insert_unpack(char *data, char *key, char *value, unsigned char *valuelen,
                  unsigned char *masknum, unsigned int *maskarray, int *pos)
{
    int count = CMD_REQ_HEAD_LEN;

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
cmd_move_pack(char *data, char *key, char *value, unsigned char valuelen, int pos)
{
    unsigned char cmd = CMD_MOVE;
    unsigned int len;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    count += pack_string(data + count, key, 0);
    count += pack_string(data + count, value, valuelen);
    memcpy(data + count, &pos, sizeof(int));
    count += sizeof(int);

    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN); 

    return count;
}

int 
cmd_move_unpack(char *data, char *key, char *value, unsigned char *valuelen, int *pos)
{
    int count = CMD_REQ_HEAD_LEN;
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
    unsigned int len;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    count += pack_string(data + count, key, 0);
    count += pack_string(data + count, value, valuelen);
    //count += pack_string(data + count, maskformat, masknum);
    count += pack_mask(data + count, maskarray, masknum);

    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);

    return count;
}

int 
cmd_mask_unpack(char *data, char *key, char *value, unsigned char *valuelen, 
                unsigned char *masknum, unsigned int *maskarray)
{
    int count = CMD_REQ_HEAD_LEN;
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
    unsigned int len;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    count += pack_string(data + count, key, 0);
    count += pack_string(data + count, value, valuelen);
    memcpy(data + count, &tag, sizeof(char));
    count += sizeof(char);

    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);

    return count;
}

int 
cmd_tag_unpack(char *data, char *key, char *value, unsigned char *valuelen, unsigned char *tag)
{
    int count = CMD_REQ_HEAD_LEN;
    unsigned char vlen;
    count += unpack_string(data + count, key, NULL);
    count += unpack_string(data + count, value, &vlen);
    memcpy(tag, data + count, sizeof(char));
    *valuelen = vlen;
    return 0;
}

int 
cmd_range_pack(char *data, char *key, unsigned char kind, unsigned char masknum, unsigned int *maskarray, 
               int frompos, int rlen)
{
    unsigned char cmd = CMD_RANGE;
    unsigned int len;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    count += pack_string(data + count, key, 0);
	memcpy(data + count, &kind, sizeof(char));
	count += sizeof(char);
    //count += pack_string(data + count, maskformat, masknum);
    count += pack_mask(data + count, maskarray, masknum);
    memcpy(data + count, &frompos, sizeof(int));
    count += sizeof(int);
    memcpy(data + count, &rlen, sizeof(int));
    count += sizeof(int);

    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);

    return count;
}

int 
cmd_range_unpack(char *data, char *key, unsigned char *kind, unsigned char *masknum, unsigned int *maskarray, 
                 int *frompos, int *len)
{
    int count = CMD_REQ_HEAD_LEN;
    //unsigned char mlen;
    count += unpack_string(data + count, key, NULL); 
	memcpy(kind, data + count, sizeof(char));
	count += sizeof(char);
    //count += unpack_string(data + count, maskformat, &mlen);
    count += unpack_mask(data + count, maskarray, masknum);
    memcpy(frompos, data + count, sizeof(int));
    count += sizeof(int);
    memcpy(len, data + count, sizeof(int));
    //*masknum = mlen;
    return 0;
}

int 
cmd_sortlist_range_pack(char *data, char *key, unsigned char kind, 
                unsigned char masknum, unsigned int *maskarray, 
                void *valmin, unsigned char vminlen, void *valmax, unsigned char vmaxlen)
{
    unsigned char cmd = CMD_SL_RANGE;
    unsigned int len;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    count += pack_string(data + count, key, 0);
	memcpy(data + count, &kind, sizeof(char));
	count += sizeof(char);
    //count += pack_string(data + count, maskformat, masknum);
    count += pack_mask(data + count, maskarray, masknum);
    //memcpy(data + count, &frompos, sizeof(int));
    //count += sizeof(int);
    count += pack_string(data + count, valmin, vminlen);
    count += pack_string(data + count, valmax, vmaxlen);
    //memcpy(data + count, &rlen, sizeof(int));
    //count += sizeof(int);

    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);

    return count;
}

int 
cmd_sortlist_range_unpack(char *data, char *key, unsigned char *kind, 
                    unsigned char *masknum, unsigned int *maskarray, 
                    void *valmin, unsigned char *vminlen, void *valmax, unsigned char *vmaxlen)
{
    int count = CMD_REQ_HEAD_LEN;
    //unsigned char mlen;
    count += unpack_string(data + count, key, NULL); 
	memcpy(kind, data + count, sizeof(char));
	count += sizeof(char);
    //count += unpack_string(data + count, maskformat, &mlen);
    count += unpack_mask(data + count, maskarray, masknum);
    //memcpy(frompos, data + count, sizeof(int));
    //count += sizeof(int);
    count += unpack_string(data + count, valmin, vminlen);
    count += unpack_string(data + count, valmax, vmaxlen);

    //memcpy(len, data + count, sizeof(int));
    //*masknum = mlen;
    return 0;
}

int 
cmd_ping_pack(char *data)
{
	unsigned char  cmd = CMD_PING;
    unsigned int  len;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char)); 
    count += sizeof(char);
    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);

	return count;
}
int 
cmd_ping_unpack(char *data)
{
	return 0;
}

int 
cmd_push_pack(char *data, unsigned char cmd, char *key, char *value, unsigned char valuelen, 
                unsigned char masknum, unsigned *maskarray)
{
    unsigned int len;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    count += pack_string(data + count, key, 0);  
    count += pack_string(data + count, value, valuelen);
    count += pack_mask(data + count, maskarray, masknum);

    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);

    return count;
}


int
cmd_push_unpack(char *data, char *key, char *value, unsigned char *valuelen,
            unsigned char *masknum, unsigned int *maskarray)
{
    int count = CMD_REQ_HEAD_LEN;

    //memcpy(cmd, data + count, sizeof(char));
    //count += sizeof(char);
    unsigned char vlen;
    count += unpack_string(data + count, key, NULL);
    count += unpack_string(data + count, value, &vlen);
    count += unpack_mask(data + count, maskarray, masknum);

    *valuelen = vlen;
    return 0;
}

int 
cmd_lpush_pack(char *data, char *key, char *value, unsigned char valuelen, 
                unsigned char masknum, unsigned *maskarray)
{
    return cmd_push_pack(data, CMD_LPUSH, key, value, valuelen, masknum, maskarray);
}

int 
cmd_rpush_pack(char *data, char *key, char *value, unsigned char valuelen, 
                unsigned char masknum, unsigned *maskarray)
{
    return cmd_push_pack(data, CMD_RPUSH, key, value, valuelen, masknum, maskarray);
}


int
cmd_pop_pack(char *data, unsigned char cmd, char *key, int num)
{
    unsigned int len;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);

    count += pack_string(data + count, key, 0);
    memcpy(data + count, &num, sizeof(int));
    count += sizeof(int);
    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);
    return count;
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
    int count = CMD_REQ_HEAD_LEN;

    //memcpy(cmd, data + count, sizeof(char));
    //count += sizeof(char);
    count += unpack_string(data + count, key, NULL); 
    memcpy(num, data + count, sizeof(int));

    return 0;
}

int 
cmd_sync_pack(char *data, unsigned int logver, unsigned int logpos)
{
    unsigned char cmd = CMD_SYNC;
    unsigned int len;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);

	memcpy(data + count, &logver, sizeof(int));
	count += sizeof(int);
	memcpy(data + count, &logpos, sizeof(int));
	count += sizeof(int);
	len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);
	
	return count;
}

int 
cmd_sync_unpack(char *data, unsigned int *logver, unsigned int *logpos)
{
	int count = CMD_REQ_HEAD_LEN;
	
	memcpy(logver, data + count, sizeof(int));
	count += sizeof(int);
	memcpy(logpos, data + count, sizeof(int));

	return 0;
}

int 
cmd_getdump_pack(char *data, unsigned int dumpver, unsigned long long size)
{
    unsigned char cmd = CMD_GETDUMP;
    unsigned int len;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);

	memcpy(data + count, &dumpver, sizeof(int));
	count += sizeof(int);
	memcpy(data + count, &size, sizeof(long long));
	count += sizeof(long long);
	len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);
	
	return count;
}

int 
cmd_getdump_unpack(char *data, unsigned int *dumpver, unsigned long long *size)
{
	int count = CMD_REQ_HEAD_LEN;
	
	memcpy(dumpver, data + count, sizeof(int));
	count += sizeof(int);
	memcpy(size, data + count, sizeof(long long));

	return 0;
}

/*int 
cmd_insert_mvalue_pack(char *data, char *key, MemLinkInsertVal *items, int num)
{
    unsigned char cmd = CMD_INSERT_MVALUE;
    unsigned int len;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    
    count += pack_string(data + count, key, 0);
    int i;

    memcpy(data + count, &num, sizeof(int));
    count += sizeof(int);

    for (i = 0; i < num; i++) {
        MemLinkInsertVal *item = &items[i];
        count += pack_string(data + count, item->value, item->valuelen);
        count += pack_mask(data + count, item->maskarray, item->masknum);
        memcpy(data + count, &item->pos, sizeof(int));
        count += sizeof(int);
    }
    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);

    return count;
}

int 
cmd_insert_mvalue_unpack(char *data, char *key, MemLinkInsertVal **items, int *num)
{
    int count = CMD_REQ_HEAD_LEN;
    MemLinkInsertVal  *values;

    count += unpack_string(data + count, key, NULL);
    memcpy(num, data + count, sizeof(int));

    values = (MemLinkInsertVal*)zz_malloc(sizeof(MemLinkInsertVal) * *num);
    if (NULL == values) {
        DERROR("malloc MemLinkInsertVal %d error!\n", (int)sizeof(MemLinkInsertVal) * *num);
        MEMLINK_EXIT;
    }
    
    int i;
    for (i = 0; i < *num; i++) {
        MemLinkInsertVal *item = &values[i];
        count += unpack_string(data + count, item->value, &item->valuelen);
        count += unpack_mask(data + count, item->maskarray, &item->masknum);
        memcpy(data + count, &item->pos, sizeof(int));
        count += sizeof(int);
    }

    return 0;
}
*/
//add by lanwenhong
int
cmd_del_by_mask_pack(char *data, char *key, unsigned int *maskarray, unsigned char masknum)
{
	unsigned char cmd = CMD_DEL_BY_MASK;
	int count = CMD_REQ_SIZE_LEN;
	unsigned int len;

	memcpy(data + count, &cmd, sizeof(char));
	count += sizeof(char);
	count += pack_string(data + count, key, 0);
	count += pack_mask(data + count, maskarray, masknum);

	len = count - CMD_REQ_SIZE_LEN;
	memcpy(data, &len, CMD_REQ_SIZE_LEN);

	return count;
}

int
cmd_del_by_mask_unpack(char *data, char *key, unsigned int *maskarray, unsigned char *masknum)
{
	int count = CMD_REQ_HEAD_LEN;

	count += unpack_string(data + count, key, NULL);
	count += unpack_mask(data + count, maskarray, masknum);

	return 0;
}

int
cmd_insert_mkv_pack(char *data, MemLinkInsertMkv *mkv)
{
	unsigned char cmd = CMD_INSERT_MKV;
	unsigned int len;
	int count = CMD_REQ_SIZE_LEN;
	MemLinkInsertKey *keyitem;
	MemLinkInsertVal *valitem;

	memcpy(data + count, &cmd, sizeof(char));
	count += sizeof(char);
	//memcpy(data + count, &(mkv->keynum), sizeof(int));
	//count += sizeof(int);

	keyitem = mkv->keylist;

	while(keyitem != NULL) {
		valitem = keyitem->vallist;
		//memcpy(data + count, &(keyitem->keylen), sizeof(char));
		//count += sizeof(char);
		//memcpy(data + count , keyitem->key, keyitem->keylen);
		//count += keyitem->keylen;
		count += pack_string(data + count, keyitem->key, 0);
		memcpy(data + count, &(keyitem->valnum), sizeof(int));
		count += sizeof(int);

		while (valitem != NULL) {
			count += pack_string(data + count, valitem->value, valitem->valuelen);
			count += pack_mask(data + count, valitem->maskarray, valitem->masknum);
			memcpy(data + count, &(valitem->pos), sizeof(int));
			count += sizeof(int);
			valitem = valitem->next;
		}
		keyitem = keyitem->next;
	}

	len = count - CMD_REQ_SIZE_LEN;
	memcpy(data, &len, CMD_REQ_SIZE_LEN);
	
	return count;
}

int
cmd_insert_mkv_unpack_packagelen(char *data, unsigned int *package_len)
{
	int count = 0;

	memcpy(package_len, data, sizeof(int));
	count += sizeof(int);
	return count;
}

int
cmd_insert_mkv_unpack_keycount(char *data, unsigned int *keycount)
{
	int count = 0;

	memcpy(keycount, data, sizeof(int));
	count += sizeof(int);
	
	return count;
}

int
cmd_insert_mkv_unpack_key(char *data, char *key, unsigned int *valcount, char **countstart)
{
	int count = 0;
	
	count += unpack_string(data, key, NULL);
	memcpy(valcount, data + count, sizeof(int));
	//记录每个key下面value个数在包中的位置， 这个可能被修改
	*countstart = data + count;
	count += sizeof(int);

	return count;
}

int
cmd_insert_mkv_unpack_val(char *data, char *value, unsigned char *valuelen,
	unsigned char *masknum, unsigned int *maskarray, int *pos)
{
	int count = 0;
	unsigned char vlen;

	count += unpack_string(data, value, &vlen);
	*valuelen = vlen;
	count += unpack_mask(data + count, maskarray, masknum);
	memcpy(pos, data + count, sizeof(int));
	count += sizeof(int);

	return count;
}

int
cmd_read_conn_info_pack(char *data)
{
	unsigned char cmd = CMD_READ_CONN_INFO;
	int count = CMD_REQ_SIZE_LEN;

	memcpy(data + count, &cmd, sizeof(char));
	count += sizeof(char);
	
	int len = count - CMD_REQ_SIZE_LEN;
	memcpy(data, &len, CMD_REQ_SIZE_LEN);
	return count;
}
int
cmd_read_conn_info_unpack(char *data)
{
	return 0;
}


int
cmd_write_conn_info_pack(char *data)
{
	unsigned char cmd = CMD_WRITE_CONN_INFO;
	int count = CMD_REQ_SIZE_LEN;

	memcpy(data + count, &cmd, sizeof(char));
	count += sizeof(char);
	
	int len = count - CMD_REQ_SIZE_LEN;

	memcpy(data, &len, CMD_REQ_SIZE_LEN);
	return count;
}
int
cmd_write_conn_info_unpack(char *data)
{
	return 0;
}

int
cmd_sync_conn_info_pack(char *data)
{
	unsigned char cmd = CMD_SYNC_CONN_INFO;
	int count = CMD_REQ_SIZE_LEN;

	memcpy(data + count, &cmd, sizeof(char));
	count += sizeof(char);
	
	int len = count - CMD_REQ_SIZE_LEN;

	memcpy(data, &len, CMD_REQ_SIZE_LEN);
	return count;
}

int
cmd_sync_conn_info_unpack(char *data)
{
	return 0;
}

int 
pack_config_struct(char *data, MyConfig *config)
{
    unsigned int count = 0;
    int i;
    int item_count = 0;
    int ret;
    
    //pack bloc_data_count array
    count += sizeof(int);
    for (i = 0; i < BLOCK_DATA_COUNT_MAX; i++) {
        if (config->block_data_count[i] > 0) {
            memcpy(data + count, &(config->block_data_count[i]), sizeof(int));
            item_count++;
            count += sizeof(int);
        }
    }
    memcpy(data, &item_count, sizeof(int)); 

    memcpy(data + count, &config->block_data_count_items, sizeof(int));
    count += sizeof(int);

    memcpy(data + count, &config->block_data_reduce, sizeof(float));
    count += sizeof(float);

    memcpy(data + count, &config->dump_interval, sizeof(int));
    count += sizeof(int);

    memcpy(data + count, &config->block_clean_cond, sizeof(float));
    count += sizeof(float);

    memcpy(data + count, &config->block_clean_start, sizeof(int));
    count += sizeof(int);

    memcpy(data + count, &config->block_clean_num, sizeof(int));
    count += sizeof(int);

    memcpy(data + count, &config->read_port, sizeof(int));
    count += sizeof(int);

    memcpy(data + count, &config->write_port, sizeof(int));
    count += sizeof(int);

    memcpy(data + count, &config->sync_port, sizeof(int));
    count += sizeof(int);
    
    ret = pack_string(data + count, config->datadir, 0);
    count += ret;

    memcpy(data + count, &config->log_level, sizeof(int));
    count += sizeof(int);

    ret = pack_string(data + count, config->log_name, 0);
    count += ret;

    memcpy(data + count, &config->write_binlog, sizeof(int));
    count += sizeof(int);

    memcpy(data + count, &config->timeout, sizeof(int));
    count += sizeof(int);

    memcpy(data + count, &config->thread_num, sizeof(int));
    count += sizeof(int);

    memcpy(data + count, &config->max_conn, sizeof(int));
    count += sizeof(int);

    memcpy(data + count, &config->max_core, sizeof(int));
    count += sizeof(int);

    memcpy(data + count, &config->is_daemon, sizeof(int));
    count += sizeof(int);
    
    memcpy(data + count, &config->role, sizeof(char));
    count += sizeof(char);

    ret = pack_string(data + count, config->master_sync_host, 0);
    count += ret;

    memcpy(data + count, &config->master_sync_port, sizeof(int));
    count += sizeof(int);

    memcpy(data + count, &config->sync_interval, sizeof(int));
    count += sizeof(int);
    
    return count;
}

int 
unpack_config_struct(char *data, MyConfig *config)
{
    unsigned int count = 0;
    unsigned int item_count = 0;
    unsigned char len;

    memcpy(&item_count, data + count, sizeof(int));
    count += sizeof(int);

    int i;
    for (i = 0; i < item_count; i++) {
        memcpy(&config->block_data_count[i], data + count, sizeof(int));
        count += sizeof(int);
    }

    memcpy(&config->block_data_count_items, data + count, sizeof(int)); 
    count += sizeof(int);

    memcpy(&config->block_data_reduce, data + count, sizeof(float));
    count += sizeof(float);

    memcpy(&config->dump_interval, data + count, sizeof(int));
    count += sizeof(int);

    memcpy(&config->block_clean_cond, data + count, sizeof(float));
    count += sizeof(float);

    memcpy(&config->block_clean_start, data + count, sizeof(int));
    count += sizeof(int);

    memcpy(&config->block_clean_num, data + count, sizeof(int));
    count += sizeof(int);

    memcpy(&config->read_port, data + count, sizeof(int));
    count += sizeof(int);

    memcpy(&config->write_port, data + count, sizeof(int));
    count += sizeof(int);

    memcpy(&config->sync_port, data + count, sizeof(int));
    count += sizeof(int);

    len = unpack_string(data + count, config->datadir, NULL);
    count += len;
    
    memcpy(&config->log_level, data + count, sizeof(int));
    count += sizeof(int);

    len = unpack_string(data + count, config->log_name, NULL);
    count += len;

    memcpy(&config->write_binlog, data + count, sizeof(int));
    count += sizeof(int);

    memcpy(&config->timeout, data + count, sizeof(int));
    count += sizeof(int);
    
    memcpy(&config->thread_num, data + count,  sizeof(int));
    count += sizeof(int);

    memcpy(&config->max_conn, data + count, sizeof(int));
    count += sizeof(int);

    memcpy(&config->max_core, data + count, sizeof(int));
    count += sizeof(int);
    
    memcpy(&config->is_daemon, data + count,  sizeof(int));
    count += sizeof(int);

    memcpy(&config->role, data + count, sizeof(char));
    count += sizeof(char);

    len = unpack_string(data + count, config->master_sync_host, NULL);
    count += len;

    memcpy(&config->master_sync_port, data + count, sizeof(int));
    count += sizeof(int);

    memcpy(&config->sync_interval, data + count, sizeof(int));
    count += sizeof(int);
    
    return count;
}

int
cmd_config_info_pack(char *data)
{
    unsigned char cmd = CMD_CONFIG_INFO;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);

    int len = count - CMD_REQ_SIZE_LEN;

    memcpy(data, &len, CMD_REQ_SIZE_LEN);
    return count;
}

int
cmd_config_info_unpack(char *data)
{
    return 0;
}

int
cmd_set_config_dynamic_pack(char *data, char *key, char *value)
{
    unsigned char cmd = CMD_SET_CONFIG;
    int count = CMD_REQ_SIZE_LEN;
    int ret;
    
    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);

    ret = pack_string(data + count, key, 0); 
    count += ret;
    
    ret = pack_string(data + count, value, 0);
    count += ret;

    int len = count - CMD_REQ_SIZE_LEN;

    memcpy(data, &len, sizeof(int));

    return count;

}

int
cmd_set_config_dynamic_unpack(char *data, char *key, char *value)
{
    unsigned int count = CMD_REQ_HEAD_LEN;
    unsigned char len;
    
    len = unpack_string(data + count, key, NULL);
    count += len;
    
    len = unpack_string(data + count, value, NULL);
    count += len;

    return 0;
}

int
cmd_clean_all_pack(char *data)
{
    unsigned char cmd = CMD_CLEAN_ALL;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    int len = count - CMD_REQ_SIZE_LEN;

    memcpy(data, &len, sizeof(int));
    return count;
}

int
cmd_clean_all_unpack(char *data)
{
    return 0;
}
/**
 * @}
 */
