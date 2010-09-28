#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include "zzmalloc.h"
#include "logfile.h"
#include "hashtable.h"
#include "serial.h"
#include "utils.h"


/**
 * 检查数据项中是否有数据
 * 真实删除位为0表示无效数据，为1表示存在有效数据
 */
static int 
dataitem_have_data(HashNode *node, char *itemdata)
{
    char *maskdata  = itemdata + node->valuesize;

    //char buf[128];
    //DINFO("check mask have data: %s, %d\n", formatb(maskdata, node->masksize, buf, 128), *maskdata & (unsigned char)0x01);
    //printh(itemdata + node->valuesize, node->masksize);

    if ((*maskdata & (unsigned char)0x01) == 1) {
        //DINFO("ok, have data.\n");
        return 1;
    }
    return 0;
}

/**
 * 检查数据项中是否有要显示的数据
 * 标记删除位为1表示数据标记为删除
 */
static int 
dataitem_have_visiable_data(char *itemdata, int valuelen)
{
    unsigned char n = *(itemdata + valuelen);
    if ((n & (unsigned char)0x03) == 0x01)
        return 1;
    return 0;
}

/**
 * 在数据链的一块中查找某一条数据
 */
static char*
dataitem_lookup(HashNode *node, void *value)
{
    int i;
    int datalen = node->valuesize + node->masksize;
    DataBlock *root = node->data;

    while (root) {
        char *data = root->data;
        DINFO("data: %p\n", data);
        for (i = 0; i < g_cf->block_data_count; i++) {
            if (memcmp(value, data, node->valuesize) == 0) {
                return data;
            }
            data += datalen;
        }
        root = root->next;
    }
    return NULL;
}

/**
 * 复制一条数据的value, mask到指定地址
 * @param node 该数据key的HashNode
 * @param addr 要复制到的目标地址
 * @param value 要复制的value
 * @param mask  要复制的mask。为二进制的存储格式。
 */
static int
dataitem_copy(HashNode *node, char *addr, void *value, void *mask)
{
    //char *m = mask;
    //*m = *m | (*addr & 0x03);

    DINFO("dataitem_copy valuesize: %d, masksize: %d\n", node->valuesize, node->masksize);
    memcpy(addr, value, node->valuesize);
    memcpy(addr + node->valuesize, mask, node->masksize);
    
    return 0;
}

static int
dataitem_copy_mask(HashNode *node, char *addr, char *maskflag, char *mask)
{
    //char *m = mask;
    //*m = *m | (*addr & 0x03);
    int i;
    char *maskaddr = addr + node->valuesize;
    char buf[128];
    DINFO("copy_mask before: %s\n", formatb(addr, node->masksize, buf, 128)); 

    for (i = 0; i < node->masksize; i++) {
        maskaddr[i] = (maskaddr[i] & maskflag[i]) | mask[i];
    }
    //memcpy(addr + node->valuesize, mask, node->masksize);
    DINFO("copy_mask after: %s\n", formatb(addr, node->masksize, buf, 128)); 

    return 0;
}

/**
 * 创建HashTable
 */
HashTable*
hashtable_create()
{
    HashTable *ht;

    ht = (HashTable*)zz_malloc(sizeof(HashTable));
    if (NULL == ht) {
        DERROR("malloc HashTable error! %s\n", strerror(errno));
        return NULL;
    }
    memset(ht, 0, sizeof(HashTable));    

    return ht;
}

/**
 * 释放HashTable占用的资源
 */
void
hashtable_destroy(HashTable *ht)
{
    if (NULL == ht)
        return;

    int i;

    for (i = 0; i < HASHTABLE_BUNKNUM; i++) {
        if (ht->bunks[i] != NULL) {
            DataBlock   *dbk = ht->bunks[i]->data;
            DataBlock   *tmp; 
            while (dbk) {
                tmp = dbk;
                dbk = dbk->next;
                zz_free(tmp);
            }
        }
        zz_free(ht->bunks[i]->key);
        zz_free(ht->bunks[i]->maskformat);
    }

    zz_free(ht);
}

/**
 * 创建HashTable中一个key的结构信息
 */
