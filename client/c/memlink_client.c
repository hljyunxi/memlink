#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <netdb.h>
#ifdef __linux
    #include <linux/if.h>
    #include <sys/un.h>
#endif
#include "memlink_client.h"
#include "logfile.h"
#include "zzmalloc.h"
#include "utils.h"

#define RECV_PKG_SIZE_LEN	sizeof(int)

MemLink*    
memlink_create(char *host, int readport, int writeport, int timeout)
{
    MemLink *m;

#ifdef DEBUG
	logfile_create("stdout", 1);
#endif

    m = (MemLink*)zz_malloc(sizeof(MemLink));
    if (NULL == m) {
        DERROR("malloc MemLink error!\n"); 
        return NULL;
    }
    memset(m, 0, sizeof(MemLink));

    snprintf(m->host, 16, "%s", host);
    m->read_port  = readport;
    m->write_port = writeport;
    m->timeout    = timeout;

    return m;
}


static int
memlink_connect(MemLink *m, int fdtype)
{
    int ret;
    int sock;
    
    DINFO("memlink connect ...\n");
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sock) {
        DERROR("socket create error: %s\n", strerror(errno));
        return -1;
    }

    struct linger ling = {0, 0}; 
    ret = setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));
    if (ret != 0) {
        DERROR("setsockopt LINGER error: %s\n", strerror(errno));
    }

    int flags = 1;
    ret = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));
    if (ret != 0) {
        DERROR("setsockopt NODELAY error: %s\n", strerror(errno));
    }

    flags = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&flags, sizeof(flags));

    struct sockaddr_in  sin; 

    sin.sin_family = AF_INET;
    if (fdtype == MEMLINK_READER) {
        sin.sin_port = htons((short)(m->read_port));
    }else if (fdtype == MEMLINK_WRITER) {
        sin.sin_port = htons((short)(m->write_port));
    }else{
        return -2;
    }

    if (m->host[0] == 0) {
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    }else{
        sin.sin_addr.s_addr = inet_addr(m->host);
    }    

    do { 
        ret = connect(sock, (struct sockaddr*)&sin, sizeof(sin));
    } while (ret == -1 && errno == EINTR);  

    DINFO("connect ret: %d\n", ret);
    if (ret >= 0) {
        if (fdtype == MEMLINK_READER) {
            m->readfd = sock;
        }else{
            m->writefd = sock;
        }
    }else{
        DINFO("connect error: %s\n", strerror(errno));
        close(sock);
    }

    return ret;
}

// TODO should check the received data is not bigger than rlen
static int
memlink_read(MemLink *m, int fdtype, char *rbuf, int rlen)
{
    //int rdlen = 0; 
    //int nleft = rlen; // TODO remove it if useless
    unsigned int datalen = 0;
    int ret;
    int fd;
   
    if (fdtype == MEMLINK_READER) {
        fd = m->readfd;
    }else if (fdtype == MEMLINK_WRITER) {
        fd = m->writefd;
    }else{
        return -1;
    }
    //DINFO("fd: %d\n", fd);
    
    if (fd <= 0) {
        DINFO("read fd connect ...\n");
        ret = memlink_connect(m, fdtype);
        if (ret < 0)
            return ret;
    }
	
    ret = readn(fd, rbuf, RECV_PKG_SIZE_LEN, m->timeout);
    DINFO("read head: %d\n", ret);
    if (ret != RECV_PKG_SIZE_LEN) {
        DERROR("read head error!\n");
        close(fd);

        if (fd == m->readfd) {
            m->readfd = 0;
        }else{
            m->writefd = 0;
        }
        return -2;
    }
    memcpy(&datalen, rbuf, RECV_PKG_SIZE_LEN);
    DINFO("read body: %d\n", datalen);

    if (datalen + RECV_PKG_SIZE_LEN > rlen) {
        DERROR("datalen bigger than buffer size: %d, %d\n", datalen, rlen);
        return -3;
    }

    ret = readn(fd, rbuf + RECV_PKG_SIZE_LEN, datalen, m->timeout);
    //DINFO("readn return: %d\n", ret);
    if (ret != datalen) {
        DERROR("read data error! ret:%d\n", ret);
        close(fd);

        if (fd == m->readfd) {
            m->readfd = 0;
        }else{
            m->writefd = 0;
        }

		return -4;
    }

    return ret + RECV_PKG_SIZE_LEN;
}

