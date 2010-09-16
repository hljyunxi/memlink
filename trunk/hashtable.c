#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include "zzmalloc.h"
#include "logfile.h"
#include "hashtable.h"


/**
 * 把字符串形式的mask转换为数组形式
 */
static int 
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

static int
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
        /*if (b >= sizeof(char)) {
            idx += 1;
            b = b % sizeof(char);
        }*/
        mf = maskformat[i];
        v  = maskarray[i]; 
        if (v == UINT_MAX) {
            b += maskformat[i];
            continue;
        }
        
        unsigned char y = (b + mf) % sizeof(char);

        n = (b + mf) / sizeof(char) + y>0 ? 1: 0;
        v = v << b;
       
        unsigned char m = pow(2, b) - 1;
        unsigned char x = mask[idx] & m;

        v = v | x;

        m = (pow(2, sizeof(char) - y) - 1);
        m = m << y;
        x = mask[idx + n - 1] & m;

        memcpy(&mask[idx], &v, n);

        idx += n - 1;
        
        mask[idx] = mask[idx] | x;

        b = y;
    }

    return idx; 
}

static int
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

/**
 * 检查数据项中是否有数据
 */
static int 
dataitem_have_data(char *itemdata, int valuelen)
{
    unsigned char n = *(itemdata + valuelen);
    if ((n & (unsigned char)0x01) == 0)
        return 1;
    return 0;
}

/**
 * 检查数据项中是否有要显示的数据
 */
static int 
dataitem_have_visiable_data(char *itemdata, int valuelen)
{
    unsigned char n = *(itemdata + valuelen);
    if ((n & (unsigned char)0x03) == 0)
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
    char *m = mask;
    
    *m = *m | (*addr & 0x03);

    if (mask) {
        memcpy(addr + node->valuesize, mask, node->masksize);
    }else{
        memset(addr + node->valuesize, 0, node->masksize);
    }
    memcpy(node, value, node->valuesize);
    
    return 0;
}