int
hashtable_add_info(HashTable *ht, char *key, int valuesize, char *maskstr)
{
    unsigned int format[HASHTABLE_MASK_MAX_LEN] = {0};
    int  masknum = mask_string2array(maskstr, format);
    int  i; 

    for (i = 0; i < masknum; i++) {
        if (format[i] == UINT_MAX) {
            DERROR("maskformat error: %s\n", maskstr);
            return -3;
        }
    }

    return hashtable_add_info_mask(ht, key, valuesize, format, masknum);
}

int
hashtable_add_info_mask(HashTable *ht, char *key, int valuesize, unsigned int *maskarray, char masknum)
{
    int             keylen = strlen(key);
    unsigned int    hash   = hashtable_hash(key, keylen);
    HashNode        *node  = ht->bunks[hash];
  
    DINFO("hashtable_add_info_mask call ... %s, %p, hash: %d\n", key, node, hash);
    // handle hash collisions
    while (node) {
        DINFO("node->key: %p\n", node);
        //DINFO("node->key: %p, valuesize: %d\n", node, node->valuesize);
        if (strcmp(node->key, key) == 0) {
            DWARNING("key %s exists.\n", key);
            return -1;
        }
        node = node->next;
    }
    
    node = (HashNode*)zz_malloc(sizeof(HashNode));
    if (NULL == node) {
        DERROR("malloc HashNode error!\n");
        return -2;
    }
    memset(node, 0, sizeof(HashNode));
    
    node->key  = zz_strdup(key);
    node->data = NULL;
    node->next = ht->bunks[hash];
    node->valuesize = valuesize;

    int masksize = 0;
    int i;
    for (i = 0; i < masknum; i++) {
        masksize += maskarray[i];
    }

    masksize += 2; // add tag 1 bit, clear flag 1 bit
    node->masksize = masksize / 8;

    if (masksize % 8 > 0) {
        node->masksize += 1;
    }
    node->masknum = masknum;

    if (masknum > 0) {
        node->maskformat = (unsigned char *)zz_malloc(i);
        if (NULL == node->maskformat) {
            DERROR("malloc maskformat error!\n");
            MEMLINK_EXIT;
        }
        for (i = 0; i < node->masknum; i++) {
            node->maskformat[i] = maskarray[i];
        }
    }
    DINFO("node key: %s, format: %d,%d,%d\n", node->key, node->maskformat[0], node->maskformat[1], node->maskformat[2]);
    ht->bunks[hash] = node;
    DINFO("ht key:%s, hash:%d\n", ht->bunks[hash]->key, hash);

    return 0;    
}


/**
 * 通过key找到一个HashNode
 */
HashNode*
hashtable_find(HashTable *ht, char *key)
{
    int             keylen = strlen(key);
    unsigned int    hash   = hashtable_hash(key, keylen);
    HashNode        *node  = ht->bunks[hash];

    while (node) {
        if (strcmp(key, node->key) == 0) {
            return node;
        }
        node = node->next;
    }

    return NULL;
}

/**
 * hash函数
 */
unsigned int
hashtable_hash(char *str, int len)
{
    //BKDR Hash
    unsigned int seed = 131;
    unsigned int hash = 0;
    int i;

    for (i = 0; i < len; i++) {   
        hash = hash * seed + (*str++);
    }   

    return (hash & 0x7FFFFFFF) % HASHTABLE_BUNKNUM;
}

int
hashtable_add_first(HashTable *ht, char *key, void *value, char *maskstr)
{
    unsigned int maskarray[HASHTABLE_MASK_MAX_LEN];
    char masknum;

    masknum = mask_string2array(maskstr, maskarray);
    if (masknum <= 0) {
        DERROR("mask_string2array error: %s\n", maskstr);
        return -1;
    }

    return hashtable_add_first_mask(ht, key, value, maskarray, masknum);
}

/**
 * 在数据链的头部添加数据
 */
