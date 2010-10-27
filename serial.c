#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "logfile.h"
#include "serial.h"
#include "utils.h"

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
        if (m[0] == ':' || m[0] == 0) {
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
        v = v << b;
        //DINFO("v: %x %b\n", v, v); 
        unsigned char m = pow(2, b) - 1;
        unsigned char x = mask[idx] & m;
        //DINFO("m: %d, %x, x: %d, %x\n", m, m, x, x);

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

        //DINFO("============================\n");
    }
    n = b / 8;
    if (n > 0) {
        idx += n;
        b = b % 8;
    }

    if (b > 0) {
        mask[idx] = mask[idx] & (char)(pow(2, b) - 1);
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
int2string(char *s, unsigned int val)
{
    unsigned int v = val;
    int yu;
    int ret, i = 0, j = 0;
    char ss[32];

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
		int yu = ((fs + n) % 8) > 0 ? 1: 0;
		int cs = (fs + n) / 8 + yu;
	    
        //DINFO("fs:%d, yu: %d, cs: %d\n", fs, yu, cs);
		val = 0;
		
		//DINFO("idx:%d, cs:%d, n:%d, yu:%d\n", idx, cs, n, yu);
		memcpy(&val, &mask[idx], cs);
		val <<= 32 - fs - n;
		val >>= 32 - fs;

		//DINFO("i:%d, val:%d\n", i, val);
        //DINFO("i: %d, widx: %d\n", i, widx);
		if (widx != 0) {
			maskstr[widx] = ':';	
			widx ++;
		}
		//int len = sprintf(&maskstr[widx], "%d", val);
		int len = int2string(&maskstr[widx], val);
		widx += len;
		//maskstr[widx] = val;

		idx += cs - 1;

		if (yu == 0) {
			idx += 1;
		}
		if (fs + n > 8) {
			n = yu;
		}else{
			n = fs + n;
		}
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

    DINFO("masknum: %d\n", masknum);

    for (i = 0; i < masknum; i++) {
        mf = maskformat[i];
        m  = maskarray[i];
        
        DINFO("i: %d, format: %d, array: %d\n", i, mf, m);
        if (m == UINT_MAX) { // set to 1
            xlen = (b + mf) / 8 + ((b + mf) % 8 > 0 ? 1:0);
            v = pow(2, mf) - 1;
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
    unsigned short len = sizeof(char);
    unsigned char  cmd = CMD_DUMP;

    memcpy(data, &len, sizeof(short));
    memcpy(data + sizeof(short), &cmd, sizeof(char));

    return len + sizeof(short);
}

int 
cmd_dump_unpack(char *data)
{
    //unsigned short len;
    //int count = sizeof(short) + sizeof(char);

    //memcpy(&len, data + count, sizeof(short));
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
    //DINFO("clean len: %d, count: %d\n", len, count);
    memcpy(data, &len, sizeof(short));
    
    return count;
}

int 
cmd_clean_unpack(char *data, char *key)
{
    unsigned char keylen;
    int count = sizeof(short) + sizeof(char);

    unpack_string(data + count, key, &keylen);

    return 0;
}

int 
cmd_rmkey_pack(char *data, char *key)
{
    unsigned char  cmd = CMD_RMKEY;
    unsigned short  len;
    int count = sizeof(short);
    int ret;

    memcpy(data + count, &cmd, sizeof(char)); 
    count += sizeof(char);

    ret = pack_string(data + count, key, 0);
    count += ret;
    len = count - sizeof(short);
    //DINFO("clean len: %d, count: %d\n", len, count);
    memcpy(data, &len, sizeof(short));
    
    return count;
}

int 
cmd_rmkey_unpack(char *data, char *key)
{
    unsigned char keylen;
    int count = sizeof(short) + sizeof(char);

    unpack_string(data + count, key, &keylen);

    return 0;
}


int 
cmd_count_pack(char *data, char *key, unsigned char masknum, unsigned int *maskarray)
{
    unsigned char  cmd = CMD_COUNT;
    unsigned short  len;
    int count = sizeof(short);
    int ret;

    memcpy(data + count, &cmd, sizeof(char)); 
    count += sizeof(char);

    ret = pack_string(data + count, key, 0);
    count += ret;
	count += pack_mask(data + count, maskarray, masknum);

    len = count - sizeof(short);
    memcpy(data, &len, sizeof(short));
    
    return count;
}

int 
cmd_count_unpack(char *data, char *key, unsigned char *masknum, unsigned int *maskarray)
{
    unsigned char keylen;
    int count = sizeof(short) + sizeof(char);

    count += unpack_string(data + count, key, &keylen);
	unpack_mask(data + count, maskarray, masknum);

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
    int count = sizeof(short) + sizeof(char);

    count = unpack_string(data + count, key, &keylen);
    //memcpy(stat, data + count, sizeof(HashTableStat)); 

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
cmd_sync_unpack(char *data, unsigned int *logver, unsigned int *log_index_pos) 
{
    int count = sizeof(short) + sizeof(char);
    memcpy(logver, data + count, sizeof(int));
    count += sizeof(int);
    memcpy(log_index_pos, data + count, sizeof(int));
    return 0;
}

int 
cmd_sync_dump_unpack(char *data, unsigned int *dumpver, unsigned int *size)
{
    int count = sizeof(short) + sizeof(char);
    memcpy(dumpver, data + count, sizeof(int));
    count += sizeof(int);
    memcpy(size, data + count, sizeof(int));
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
    count += sizeof(int);

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

    len = count - sizeof(short);
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
    
    memcpy(data, &len, sizeof(short));

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

int 
cmd_sync_pack(char *data, unsigned int logver, unsigned int logpos)
{
    unsigned char cmd = CMD_SYNC;
    unsigned short len;
    int count = sizeof(short);

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);

	memcpy(data + count, &logver, sizeof(int));
	count += sizeof(int);
	memcpy(data + count, &logpos, sizeof(int));
	count += sizeof(int);
	
	return count;
}

int 
cmd_sync_unpack(char *data, unsigned int *logver, unsigned int *logpos)
{
	int count = sizeof(short) + sizeof(char);
	
	memcpy(logver, data + count, sizeof(int));
	count += sizeof(int);
	memcpy(logpos, data + count, sizeof(int));

	return 0;
}

int 
cmd_getdump_pack(char *data, unsigned int dumpver, unsigned int size)
{
    unsigned char cmd = CMD_GETDUMP;
    unsigned short len;
    int count = sizeof(short);

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);

	memcpy(data + count, &dumpver, sizeof(int));
	count += sizeof(int);
	memcpy(data + count, &size, sizeof(int));
	count += sizeof(int);
	
	return count;
}

int 
cmd_getdump_unpack(char *data, unsigned int *dumpver, unsigned int *size)
{
	int count = sizeof(short) + sizeof(char);
	
	memcpy(logver, data + count, sizeof(int));
	count += sizeof(int);
	memcpy(logpos, data + count, sizeof(int));

	return 0;
}


