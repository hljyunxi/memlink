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
            b += maskformat[i];
            continue;
        }
        
        unsigned char y = (b + mf) % 8;
        n = (b + mf) / 8 + (y>0 ? 1: 0);
        //DINFO("y: %d, n: %d\n", y, n);
		if (n > 4) {
			flow = v >> (32 - b);
		}
        v = v << b;
        //DINFO("v: %x %b\n", v, v); 
        //unsigned char m = pow(2, b) - 1;
        unsigned char m = 0xffffffff >> (32 - b);
        unsigned char x = mask[idx] & m;
        //DINFO("m: %d, %x, x: %d, %x\n", m, m, x, x);

        v = v | x;
		
		//modified by wyx 12/31
        //m = 0xff >> y;
        //m = m << y;
        //x = mask[idx + n - 1] & m;

        //DINFO("copy idx:%d, v:%d, n:%d\n", idx, v, n);
        //printb((char *)&v, n);
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
        
        //mask[idx] = mask[idx] | x;
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
        //DINFO("i: %d\n", i);
		int fs = maskformat[i];
		int offset = (fs + n) % 8;
		int yu = offset > 0 ? 1: 0;
		int cs = (fs + n) / 8 + yu;
	    
        //DINFO("fs:%d, yu: %d, cs: %d\n", fs, yu, cs);
		val = 0;
		
		//DINFO("idx:%d, cs:%d, n:%d, yu:%d\n", idx, cs, n, yu);
		memcpy(&val, &mask[idx], cs);
        //DERROR("%d: %x, n:%d\n", i, val, n);
		val <<= 32 - fs - n;
		val >>= 32 - fs;

		//DINFO("i:%d, val:%d\n", i, val);
        //DINFO("i: %d, widx: %d\n", i, widx);
		if (widx != 0) {
			maskstr[widx] = ':';	
			widx ++;
		}
		//int len = sprintf(&maskstr[widx], "%d", val);		
        //DERROR("%d: %d\n", i, val);
		int len = int2string(&maskstr[widx], val);		
        //DERROR("%d: %c\n", i, maskstr[widx]);
		widx += len;
		//maskstr[widx] = val;

		idx += cs - 1;

		if (yu == 0) {
			idx += 1;
		}
		//if (fs + n >= 8) {
			n = offset;
		//}else{
			//n = fs + n;
		//}
	}
    maskstr[widx] = 0;

	return widx;
}

/**
 * mask必须先初始化为0
 */
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

    memset(mask, 0, masknum);

    mask[0] = 0x03;
    b = 2;

    DINFO("masknum: %d\n", masknum);
    v = 0;
    for (i = 0; i < masknum; i++) {
        mf = maskformat[i];
        m  = maskarray[i];
        
        DINFO("i: %d, format: %d, array: %d\n", i, mf, m);
        if (m == UINT_MAX) { // set to 1
            xlen = (b + mf) / 8 + ((b + mf) % 8 > 0 ? 1:0);
            //v = pow(2, mf) - 1;
            v = 0xffffffff >> (32 - mf);
            //v = 0xffffffff;
            v = v << b;

            char *cpdata = (char *)&v;
            for (j = 0; j < xlen; j++) {
                mask[idx + j] |= cpdata[j];
            }
            
            idx += xlen - 1;
            b = (b + mf) % 8;
        }else{ // set to 0
            b += mf;
            if (b >= 8) {
                idx++;
                b = b % 8;
            }
        }
    }

    if (b > 0) {
        //mask[idx] = mask[idx] & (char)(pow(2, b) - 1);
        mask[idx] = mask[idx] & (char)(UCHAR_MAX >> (8 - b));
    }

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
    unsigned char  cmd = CMD_COUNT;
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
cmd_sortlist_del_pack(char *data, char *key, char *valmin, unsigned char vminlen, 
            char *valmax, unsigned char vmaxlen)
{
    unsigned char cmd = CMD_DEL;
    unsigned int len;
    int count = CMD_REQ_SIZE_LEN;

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);

    count += pack_string(data + count, key, 0);
    count += pack_string(data + count, valmin, vminlen);
    count += pack_string(data + count, valmax, vmaxlen);

    len = count - CMD_REQ_SIZE_LEN;
    memcpy(data, &len, CMD_REQ_SIZE_LEN);

    return count;
}

int 
cmd_sortlist_del_unpack(char *data, char *key, char *valmin, unsigned char *vminlen,
                        char *valmax, unsigned char *vmaxlen)
{
    int count = CMD_REQ_HEAD_LEN;
    unsigned char len;
    
    count += unpack_string(data + count, key, &len);
    count += unpack_string(data + count, valmin, vminlen);
    unpack_string(data + count, valmax, vmaxlen);
    
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

int 
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
/**
 * @}
 */