int 
hashtable_add_first_mask(HashTable *ht, char *key, void *value, unsigned int *maskarray, char masknum)
{
    char mask[HASHTABLE_MASK_MAX_LEN * sizeof(int)];
    int  ret;
    HashNode *node = hashtable_find(ht, key);
    if (NULL == node) {
        DERROR("hashtable_add not found node\n");
        return -1;
    }
    
    ret = mask_array2binary(node->maskformat, maskarray, masknum, mask);
    if (ret <= 0) {
        DERROR("mask_array2binary error: %d\n", ret);
        return -2;
    }

    DataBlock *dbk = node->data;
    //int count = 0;
    int datalen = node->valuesize + node->masksize;
    int i;
    int idle = 0;
    char *first_idle = NULL;
    char *itemdata = dbk->data;

    // find idle space before first item
    for (i = 0; i < g_cf->block_data_count; i++) {
        if (dataitem_have_data(node, itemdata) == 0) {
            if (first_idle == NULL) {
                first_idle = itemdata;
            }
            idle = 0;
        }
        itemdata += datalen;
    }

    if (first_idle) { // found, copy data
        dataitem_copy(node, first_idle, value, mask);
        node->used++;
    }else{ // create new datablock
        DataBlock *newbk = mempool_get(g_mpool, datalen);
        if (NULL == newbk) {
            DERROR("hashtable_add get new DataBlock error!\n");
            return 500;
        }

        newbk->next = dbk; 
        newbk->count = 1;
        itemdata = newbk->data + (g_cf->block_data_count - 1) * datalen;

        dataitem_copy(node, itemdata, value, mask);

        node->data = newbk;
    }

    return 0;
}

int
hashtable_add(HashTable *ht, char *key, void *value, char *maskstr, int pos)
{
    unsigned int maskarray[HASHTABLE_MASK_MAX_LEN];
    char masknum;

    masknum = mask_string2array(maskstr, maskarray);
    if (masknum <= 0) {
        DERROR("mask_string2array error: %s\n", maskstr);
        return -1;
    }

    return hashtable_add_mask(ht, key, value, maskarray, masknum, pos);
}

int
hashtable_add_mask_bin(HashTable *ht, char *key, void *value, void *mask, int pos)
{
    HashNode *node = hashtable_find(ht, key);
    if (NULL == node) {
        DERROR("hashtable_add not found node\n");
        return -1;
    }
    
    DataBlock *dbk = node->data;
    int count = 0;
    int datalen = node->valuesize + node->masksize;
    int i;
    char *posaddr  = NULL;
    char *itemdata;
    DataBlock *last = NULL;

    // find copy position
    while (dbk) {
        itemdata = dbk->data;

        for (i = 0; i < g_cf->block_data_count; i++) {
            if (dataitem_have_data(node, itemdata)) {
                if (count == pos) {
                    posaddr = itemdata;
                    break;
                }
                count += 1;
            }
            itemdata += datalen;
        }

        if (posaddr != NULL) {
            break;
        }

        last = dbk;
        dbk  = dbk->next;
    }

    // create new block for copy item
    DataBlock *newbk = mempool_get(g_mpool, datalen);
    if (NULL == newbk) {
        DERROR("hashtable_add get new DataBlock error!\n");
        MEMLINK_EXIT;
        return -2;
    }
    DINFO("create newbk:%p, dbk:%p\n", newbk, dbk);

    node->all += g_cf->block_data_count;

    if (posaddr == NULL) { // position out of link, only create a new block
        DINFO("not found the same hash, create new\n");
        newbk->count = 1;
        newbk->next  = NULL;

        dataitem_copy(node, newbk->data, value, mask);
        //node->used++;
        if (last) {
            last->next = newbk;
        }else{
            node->data = newbk;
        }
    }else{ //
        char *todata  = newbk->data;
        char *enddata = todata + g_cf->block_data_count * datalen;
        itemdata    = dbk->data;
        newbk->next = dbk->next;

        DINFO("found exist hash, newbk: %p, dbk: %p, todata:%p, enddata:%p\n", newbk, dbk, todata, enddata);

        DataBlock   *lastnewbk = newbk;
        for (i = 0; i < g_cf->block_data_count; i++) {
            char buf[128] = {0};
            if (dataitem_have_data(node, itemdata)) { 
                if (itemdata == posaddr) { // position for insert
                    DINFO("insert pos, go\n");
                    dataitem_copy(node, todata, value, mask);
                    lastnewbk->count ++;
                    todata += datalen; 
        
                    //hashtable_print(ht, key);
                    DINFO("data1 %p ... %s\n", itemdata, formatb(itemdata + node->valuesize, node->masksize, buf, 128) );
                    DINFO("data1 %p ... %s\n", itemdata, formatb(itemdata + datalen + node->valuesize, node->masksize, buf, 128) );
                }

                if (todata >= enddata) { // at the end of new datablock, must create another datablock
                    DINFO("create a new datablock.\n");
                    DataBlock *newbk2 = mempool_get(g_mpool, datalen);
                    if (NULL == newbk2) {
                        DERROR("hashtable_add error!\n");
                        //mempool_put(g_mpool, newbk, datalen);
                        MEMLINK_EXIT;
                        return -3;
                    }
                    newbk2->next = newbk->next;
                    newbk->next  = newbk2;

                    lastnewbk = newbk2;

                    node->all += g_cf->block_data_count;

                    todata  = newbk2->data;
                    enddata = todata + g_cf->block_data_count * datalen;
                }
                // only copy data
                    
                //DINFO("data2 %p ... %s\n", itemdata, formatb(itemdata + node->valuesize, node->masksize, buf, 128) );

                DINFO("copy data ...  %s\n", formatb(itemdata + node->valuesize, node->masksize, buf, 128) );
                memcpy(todata, itemdata, datalen);
                lastnewbk->count ++;
                todata += datalen; 
            }
            itemdata += datalen;
        }
        if (last) {
            last->next = newbk;
        }else{
            node->data = newbk;
        }
        DINFO("dbk %p put back to mempool.\n", dbk);
        mempool_put(g_runtime->mpool, dbk, datalen);
        node->all -= g_cf->block_data_count;
        //node->used -= dbk->count;
    }
   
    node->used += 1;

    return 0;
}