static int
memlink_write(MemLink *m, int fdtype, char *wbuf, int wlen)
{
    //int wrlen = 0; 
    //int nleft = wlen;
    int ret;
    int fd;
   
    if (fdtype == MEMLINK_READER) {
        fd = m->readfd;
    }else if (fdtype == MEMLINK_WRITER) {
        fd = m->writefd;
    }else{
        return -100;
    }
    //DINFO("fd: %d\n", fd);
    if (fd <= 0) {
        //DINFO("write fd connect ...\n");
        ret = memlink_connect(m, fdtype);
        DINFO("memlink_connect return: %d\n", ret);
        if (ret < 0)
            return ret;

        if (fdtype == MEMLINK_READER) {
            fd = m->readfd;
        }else{
            fd = m->writefd;
        }
    }

    ret = writen(fd, wbuf, wlen, m->timeout);
    //DINFO("writen: %d\n", ret);
    if (ret < 0) {
        DERROR("write data error, ret: %d\n", ret);
        close(fd);
        if (fd == m->readfd) {
            m->readfd = 0;
        }else{
            m->writefd = 0;
        }

    }

    return ret;
}

/**
 * Send a command to memlink server and recieve the response. Response format:
 * -----------------------------------------------------------
 * | response length (2 bytes) | return code (2 bytes)| data |
 * -----------------------------------------------------------
 *
 * @param m memlink client
 * @param fdtype read or write
 * @param data command data
 * @param len command length
 * @param retdata return data
 * @param retlen max length of return data
 * @return return code
 *
 */
static int
memlink_do_cmd(MemLink *m, int fdtype, char *data, int len, char *retdata, int retlen)
{
    int ret;
	DINFO("======= write to server ...\n");
    ret = memlink_write(m, fdtype, data, len);
    DINFO("memlink_write ret: %d, len: %d\n", ret, len);
    
    if (ret >= 0 && ret != len) {
        DERROR("memlink write data error! ret:%d, len:%d\n", ret, len);
        return MEMLINK_ERR_SEND;
    }
	DINFO("read from server ...\n"); 
    ret = memlink_read(m, fdtype, retdata, retlen);
    DINFO("memlink_read return: %d\n", ret);

    if (ret > 0) {
        short retcode;
#ifdef DEBUG
        char buf[10240];
        DINFO("ret:%d, read: %s\n", ret, formath(retdata, ret, buf, 10240));
#endif
        memcpy(&retcode, retdata + RECV_PKG_SIZE_LEN, sizeof(short));
        DINFO("retcode: %d\n", retcode);
		
		if (retcode != 0) {
			return retcode;
		}
        DINFO("ret: %d\n", ret);
		return ret;  // return read data len
    }else{
        DERROR("memlink recv data error! ret:%d\n", ret);
		return MEMLINK_ERR_RECV;
	}
}

int
memlink_cmd_ping(MemLink *m)
{
    char data[1024];
    int  len, ret;

    len = cmd_ping_pack(data);
    DINFO("pack ping len: %d\n", len); 

    char retdata[1024];
    ret = memlink_do_cmd(m, MEMLINK_READER, data, len, retdata, 1024);

	if (ret < 0) 
		return ret;
	return MEMLINK_OK;
}

int
memlink_cmd_dump(MemLink *m)
{
    char data[1024];
    int  len, ret;

    len = cmd_dump_pack(data);
    DINFO("pack dump len: %d\n", len); 

    char retdata[1024];
    ret = memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);

	if (ret < 0) 
		return ret;
	return MEMLINK_OK;
}