static int
dataitem_copy_mask(HashNode *node, char *addr, void *mask)
{
    char *m = mask;
    
    *m = *m | (*addr & 0x03);

    if (mask) {
        memcpy(addr + node->valuesize, mask, node->masksize);
    }else{
        memset(addr + node->valuesize, 0, node->masksize);
    }
    
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
    int         keylen = strlen(key);
    unsigned    hash   = hashtable_hash(key, keylen);
    HashNode    *node  = ht->bunks[hash];
    
    while (node) {
        if (strcmp(node->key, key) == 0) {
            // find it
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
    node->masksize = masksize / sizeof(char);

    if (masksize % sizeof(char) > 0) {
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
    

    ht->bunks[hash] = node;

    return 0;    
}


/**
 * 通过key找到一个HashNode
 */
HashNode*
hashtable_find(HashTable *ht, char *key)
{
    int         keylen = strlen(key);
    unsigned    hash   = hashtable_hash(key, keylen);
    HashNode    *node  = ht->bunks[hash];

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
        DERROR("mask_array2binary error.\n");
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
        if (dataitem_have_data(itemdata, node->valuesize) == 0) {
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
    DataBlock *last = dbk;

    // find copy position
    while (dbk) {
        itemdata = dbk->data;

        for (i = 0; i < g_cf->block_data_count; i++) {
            if (dataitem_have_data(itemdata, node->valuesize)) {
                if (count == pos) {
                    posaddr = itemdata;
                    break;
                }
                count += 1;
            }
            itemdata += datalen;
        }
        last = dbk;
        dbk  = dbk->next;
    }

    // create new block for copy item
    DataBlock *newbk = mempool_get(g_mpool, datalen);
    if (NULL == newbk) {
        DERROR("hashtable_add get new DataBlock error!\n");
        MEMLINK_EXIT;
        return 500;
    }

    if (posaddr == NULL) { // position out of link, only create one new block
        newbk->count = 1;
        newbk->next = NULL;

        dataitem_copy(node, newbk->data, value, mask);
        node->used++;
        last->next = newbk;
    }else{ //
        char *todata = newbk->data;
        char *enddata = todata + g_cf->block_data_count * datalen;

        itemdata = dbk->data;
        newbk->next = dbk->next;

        for (i = 0; i < g_cf->block_data_count; i++) {
            if (dataitem_have_data(itemdata, node->valuesize)) {
                if (itemdata == posaddr) { // position for insert
                    dataitem_copy(node, todata, value, mask);
                    todata += datalen; 
                }
                // only copy data
                if (todata >= enddata) { // at the end of new datablock, must create another datablock
                    DataBlock *newbk2 = mempool_get(g_mpool, datalen);
                    if (NULL == newbk2) {
                        DERROR("hashtable_add error!");
                        mempool_put(g_mpool, newbk, datalen);

                        MEMLINK_EXIT;
                        return 500;
                    }
                    newbk2->next = newbk->next;
                    newbk->next = newbk2;

                    node->all += g_cf->block_data_count;

                    todata  = newbk2->data;
                    enddata = todata + g_cf->block_data_count * datalen;
                }
                memcpy(todata, itemdata, datalen);
                todata += datalen; 
            }
            itemdata += datalen;
        }
        last->next = newbk;
        mempool_put(g_runtime->mpool, dbk, datalen);
    }
    
    return 0;

}

/**
 * 在数据链中任意指定位置添加数据
 */
int
hashtable_add_mask(HashTable *ht, char *key, void *value, unsigned int *maskarray, char masknum, int pos)
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
        DERROR("mask_array2binary error.\n");
        return -2;
    }

    return hashtable_add_mask_bin(ht, key, value, mask, pos);

    /*
    DataBlock *dbk = node->data;
    int count = 0;
    int datalen = node->valuesize + node->masksize;
    int i;
    char *posaddr  = NULL;
    char *itemdata;
    DataBlock *last = dbk;

    // find copy position
    while (dbk) {
        itemdata = dbk->data;

        for (i = 0; i < g_cf->block_data_count; i++) {
            if (dataitem_have_data(itemdata, node->valuesize)) {
                if (count == pos) {
                    posaddr = itemdata;
                    break;
                }
                count += 1;
            }
            itemdata += datalen;
        }
        last = dbk;
        dbk  = dbk->next;
    }

    // create new block for copy item
    DataBlock *newbk = mempool_get(g_mpool, datalen);
    if (NULL == newbk) {
        DERROR("hashtable_add get new DataBlock error!\n");
        MEMLINK_EXIT;
        return 500;
    }

    if (posaddr == NULL) { // position out of link, only create one new block
        newbk->count = 1;
        newbk->next = NULL;

        dataitem_copy(node, newbk->data, value, mask);
        node->used++;
        last->next = newbk;
    }else{ //
        char *todata = newbk->data;
        char *enddata = todata + g_cf->block_data_count * datalen;

        itemdata = dbk->data;
        newbk->next = dbk->next;

        for (i = 0; i < g_cf->block_data_count; i++) {
            if (dataitem_have_data(itemdata, node->valuesize)) {
                if (itemdata == posaddr) { // position for insert
                    dataitem_copy(node, todata, value, mask);
                    todata += datalen; 
                }
                // only copy data
                if (todata >= enddata) { // at the end of new datablock, must create another datablock
                    DataBlock *newbk2 = mempool_get(g_mpool, datalen);
                    if (NULL == newbk2) {
                        DERROR("hashtable_add error!");
                        mempool_put(g_mpool, newbk, datalen);

                        MEMLINK_EXIT;
                        return 500;
                    }
                    newbk2->next = newbk->next;
                    newbk->next = newbk2;

                    node->all += g_cf->block_data_count;

                    todata  = newbk2->data;
                    enddata = todata + g_cf->block_data_count * datalen;
                }
                memcpy(todata, itemdata, datalen);
                todata += datalen; 
            }
            itemdata += datalen;
        }
        last->next = newbk;
        mempool_put(g_runtime->mpool, dbk, datalen);
    }
    
    return 0;*/
}

/**
 * 把数据链中的某一条数据移动到链头部
 */
int
hashtable_update(HashTable *ht, char *key, void *value, int pos)
{
    char mask[HASHTABLE_MASK_MAX_LEN * sizeof(int)];
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

    if (NULL == node) {
        DWARNING("hashtable_del error: %s\n", key);
        return -1;
    }
    if (node) {
        *node = fnode;
    }

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

    int ret = hashtable_find_value(ht, key, value, &node, &item);
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
        return ret;
    }
    *(item + node->valuesize) |= tag << 1; 

    return 0;
}

/**
 * 修改一条数据的mask值
 */
int
hashtable_mask(HashTable *ht, char *key, void *value, char *maskstr)
{
    char        *item = NULL;
    HashNode    *node = NULL;

    int  ret = hashtable_find_value(ht, key, value, &node, &item);
    if (ret < 0) {
        return ret;
    }

    char mask[HASHTABLE_MASK_MAX_LEN * sizeof(int)];
    int  len = mask_string2binary(node->maskformat, maskstr, mask);
    if (len <= 0) {
        DERROR("mask string2binary error: %s\n", maskstr);
        return -1;
    }

    dataitem_copy_mask(node, item, mask);

    return 0;
}

int
hashtable_range(HashTable *ht, char *key, unsigned int *maskarray, int masknum, int frompos, int len, 
                char **data, int *datanum, int *valuesize, int *masksize)
{
    char mask[HASHTABLE_MASK_MAX_LEN * sizeof(int)];
    //char masknum;
    HashNode    *node;

    node = hashtable_find(ht, key);
    if (NULL == node) {
        DERROR("hashtable_add not found node\n");
        return -1;
    }

    //masknum = mask_string2binary(node->maskformat, maskstr, mask);
    masknum = mask_array2binary(node->maskformat, maskarray, masknum, mask);
    if (masknum <= 0) {
        DERROR("mask_string2array error");
        return -2;
    }
    
    mask[0] = mask[0] & 0xfc;

    DataBlock *dbk = node->data;
    int n = 0; 
    int datalen = node->valuesize + node->masksize;

    *valuesize = node->valuesize;
    *masksize  = node->masksize;

    char *retv = (char *)zz_malloc(len * datalen);
    if (NULL == retv) {
        DERROR("malloc error\n");
        MEMLINK_EXIT;
    }
    
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
        for (int i = 0; i < g_cf->block_data_count; i++) {
            if (dataitem_have_visiable_data(itemdata, node->valuesize)) {
                if (n >= frompos && n < endpos) {
                    memcpy(retv + idx, itemdata, datalen);
                    idx++;
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
    *data = retv;
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
            if (dataitem_have_data(itemdata, node->valuesize)) {
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