/**
 * 在数据链中任意指定位置添加数据
 */
int
hashtable_add_mask(HashTable *ht, char *key, void *value, unsigned int *maskarray, char masknum, int pos)
{
    char mask[HASHTABLE_MASK_MAX_LEN * sizeof(int)] = {0};
    int  ret;
    HashNode *node = hashtable_find(ht, key);
    if (NULL == node) {
        DERROR("hashtable_add not found node\n");
        return -1;
    }
    
    ret = mask_array2binary(node->maskformat, maskarray, masknum, mask);
    if (ret <= 0) {
        DERROR("mask_array2binary error: %d\n", ret);
        return -2;
    }

    //printh(mask, node->masksize);

    return hashtable_add_mask_bin(ht, key, value, mask, pos);
}

/**
 * 把数据链中的某一条数据移动到链头部
 */
int
hashtable_update(HashTable *ht, char *key, void *value, int pos)
{
    char mask[HASHTABLE_MASK_MAX_LEN * sizeof(int)] = {0};
    HashNode *node = NULL;
    char     *item = NULL; 

    char ret = hashtable_find_value(ht, key, value, &node, &item);
    if (ret < 0) {
        DWARNING("not found value: %d, %s\n", ret, key);
        return -1;
    }

    memcpy(mask, item + node->valuesize, node->masksize);  
    //FIXME: mask is binary or string ?
    return hashtable_add_mask_bin(ht, key, value, mask, pos);
}

/**
 * 通过key, value，在HashTable中找到一条数据的地址，同时返回对应的HashNode
 */
int
hashtable_find_value(HashTable *ht, char *key, void *value, HashNode **node, char **data)
{
    HashNode *fnode = hashtable_find(ht, key);

    if (NULL == fnode) {
        DWARNING("hashtable_del error: %s\n", key);
        return -1;
    }
    if (node) {
        *node = fnode;
    }

    DINFO("find dataitem ...\n");
    char *item = dataitem_lookup(fnode, value);
    if (NULL == item) {
        DWARNING("dataitem_lookup error: %s, %x\n", key, *(unsigned int*)value);
        return -2;
    }
    *data = item;

    return 0; 
}

/**
 * 在HashTable中删除一条数据。只是把真实删除标记位置1
 */
int 
hashtable_del(HashTable *ht, char *key, void *value)
{
    char        *item = NULL;
    HashNode    *node = NULL;

    DINFO("hashtable_find_value: %s, value: %s\n", key, (char*)value);
    int ret = hashtable_find_value(ht, key, value, &node, &item);
    DINFO("hashtable_find_value ret: %d\n", ret);
    if (ret < 0) {
        return ret;
    }
    *(item + node->valuesize) |= 0x01; 

    return 0;
}