int
memlink_cmd_clean(MemLink *m, char *key)
{
	if (NULL == key || strlen(key) > HASHTABLE_KEY_MAX)
		return MEMLINK_ERR_PARAM;

    char data[1024];
    int  len;
    //unsigned short dlen;

    len = cmd_clean_pack(data, key);
    //memcpy(&dlen, data, sizeof(short));
    //DINFO("pack clean len: %d, pkg body len: %d\n", len, dlen);
    DINFO("pack clean len: %d\n", len);

    char retdata[1024];
    int ret = memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);
	if (ret < 0) 
		return ret;
	return MEMLINK_OK;
}

int 
memlink_cmd_stat(MemLink *m, char *key, MemLinkStat *stat)
{
	if (NULL == key || strlen(key) > HASHTABLE_KEY_MAX)
		return MEMLINK_ERR_PARAM;

    char data[1024];
    int  len;

    len = cmd_stat_pack(data, key);
    DINFO("pack stat len: %d\n", len);

    char retdata[1024];
    int ret = memlink_do_cmd(m, MEMLINK_READER, data, len, retdata, 1024);
    DINFO("memlink_do_cmd: %d\n", ret);
    if (ret <= 0) {
        return ret;
    }

    //char msglen;
    int  pos = CMD_REPLY_HEAD_LEN;

    /*memcpy(&msglen, retdata + pos, sizeof(char));
    pos += sizeof(char);
    if (msglen > 0) {
        pos += msglen;
    }*/
    
    memcpy(stat, retdata + pos, sizeof(MemLinkStat));
#ifdef DEBUG
    char buf[512] = {0};
    DINFO("stat buf: %s\n", formath((char *)stat, sizeof(MemLinkStat), buf, 512));

    DINFO("stat blocks:%d, data:%d, data_used:%d, mem:%d, valuesize:%d, masksize:%d\n", 
                    stat->blocks, stat->data, stat->data_used, 
                    stat->mem, stat->valuesize, stat->masksize);
    
#endif
    return MEMLINK_OK;
}

int 
memlink_cmd_stat_sys(MemLink *m, MemLinkStatSys *stat)
{
    char data[1024];
    int  len;

    len = cmd_stat_sys_pack(data);
    DINFO("pack stat sys len: %d\n", len);

    char retdata[1024];
    int ret = memlink_do_cmd(m, MEMLINK_READER, data, len, retdata, 1024);
    DINFO("memlink_do_cmd: %d\n", ret);
    if (ret <= 0) {
        return ret;
    }

    int  pos = CMD_REPLY_HEAD_LEN;
    memcpy(stat, retdata + pos, sizeof(MemLinkStatSys));
    return MEMLINK_OK;
}

int
memlink_cmd_create(MemLink *m, char *key, int valuelen, char *maskstr)
{
	if (NULL == key || strlen(key) > HASHTABLE_KEY_MAX)
		return MEMLINK_ERR_PARAM;
	if (valuelen <= 0 || valuelen > HASHTABLE_VALUE_MAX)
		return MEMLINK_ERR_PARAM;
	
    char data[1024];
    int  len;
    int  masknum = 0;
    unsigned int maskarray[128];   

    masknum = mask_string2array(maskstr, maskarray);
    DINFO("create masknum: %d\n", masknum);	
	if (masknum > HASHTABLE_MASK_MAX_ITEM)
		return MEMLINK_ERR_PARAM;
	int i = 0;
	for (i = 0; i < masknum; i++)
	{
		if (maskarray[i] > HASHTABLE_MASK_MAX_BIT)
		{
			DERROR("maskarray[%d]: %d\n", i, maskarray[i]);
			return MEMLINK_ERR_PARAM;
		}
	}
   
    len = cmd_create_pack(data, key, valuelen, masknum, maskarray);
    DINFO("pack create len: %d\n", len);

    //printh(data, len); 
    char retdata[1024];
    int ret = memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);
	if (ret < 0) 
		return ret;
	return MEMLINK_OK;

}

