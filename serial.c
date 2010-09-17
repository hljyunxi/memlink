#include <stdlib.h>
#include <string.h>
#include "serial.h"

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
    //unsigned char  keylen = strlen(key);
    unsigned char  cmd = CMD_CLEAN;
    unsigned char  len;
    int count = sizeof(short);
    int ret;

    memcpy(data + count, &cmd, sizeof(char)); 
    count += sizeof(char);
    //memcpy(data + count, &keylen, sizeof(char));
    //count += sizeof(char);
    //memcpy(data + count, &key, keylen);
    //count += keylen;

    ret = pack_string(data + count, key, 0);
    count += ret;
    len = count - sizeof(short);
    memcpy(data, &len, sizeof(short));
    
    return count;
}

int 
cmd_clean_unpack(char *data, char *key)
{
    unsigned char keylen;
    //int n = sizeof(short) + sizeof(char);

    //memcpy(&keylen, data + n, sizeof(char));
    //n += sizeof(char);
    //memcpy(*key, data + n, keylen);

    unpack_string(data, key, &keylen);

    return 0;
}

int 
cmd_stat_pack(char *data, char *key)
{
    //unsigned char  keylen = strlen(key);
    unsigned char  cmd = CMD_STAT;
    unsigned short count = sizeof(short);
    unsigned short len;

    memcpy(data + count, &cmd, sizeof(char)); 
    count += sizeof(char);
    /*
    memcpy(data + count, &keylen, sizeof(char));
    count += sizeof(char);
    memcpy(data + count, &key, keylen);
    count += keylen;
    */

    count += pack_string(data + count, key, 0);

    len = count - sizeof(short);
    memcpy(data, &len, sizeof(short));
    
    return count;
}

int 
cmd_stat_unpack(char *data, char *key)
{
    return cmd_clean_unpack(data, key);
}

int 
cmd_create_pack(char *data, unsigned char valuelen, unsigned char masknum, char *maskformat)
{
    unsigned char  cmd = CMD_STAT;
    unsigned short count = sizeof(short);

    memcpy(data + count, &cmd, sizeof(char)); 
    count += sizeof(char);
    memcpy(data + count, &valuelen, sizeof(char));
    count += sizeof(char);

    /*
    memcpy(data + count, &masknum, sizeof(char));
    count += sizeof(char);
    memcpy(data + count, maskformat, masknum);
    count += masknum;
    */
    count += pack_string(data + count, maskformat, masknum);
    count -= sizeof(short);
    memcpy(data, &count, sizeof(short));
    
    return count + sizeof(short);

}

int 
cmd_create_unpack(char *data, unsigned char *valuelen, unsigned char *masknum, char *maskformat)
{
    //unsigned short len; 
    int count = sizeof(short) + sizeof(char);

    memcpy(valuelen, data + count, sizeof(char));
    count += sizeof(char);
    /*
    memcpy(masknum, data + count, sizeof(char));
    count += sizeof(char);
    memcpy(*maskformat, data + count, *masknum);
    */
    unpack_string(data + count, maskformat, masknum);

    return 0;
}


int 
cmd_del_pack(char *data, char *key, char *value, unsigned char valuelen)
{
    unsigned char cmd = CMD_DEL;
    unsigned short len;
    //unsigned char keylen = strlen(key);
    int count = sizeof(short);

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    /*
    memcpy(data + count, &keylen, sizeof(char));
    count += sizeof(char);
    memcpy(data + count, key, keylen);
    count += keylen;
    memcpy(data + count, &valuelen, sizeof(char)); 
    count += sizeof(char);
    memcpy(data + count, value, valuelen);
    count += valuelen;
    */

    count += pack_string(data + count, key, 0);
    count += pack_string(data + count, value, valuelen);

    len = count -= sizeof(short);
    memcpy(data, &len, sizeof(short));

    return count;
}

int 
cmd_del_unpack(char *data, char *key, char *value, unsigned char *valuelen)
{
    int count = sizeof(short) + sizeof(char);
    //unsigned char keylen;
    
    count += unpack_string(data + count, key, NULL);
    unpack_string(data + count, value, valuelen);
    
    return 0;
}

int 
cmd_insert_pack(char *data, char *key, char *value, unsigned char valuelen, 
                unsigned char masknum, char *maskformat, unsigned int pos)
{
    unsigned char cmd = CMD_INSERT;
    unsigned short len;
    int count = sizeof(short);

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    count += pack_string(data + count, key, 0);  
    count += pack_string(data + count, value, valuelen);
    count += pack_string(data + count, maskformat, masknum);
    memcpy(data + count, &pos, sizeof(int));

    len = count - sizeof(short);
    memcpy(data, &len, sizeof(short));
    
    return count;
}

int 
cmd_insert_unpack(char *data, char *key, char *value, unsigned char *valuelen,
                  unsigned char *masknum, char *maskformat, unsigned int *pos)
{
    int count = sizeof(short) + sizeof(char);

    unsigned char vlen;
    count += unpack_string(data + count, key, NULL);
    count += unpack_string(data + count, value, &vlen);
    count += unpack_string(data + count, maskformat, masknum);
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
              unsigned char masknum, char *maskformat)
{
    unsigned char cmd = CMD_MASK;
    unsigned short len;
    int count = sizeof(short);

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    count += pack_string(data + count, key, 0);
    count += pack_string(data + count, value, valuelen);
    count += pack_string(data + count, maskformat, masknum);

    len = count - sizeof(short);
    
    memcpy(data + count, &len, sizeof(short));
    return count;
}

int 
cmd_mask_unpack(char *data, char *key, char *value, unsigned char *valuelen, 
                unsigned char *masknum, char *maskformat)
{
    int count = sizeof(short) + sizeof(char);
    unsigned char vlen, mlen; 

    count += unpack_string(data + count, key, NULL);
    count += unpack_string(data + count, value, &vlen);
    count += unpack_string(data + count, maskformat, &mlen);

    *valuelen = vlen;
    *masknum  = mlen;
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
cmd_range_pack(char *data, char *key, unsigned char masknum, char *maskformat, 
               unsigned int frompos, unsigned int rlen)
{
    unsigned char cmd = CMD_RANGE;
    unsigned short len;
    int count = sizeof(short);

    memcpy(data + count, &cmd, sizeof(char));
    count += sizeof(char);
    count += pack_string(data + count, key, 0);
    count += pack_string(data + count, maskformat, masknum);
    memcpy(data + count, &frompos, sizeof(int));
    count += sizeof(int);
    memcpy(data + count, &rlen, sizeof(int));
    count += sizeof(int);

    len = count - sizeof(short);
    memcpy(data, &len, sizeof(short));
    return count;
}

int 
cmd_range_unpack(char *data, char *key, unsigned char *masknum, char *maskformat, 
                 unsigned int *frompos, unsigned int *len)
{
    int count = sizeof(short) + sizeof(char);
    unsigned char mlen;
    count += unpack_string(data + count, key, NULL); 
    count += unpack_string(data + count, maskformat, &mlen);
    memcpy(frompos, data + count, sizeof(int));
    count += sizeof(int);
    memcpy(len, data + count, sizeof(int));
    *masknum = mlen;
    return 0;
}