/**
 * 标记删除/标记恢复。只是把对应的标记删除位置1/0
 */
int
hashtable_tag(HashTable *ht, char *key, void *value, unsigned char tag)
{
    char        *item = NULL;
    HashNode    *node = NULL;

    int ret = hashtable_find_value(ht, key, value, &node, &item);
    if (ret < 0) {
        DERROR("not found key: %s\n", key);
        return ret;
    }
    DINFO("tag: %d, %d\n", tag, tag<<1);
    *(item + node->valuesize) |= tag << 1; 

    return 0;
}

/**
 * 修改一条数据的mask值
 */
int
hashtable_mask(HashTable *ht, char *key, void *value, unsigned int *maskarray, int masknum)
{
    char        *item = NULL;
    HashNode    *node = NULL;

    int  ret = hashtable_find_value(ht, key, value, &node, &item);
    if (ret < 0) {
        return ret;
    }

    char mask[HASHTABLE_MASK_MAX_LEN * sizeof(int)] = {0};
    char maskflag[HASHTABLE_MASK_MAX_LEN * sizeof(int)] = {0};

    int  len = mask_array2binary(node->maskformat, maskarray, masknum, mask);
    if (len <= 0) {
        DERROR("mask array2binary error: %d\n", masknum);
        return -1;
    }
    char buf[128];
    DINFO("array2bin: %s\n", formatb(mask, len, buf, 128));

    int flen = mask_array2flag(node->maskformat, maskarray, masknum, maskflag);
    if (flen <= 0) {
        DERROR("mask array2flag error: %d\n", masknum);
        return -1;
    }
    DINFO("array2flag: %s\n", formatb(maskflag, flen, buf, 128));

    dataitem_copy_mask(node, item, maskflag, mask);

    return 0;
}

int
hashtable_range(HashTable *ht, char *key, unsigned int *maskarray, int masknum, int frompos, int len, 
                char *data, int *datanum, unsigned char *valuesize, unsigned char *masksize)
{
    char mask[HASHTABLE_MASK_MAX_LEN * sizeof(int)];
    //char masknum;
    HashNode    *node;
    int         masklen = 0;

    node = hashtable_find(ht, key);
    if (NULL == node) {
        DERROR("hashtable_add not found node\n");
        return -1;
    }

    //masknum = mask_string2binary(node->maskformat, maskstr, mask);
    if (masknum > 0) {
        masklen = mask_array2binary(node->maskformat, maskarray, masknum, mask);
        if (masklen <= 0) {
            DERROR("mask_string2array error\n");
            return -2;
        }

        mask[0] = mask[0] & 0xfc;
    }

    DataBlock *dbk = node->data;
    int n = 0; 
    int datalen = node->valuesize + node->masksize;

    *valuesize = node->valuesize;
    *masksize  = node->masksize;

    int endpos = frompos + len;
    int idx = 0;

    if (masknum > 0) {
        while (dbk) {
            n += dbk->count;
            if (n > frompos) {
                n -= dbk->count;
                break;
            }else if (n == frompos) {
                dbk = dbk->next;
                break;
            }

            dbk = dbk->next;
        }
    }

    while (dbk) {
        char *itemdata = dbk->data;
        int  i;
        char buf[128];
        for (i = 0; i < g_cf->block_data_count; i++) {
            if (dataitem_have_visiable_data(itemdata, node->valuesize)) {
                if (n >= frompos && n < endpos) {
                    DINFO("ok, copy item ...%s\n", formath(itemdata, datalen, buf, 128));
                    memcpy(data + idx, itemdata, datalen);
                    idx += datalen;
                }
                n++;

                if (n >= endpos) {
                    goto hashtable_range_over;
                }
            }
            itemdata += datalen;
        }
        dbk = dbk->next;
    }

hashtable_range_over:
    //*data = retv;
    *datanum = idx;

    return 0;
}