int
memlink_cmd_del(MemLink *m, char *key, char *value, int valuelen)
{
	if (NULL == key || strlen(key) > HASHTABLE_KEY_MAX)
		return MEMLINK_ERR_PARAM;
	if (valuelen <= 0 || valuelen > HASHTABLE_VALUE_MAX)
		return MEMLINK_ERR_PARAM;

    char data[1024];
    int  len;

    len = cmd_del_pack(data, key, value, valuelen);
    DINFO("pack del len: %d\n", len);
    
    unsigned short pkglen;
    unsigned char  cmd;

    memcpy(&pkglen, data, sizeof(short));
    memcpy(&cmd, data + sizeof(short), sizeof(char));

    DINFO("pkglen: %d, cmd: %d\n", pkglen, cmd);

    char retdata[1024];
    int ret = memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);

	if (ret < 0) 
		return ret;
	return MEMLINK_OK;
}

int
memlink_cmd_insert(MemLink *m, char *key, char *value, int valuelen, char *maskstr, int pos)
{
	if (NULL == key || strlen(key) > HASHTABLE_KEY_MAX)
		return MEMLINK_ERR_PARAM;
	if (valuelen <= 0 || valuelen > HASHTABLE_VALUE_MAX)
		return MEMLINK_ERR_PARAM;
	if (pos < -1)
		return MEMLINK_ERR_PARAM;
	
    char data[1024];
    int  len;
    unsigned int maskarray[128];
    int masknum = 0;

    masknum = mask_string2array(maskstr, maskarray);
    DINFO("insert mask len: %d\n", masknum);
	if (masknum < 0 || masknum > HASHTABLE_MASK_MAX_ITEM)
		return MEMLINK_ERR_PARAM;

    len = cmd_insert_pack(data, key, value, valuelen, masknum, maskarray, pos);
    DINFO("pack del len: %d\n", len);

    char retdata[1024];
    int ret = memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);

	if (ret < 0) 
		return ret;
	return MEMLINK_OK;
}

int 
memlink_cmd_update(MemLink *m, char *key, char *value, int valuelen, int pos)
{
	if (NULL == key || strlen(key) > HASHTABLE_KEY_MAX)
		return MEMLINK_ERR_PARAM;
	if (valuelen <= 0 || valuelen > HASHTABLE_VALUE_MAX)
		return MEMLINK_ERR_PARAM;
	if (pos < -1)
		return MEMLINK_ERR_PARAM;

    char data[1024];
    int  len;

    len = cmd_update_pack(data, key, value, valuelen, pos);
    DINFO("pack update len: %d\n", len);
    char retdata[1024];
    int ret = memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);

	if (ret < 0) 
		return ret;
	return MEMLINK_OK;
}


int
memlink_cmd_mask(MemLink *m, char *key, char *value, int valuelen, char *maskstr)
{
	if (NULL == key || strlen(key) > HASHTABLE_KEY_MAX)
		return MEMLINK_ERR_PARAM;
	if (valuelen <= 0 || valuelen > HASHTABLE_VALUE_MAX)
		return MEMLINK_ERR_PARAM;

    char data[1024];
    int  len;
    unsigned int maskarray[128];
    int masknum = 0;

    masknum = mask_string2array(maskstr, maskarray);
    DINFO("mask mask len: %d\n", masknum);
	if (masknum < 0 || masknum > HASHTABLE_MASK_MAX_ITEM)
		return MEMLINK_ERR_PARAM;
	
    DINFO("key: %s, valuelen: %d\n", key, valuelen);
    len = cmd_mask_pack(data, key, value, valuelen, masknum, maskarray);
    DINFO("pack mask len: %d\n", len);
    //printh(data, len);
    char retdata[1024];
    int ret = memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);

	if (ret < 0) 
		return ret;
	return MEMLINK_OK;
}

int
memlink_cmd_tag(MemLink *m, char *key, char *value, int valuelen, int tag)
{
	if (NULL == key || strlen(key) > HASHTABLE_KEY_MAX)
		return MEMLINK_ERR_PARAM;
	if (valuelen <= 0 || valuelen > HASHTABLE_VALUE_MAX)
		return MEMLINK_ERR_PARAM;

	if ( MEMLINK_TAG_DEL != tag && MEMLINK_TAG_RESTORE != tag )
		return MEMLINK_ERR_PARAM;
	
    char data[1024];
    int  len;
	
    len = cmd_tag_pack(data, key, value, valuelen, tag);
    DINFO("pack tag len: %d\n", len);
    char retdata[1024];
    int ret = memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);
	if (ret < 0) 
		return ret;
	return MEMLINK_OK;
}

int
memlink_cmd_rmkey(MemLink *m, char *key)
{
	if (NULL == key || strlen(key) > HASHTABLE_KEY_MAX)
		return MEMLINK_ERR_PARAM;

    char data[1024];
    int  len;

    len = cmd_rmkey_pack(data, key);
    DINFO("pack rmkey len: %d\n", len);
    char retdata[1024];
    int ret = memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);
	if (ret < 0) 
		return ret;
	return MEMLINK_OK;
}

int 
memlink_cmd_count(MemLink *m, char *key, char *maskstr, MemLinkCount *count)
{
	if (NULL == key || strlen(key) > HASHTABLE_KEY_MAX)
		return MEMLINK_ERR_PARAM;

	unsigned int maskarray[128];
    int masknum = 0;

    masknum = mask_string2array(maskstr, maskarray);
    DINFO("range mask len: %d\n", masknum);
	if (masknum < 0 || masknum > HASHTABLE_MASK_MAX_ITEM)
		return MEMLINK_ERR_PARAM;

    char data[1024];
    int  len;

    len = cmd_count_pack(data, key, masknum, maskarray);
    DINFO("pack rmkey len: %d\n", len);
    char retdata[1024];
    int ret = memlink_do_cmd(m, MEMLINK_READER, data, len, retdata, 1024);
	if (ret < 0) 
		return ret;

    //char msglen;
    int  pos = CMD_REPLY_HEAD_LEN;

	/*
    memcpy(&msglen, retdata + pos, sizeof(char));
    pos += sizeof(char);
    if (msglen > 0) {
        pos += msglen;
    }*/
    
    memcpy(count, retdata + pos, sizeof(MemLinkCount));
    DINFO("count, visible: %d, tag: %d\n", count->visible_count, count->tagdel_count);

    return MEMLINK_OK;
}