int
hashtable_clean(HashTable *ht, char *key)
{
    HashNode    *node = hashtable_find(ht, key); 
    if (NULL == node)
        return 0;

    DataBlock   *dbk = node->data;
    int         dlen = node->valuesize + node->masksize;

    if (NULL == dbk)
        return 0;

    if (node->used == 0) {
        DataBlock *tmp;
        while (dbk) {
            tmp = dbk; 
            dbk = dbk->next;
            mempool_put(g_runtime->mpool, dbk, dlen);
        }
        node->all  = 0;
        node->data = NULL;
        return 0;
    }

    DataBlock   *newdbk = NULL, *newroot = NULL, *newlast = NULL;
    char        *newdbk_end = NULL, *newdbk_pos = NULL;
    char        *itemdata = dbk->data;
    int         i;

    newdbk = mempool_get(g_runtime->mpool, dlen);
    newdbk_end = newdbk->data + g_cf->block_data_count * dlen;
    newdbk_pos = newdbk->data;

    newroot = newdbk;
    //newlast = newdbk;

    while (dbk) {
        for (i = 0; i < g_cf->block_data_count; i++) {
            if (dataitem_have_data(node, itemdata)) {
                memcpy(newdbk_pos, itemdata, dlen);
                newdbk_pos += dlen;
                if (newdbk_pos >= newdbk_end) {
                    newdbk = mempool_get(g_runtime->mpool, dlen);
                    newdbk_end = newdbk->data + g_cf->block_data_count * dlen;
                    newdbk_pos = newdbk->data;
                    
                    if (newlast != NULL) {
                        newlast->next = newdbk;
                    }else{
                        newlast = newdbk;
                    }
                   
                }
            }
            itemdata += dlen;
        }
        dbk = dbk->next;
    }

    node->data = newroot;

    return 0;
} 

int
hashtable_stat(HashTable *ht, char *key, HashTableStat *stat)
{
    HashNode    *node;

    node = hashtable_find(ht, key);
    if (NULL == node) {
        DERROR("hashtable_stat not found key: %s\n", key);
        return -1;
    }

    DINFO("node all: %d, used: %d\n", node->all, node->used);
    stat->valuesize = node->valuesize;
    stat->masksize  = node->masksize;
    stat->blocks    = 0;
    stat->data      = node->all;
    stat->data_used = node->used;
    stat->mem       = 0;
    stat->mem_used  = 0;

    int blockmem = sizeof(HashNode) + (node->masksize + node->valuesize) * g_cf->block_data_count;

    DataBlock *dbk = node->data;
    while (dbk) {
        stat->blocks += 1;
        dbk = dbk->next;
    }

    stat->mem = stat->blocks * blockmem;
    stat->mem_used = stat->blocks * sizeof(HashNode) + (node->masksize + node->valuesize) * stat->data_used;

    return 0;
}

int 
hashtable_print(HashTable *ht, char *key)
{
    HashNode    *node;

    node = hashtable_find(ht, key);
    if (NULL == node) {
        DERROR("not found key: %s\n", key);
        return -1;
    }

    DINFO("------ HashNode key:%s, vsize:%d, msize:%d, mnum:%d, all:%d, userd:%d\n",
                node->key, node->valuesize, node->masksize, node->masknum, node->all, node->used);

    int i;

    for (i = 0; i < node->masknum; i++) {
        DINFO("maskformat: %d, %d\n", i, node->maskformat[i]);
    }

    DataBlock *dbk = node->data;

    int blocks = 0;
    int datalen = node->valuesize + node->masksize;
    char buf1[128], buf2[128];

    while (dbk) {
        blocks += 1;
        char *itemdata = dbk->data; 
        DINFO("====== dbk %p count: %d ======\n", dbk, dbk->count);
        for (i = 0; i < g_cf->block_data_count; i++) {
            if (dataitem_have_data(node, itemdata)) {
                DINFO("i: %d, value: %s, mask: %s\n", i, formath(itemdata, node->valuesize, buf2, 128), 
                            formath(itemdata + node->valuesize, node->masksize, buf1, 128));
            }else{
                DINFO("i: %d, no data, mask: %s\n", i, formath(itemdata + node->valuesize, node->masksize, buf1, 128));
            }

            itemdata += datalen;
        }

        dbk = dbk->next;
    }
    DINFO("------ blocks: %d\n", blocks);

    return 0;
}