int 
memlink_cmd_range(MemLink *m, char *key, unsigned char kind,  char *maskstr, 
                  int frompos, int len, MemLinkResult *result)
{
	if (NULL == key || strlen(key) > HASHTABLE_KEY_MAX)
		return MEMLINK_ERR_PARAM;

	if(len <= 0 || frompos < 0)
		return MEMLINK_ERR_PARAM;

    char data[1024];
    int  plen;
    unsigned int maskarray[128];
    int maskn = 0;

    maskn = mask_string2array(maskstr, maskarray);
    DINFO("range mask len: %d\n", maskn);
	if (maskn < 0 || maskn > HASHTABLE_MASK_MAX_ITEM)
		return MEMLINK_ERR_PARAM;

    plen = cmd_range_pack(data, key, kind, maskn, maskarray, frompos, len);
    DINFO("pack range len: %d\n", plen);

    // len(4B) + retcode(2B) + valuesize(1B) + masksize(1B) + masknum(1B) + maskformat(masknum B) + value.mask * len
    int retlen = sizeof(int) + sizeof(short) + sizeof(char) + sizeof(char) + sizeof(char) + maskn * sizeof(int) + (HASHTABLE_VALUE_MAX + (HASHTABLE_MASK_MAX_BIT/8 + 2) * maskn) * len;
    DINFO("retlen: %d\n", retlen);
	if (retlen > 1024000) { // do not more than 1M
		return MEMLINK_ERR_RANGE_SIZE;
	}
    char retdata[retlen];

    int ret = memlink_do_cmd(m, MEMLINK_READER, data, plen, retdata, retlen);
    DINFO("memlink_do_cmd: %d\n", ret);
    if (ret <= 0) {
        DERROR("memlink_do_cmd error: %d\n", ret);
        return ret;
    }
    unsigned int dlen;
    memcpy(&dlen, retdata, sizeof(int));
	//dlen = *(unsigned int*)(retdata);

    int  pos = CMD_REPLY_HEAD_LEN;

    unsigned char valuesize, masksize, masknum;
    unsigned char maskformat[1024];
	valuesize = *(char*)(retdata + pos);
    pos += sizeof(char);
	masksize = *(char*)(retdata + pos);
    pos += sizeof(char);
	masknum = *(char*)(retdata + pos);
    pos += sizeof(char);
    memcpy(maskformat, retdata + pos, masknum);
    pos += masknum;

    DINFO("valuesize: %d, masksize: %d, pos:%d, ret:%d\n", valuesize, masksize, pos, ret);
    char *vdata   = retdata + pos;
    char *enddata = retdata + dlen + sizeof(int);
#ifdef DEBUG
    int  datalen  = valuesize + masksize;

    DINFO("vdata: %p, enddata: %p, datalen: %d\n", vdata, enddata, datalen);
    char buf1[128], buf2[128];
#endif
    //MemLinkResult   *mret;
    MemLinkItem     *root = NULL, *cur = NULL;

    int count = 0;

    while (vdata < enddata) {
        DINFO("value:%s, mask:%s\n", formath(vdata, valuesize, buf1, 128), 
                            formath(vdata + valuesize, masksize, buf2, 128));

        MemLinkItem     *item;
        item = (MemLinkItem*)zz_malloc(sizeof(MemLinkItem));
        if (NULL == item) {
            MemLinkItem *tmp;

            while (root) {
                tmp = root;
                root = root->next;
                zz_free(tmp);
            }
            return MEMLINK_ERR_CLIENT;
        }
        memset(item, 0, sizeof(MemLinkItem));
        
        if (root == NULL) {
            root = item;
        }

        if (cur == NULL) {
            cur = item;
        }else{
            cur->next = item;
            cur = item;
        }
       
        memcpy(item->value, vdata, valuesize);
        //item->value[valuesize] = 0;
#ifdef RANGE_MASK_STR
        unsigned char mlen;
        memcpy(&mlen, vdata + valuesize, sizeof(char));
        memcpy(item->mask, vdata + valuesize + sizeof(char), mlen);
        //item->mask[mlen] = 0;
        //vdata += datalen;
		vdata += valuesize + sizeof(char) + mlen;
#else
        //int mlen = mask_binary2string(maskformat, masknum, vdata + valuesize, masksize, item->mask);
        mask_binary2string(maskformat, masknum, vdata + valuesize, masksize, item->mask);
        vdata += valuesize + masksize;
#endif
        count += 1;
    }
    result->count     = count;
    result->valuesize = valuesize;
    result->masksize  = masksize;
    result->root      = root;

    return MEMLINK_OK;
}

int
memlink_cmd_insert_mvalue(MemLink *m, char *key, MemLinkInsertVal *values, int num)
{
	if (NULL == key || strlen(key) > HASHTABLE_KEY_MAX)
		return MEMLINK_ERR_PARAM;

    int  len;
    int  pkgsize = sizeof(int) + sizeof(char) + 256;
    int  i;
    char *data;

    MemLinkInsertVal    *item; 
    for (i = 0; i < num; i++) {
        item = &values[i];
        item->masknum = mask_string2array(item->maskstr, item->maskarray);
        pkgsize += item->valuelen + sizeof(char) + item->masknum * sizeof(int) + sizeof(char) + sizeof(int);
    }
    
    if (pkgsize > 1024000) {
        data = (char *)zz_malloc(pkgsize); 
        if (NULL == data) {
            DERROR("malloc pkgsize:%d error!\n", pkgsize);
            MEMLINK_EXIT;
        }
    }else{
        data = (char *)alloca(pkgsize);
    }


    len = cmd_insert_mvalue_pack(data, key, values, num);
    DINFO("pack del len: %d\n", len);

    char retdata[1024];
    int ret = memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);

    if (pkgsize > 1024000) {
        zz_free(data);
    }

	if (ret < 0) 
		return ret;
	return MEMLINK_OK;
}

int 
memlink_cmd_lpush(MemLink *m, char *key, char *value, int valuelen, char *maskstr)
{
    if (NULL == key || strlen(key) > HASHTABLE_KEY_MAX)
		return MEMLINK_ERR_PARAM;
	if (valuelen <= 0 || valuelen > HASHTABLE_VALUE_MAX)
		return MEMLINK_ERR_PARAM;
	
    char data[1024];
    int  len;
    unsigned int maskarray[128];
    int masknum = 0;

    masknum = mask_string2array(maskstr, maskarray);
    DINFO("insert mask len: %d\n", masknum);
	if (masknum < 0 || masknum > HASHTABLE_MASK_MAX_ITEM)
		return MEMLINK_ERR_PARAM;

    len = cmd_lpush_pack(data, key, value, valuelen, masknum, maskarray);
    DINFO("pack lpush len: %d\n", len);

    char retdata[1024];
    int ret = memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);

	if (ret < 0) 
		return ret;
	return MEMLINK_OK;
}

int 
memlink_cmd_rpush(MemLink *m, char *key, char *value, int valuelen, char *maskstr)
{
    if (NULL == key || strlen(key) > HASHTABLE_KEY_MAX)
		return MEMLINK_ERR_PARAM;
	if (valuelen <= 0 || valuelen > HASHTABLE_VALUE_MAX)
		return MEMLINK_ERR_PARAM;
	
    char data[1024];
    int  len;
    unsigned int maskarray[128];
    int masknum = 0;

    masknum = mask_string2array(maskstr, maskarray);
    DINFO("insert mask len: %d\n", masknum);
	if (masknum < 0 || masknum > HASHTABLE_MASK_MAX_ITEM)
		return MEMLINK_ERR_PARAM;

    len = cmd_rpush_pack(data, key, value, valuelen, masknum, maskarray);
    DINFO("pack rpush len: %d\n", len);

    char retdata[1024];
    int ret = memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);

	if (ret < 0) 
		return ret;
	return MEMLINK_OK;

}

int 
memlink_cmd_lpop(MemLink *m, char *key, int num, MemLinkResult *result)
{
    if (NULL == key || strlen(key) > HASHTABLE_KEY_MAX)
		return MEMLINK_ERR_PARAM;
	
    char data[1024];
    int  len;

    len = cmd_lpop_pack(data, key, num);
    DINFO("pack rpush len: %d\n", len);

    // len(4B) + retcode(2B) + valuesize(1B) + masksize(1B) + masknum(1B) + maskformat(masknum B) + value.mask * len
    char retdata[1024];
    int ret = memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);

	if (ret < 0) 
		return ret;
	return MEMLINK_OK;
}

int 
memlink_cmd_rpop(MemLink *m, char *key, int num, MemLinkResult *result)
{
    return MEMLINK_OK;
}


void
memlink_close(MemLink *m)
{
    if (m->readfd > 0) {
        close(m->readfd);
        m->readfd = 0;
    }
    if (m->writefd > 0) {
        close(m->writefd);
        m->writefd = 0;
    }
}


void
memlink_destroy(MemLink *m)
{
    memlink_close(m);

    zz_free(m);
}


void
memlink_result_free(MemLinkResult *result)
{
    MemLinkItem *item, *tmp;

    DINFO("free result ...\n");
    if (NULL == result)
        return;

    item = result->root;
    
    while (item) {
        tmp = item;
        item = item->next;
        zz_free(tmp);
    }

    result->root = NULL;
}

//add by lanwenhong
int
memlink_cmd_del_by_mask(MemLink *m, char *key, char *maskstr)
{
	if (NULL == key || strlen(key) > HASHTABLE_KEY_MAX)
		return MEMLINK_ERR_PARAM;

	char data[1024] = {0};
	char retdata[1024];
	unsigned int maskarray[128] = {0};
	int masknum;
	int len;
	int ret;

	masknum = mask_string2array(maskstr, maskarray);
	DINFO("masknum: %d\n", masknum);
	if (masknum < 0 || masknum > HASHTABLE_MASK_MAX_ITEM)
		return MEMLINK_ERR_PARAM;
	
	len = cmd_del_by_mask_pack(data, key, maskarray, masknum);
	DINFO("pack del by mask len: %d\n", len);

	ret = memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);
	if (ret < 0)
		return ret;

	return MEMLINK_OK;

}

