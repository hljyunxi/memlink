#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include "zzmalloc.h"
#include "logfile.h"
#include "hashtable.h"
#include "serial.h"
#include "utils.h"
#include "common.h"

/**
 * 检查数据项中是否有数据
 * 真实删除位为0表示无效数据，为1表示存在有效数据
 * @param node
 * @param itemdata
 * @param visible 为1表示只查找可见的，为0表示查找可见和不可见的(标记删除的)
 */
int 
dataitem_have_data(HashNode *node, char *itemdata, int visible)
{
    char *maskdata  = itemdata + node->valuesize;

	if (visible) {
		if ((*maskdata & (unsigned char)0x03) == 1) {
			return MEMLINK_TRUE;
		}
	}else{
		if ((*maskdata & (unsigned char)0x01) == 1) {
			return MEMLINK_TRUE;
		}
	}
    return MEMLINK_FALSE;
}

static int
dataitem_check_data(HashNode *node, char *itemdata)
{
    char *maskdata  = itemdata + node->valuesize;
    int  delm = *maskdata & (unsigned char)0x01;
    int  tagm = *maskdata & (unsigned char)0x02;
   
    if (delm == 0)
        return MEMLINK_ITEM_REMOVED;

    if (tagm == 1)
        return MEMLINK_ITEM_TAGDEL;

    return MEMLINK_ITEM_VISIBLE;
}

/*
static int
dataitem_have_visible_data(HashNode *node, char *itemdata)
{
    return dataitem_have_data(node, itemdata, 1);
}

static int
dataitem_have_any_data(HashNode *node, char *itemdata)
{
    return dataitem_have_data(node, itemdata, 0);
}
*/

/**
 * 检查数据项中是否有要显示的数据
 * 标记删除位为1表示数据标记为删除
 */
/*
static int 
dataitem_have_visible_data(HashNode *node, char *itemdata)
{
    unsigned char n = *(itemdata + node->valuesize);
    if ((n & (unsigned char)0x03) == 0x01)
        return 1;
    return MEMLINK_OK;
}
*/
/**
 * 在数据链的一块中查找某一条数据
 */
static char*
dataitem_lookup(HashNode *node, void *value, DataBlock **dbk)
{
    int i;
    int datalen = node->valuesize + node->masksize;
    DataBlock *root = node->data;

    while (root) {
        char *data = root->data;
        DINFO("data: %p\n", data);
        for (i = 0; i < g_cf->block_data_count; i++) {
            if (dataitem_have_data(node, data, 0) && memcmp(value, data, node->valuesize) == 0) {
                if (dbk) {
                    *dbk = root;
                }
                return data;
            }
            data += datalen;
        }
        root = root->next;
    }
    return NULL;
}

static int
datablock_del(HashNode *node, DataBlock *dbk, char *data)
{
    char *mdata = data + node->valuesize;
    unsigned char v = *mdata & 0x02;

    *mdata &= 0xfe;
    if (v == 2) {
        dbk->mask_count--;
    }else{
        dbk->visible_count--;
    }
    node->used--;

    return 0;
}

static int
datablock_del_restore(HashNode *node, DataBlock *dbk, char *data)
{
    char *mdata = data + node->valuesize;
    unsigned char v = *mdata & 0x02;

    *mdata |= 0x01;
    if (v == 2) {
        dbk->mask_count++;
    }else{
        dbk->visible_count++;
    }
    node->used++;

    return 0;
}
/**
 * 在数据链中找到某位置所在数据块
 * @param node 
 * @param pos
 * @param visible 为1表示只查找可见的，为0表示查找可见和不可见
 * @param dbk 指定位置所在数据块
 */
static int
datablock_lookup_pos(HashNode *node, int pos, int visible, DataBlock **dbk)
{
    DataBlock *root = node->data;

	int n = 0, startn = 0;
    
    while (root) {
		startn = n;
		if (visible) {
			n += root->visible_count;	
		}else{
			n += root->mask_count + root->visible_count;
		}
	
		if (n >= pos) {
			*dbk = root;
			return startn;
		}

        root = root->next;
    }

    return -1;
}


static int
dataitem_lookup_pos_mask(HashNode *node, int pos, int visible, char *maskval, char *maskflag, DataBlock **dbk, char **addr)
{
    DataBlock *root = node->data;

	int n = 0, startn = 0;
	int i, k;
	int datalen = node->valuesize + node->masksize;
    
    while (root) {
		startn = n;
		char *itemdata = root->data;

		for (i = 0; i < g_cf->block_data_count; i++) {
			if (dataitem_have_data(node, itemdata, visible)) {
				char *maskdata = itemdata + node->valuesize;	
				
				char buf[64];
				DINFO("i: %d, maskdata: %s\n", i, formatb(maskdata, node->masksize, buf, 64));
				for (k = 0; k < node->masksize; k++) {
					//DINFO("check k:%d, maskdata:%x, maskflag:%x, maskval:%x\n", k, maskdata[k], maskflag[k], maskval[k]);
					if ((maskdata[k] & maskflag[k]) != maskval[k]) {
						break;
					}
				}
				if (k < node->masksize) { // not equal
					DINFO("not equal.\n");
					continue;
				}

				n += 1;
				if (n >= pos) {
					*dbk  = root;
					*addr = itemdata;
					return startn;
				}
			}
			itemdata += datalen;
		}
		/*
		if (n >= pos) {
			*dbk = root;
			return startn;
		}*/

        root = root->next;
    }

    return -1;
}

/**
 * find a insert pos in DataBlock link
 */
static int
datablock_lookup_valid_pos(HashNode *node, int pos, int visible, DataBlock **dbk, DataBlock **prev)
{
    DataBlock *root = node->data;
    DataBlock *previtem = NULL;

	int n = 0, startn = 0;
    
    while (root) {
		startn = n;
		if (visible) {
			n += root->visible_count;	
		}else{
			n += root->mask_count + root->visible_count;
		}
	
		//if (n >= pos) {
		if (n > pos) {
			*dbk = root;
            if (prev) {
                *prev = previtem;
            }
			return startn;
		}

        if (root->next == NULL) {
            if (startn + g_cf->block_data_count > n) { // last DataBlock have idle space
                *dbk = root;
                if (prev) {
                    *prev = previtem;
                }
                return startn;
            }

            *dbk = NULL;
            if (prev) {
                *prev = root;
            }
            return  -1;
        }

        previtem = root;
        root = root->next;
    }
    
    DERROR("must not run.\n");

    return -2;
}

static int
dataitem_lookup_pos(HashNode *node, int pos, int visible, DataBlock **dbk, DataBlock **last, char **data)
{
    int ret;

    ret = datablock_lookup_valid_pos(node, pos, visible, dbk, last);
    DINFO("datablock_lookup_pos, pos:%d, dbk:%p, last:%p, ret:%d\n", pos, *dbk, *last, ret);
    if (ret < 0 || *dbk == NULL) {
        *data = 0;
        return ret;
    }

    int skipn   = pos - ret; 
    int datalen = node->valuesize + node->masksize;
    
    DataBlock   *fdbk = *dbk;
    char        *item = fdbk->data;
    int         i;
   
    // find last idle space
    if (skipn >= g_cf->block_data_count) {
        char *enddata = item + datalen * g_cf->block_data_count;
        item = enddata;
        
        for (i = 0; i < g_cf->block_data_count; i++) {
            item  -= datalen;
            if (dataitem_have_data(node, item, 0)) {
                *data = item + datalen;
                break;
            }
        }

        return  MEMLINK_OK;
    }

    // find special pos
    DINFO("skipn: %d\n", skipn);
    for (i = 0; i < g_cf->block_data_count; i++) {
        if (skipn == 0) {
            *data = item;
            return MEMLINK_OK;
        }
        skipn -= 1;
        item  += datalen;
    }

    *data = NULL;
    DERROR("dataitem_lookup_pos error! pos error! pos: %d\n", pos);

    return MEMLINK_OK;
}

/**
 * copy a value, mask to special address
 * @param node  HashNode be copied
 * @param addr  to address
 * @param value copied value
 * @param mask  copied mask, binary
 */
static int
dataitem_copy(HashNode *node, char *addr, void *value, void *mask)
{
    //char *m = mask;
    //*m = *m | (*addr & 0x03);

    DINFO("dataitem_copy valuesize: %d, masksize: %d\n", node->valuesize, node->masksize);
    memcpy(addr, value, node->valuesize);
    memcpy(addr + node->valuesize, mask, node->masksize);
    
    return MEMLINK_OK;
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

    return MEMLINK_OK;
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
            return MEMLINK_ERR_MASK;
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
            return MEMLINK_ERR_EKEY;
        }
        node = node->next;
    }
    
    node = (HashNode*)zz_malloc(sizeof(HashNode));
    if (NULL == node) {
        DERROR("malloc HashNode error!\n");
        return MEMLINK_ERR_MEM;
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

    return MEMLINK_OK;    
}

int
hashtable_remove_key(HashTable *ht, char *key)
{
	//int				ret;
    int             keylen = strlen(key);
    unsigned int    hash   = hashtable_hash(key, keylen);
    HashNode        *node  = ht->bunks[hash];
	HashNode		*last  = NULL;

    while (node) {
        if (strcmp(key, node->key) == 0) {
			break;
        }
		last = node;
        node = node->next;
    }
	
	if (NULL == node) {
		return MEMLINK_ERR_NOKEY;
	}
	
	DataBlock	*dbk = node->data;
	DataBlock	*tmp;
	int			datalen = node->valuesize + node->masksize;
	zz_free(node->key);		
	zz_free(node);
	
	while (dbk) {
		tmp = dbk;
		dbk = dbk->next;
		mempool_put(g_runtime->mpool, tmp, datalen);	
	}
	return MEMLINK_OK;
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

/*int
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
*/
/**
 * 在数据链的头部添加数据
 */
/*int 
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

    return MEMLINK_OK;
}
*/


int
hashtable_add(HashTable *ht, char *key, void *value, char *maskstr, int pos)
{
    unsigned int maskarray[HASHTABLE_MASK_MAX_LEN];
    char masknum;

    masknum = mask_string2array(maskstr, maskarray);
    if (masknum <= 0) {
        DERROR("mask_string2array error: %s\n", maskstr);
        return MEMLINK_ERR_MASK;
    }

    return hashtable_add_mask(ht, key, value, maskarray, masknum, pos);
}

int
hashtable_add_mask_bin(HashTable *ht, char *key, void *value, void *mask, int pos)
{
    HashNode *node = hashtable_find(ht, key);
    if (NULL == node) {
        DERROR("hashtable_add not found node\n");
        return MEMLINK_ERR_NOKEY;
    }
    
    DataBlock *dbk = node->data;
    //int count = 0;
    int datalen = node->valuesize + node->masksize;
    int i;
    char *posaddr  = NULL;
    char *itemdata;
    DataBlock *last = NULL;

	DINFO("lookup pos, pos:%d, dbk:%p\n", pos, dbk);
    int ret = dataitem_lookup_pos(node, pos, 0, &dbk, &last, &posaddr);
    if (ret < 0) {
        DERROR("dataitem_lookup_pos not find pos.\n");
        //return -2;
    }
    DINFO("dataitem_lookup_pos, dbk:%p, last:%p, posaddr:%p\n", dbk, last, posaddr);
    // create new block for copy item
    DataBlock *newbk = mempool_get(g_mpool, datalen);
    if (NULL == newbk) {
        DERROR("hashtable_add get new DataBlock error!\n");
        MEMLINK_EXIT;
        return MEMLINK_ERR_MEM;
    }
    DINFO("create newbk:%p, dbk:%p\n", newbk, dbk);

    node->all += g_cf->block_data_count;

    if (posaddr == NULL) { // position out of link, only create a new block
        DINFO("not found the same hash, create new\n");
        newbk->next  = NULL;

        dataitem_copy(node, newbk->data, value, mask);
        newbk->visible_count = 1;
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

            if (itemdata == posaddr) { // position for insert, if last position, may be null
                DINFO("insert pos, go\n");
                dataitem_copy(node, todata, value, mask);
                lastnewbk->visible_count ++;
                todata += datalen; 
    
                //hashtable_print(ht, key);
                //DINFO("data1 %p ... %s\n", itemdata, formatb(itemdata + node->valuesize, node->masksize, buf, 128) );
                //DINFO("data1 %p ... %s\n", itemdata, formatb(itemdata + datalen + node->valuesize, node->masksize, buf, 128) );
            }

            if (dataitem_have_data(node, itemdata, 0)) { 
                /*
                if (itemdata == posaddr) { // position for insert
                    DINFO("insert pos, go\n");
                    dataitem_copy(node, todata, value, mask);
                    lastnewbk->visible_count ++;
                    todata += datalen; 
        
                    //hashtable_print(ht, key);
                    DINFO("data1 %p ... %s\n", itemdata, formatb(itemdata + node->valuesize, node->masksize, buf, 128) );
                    DINFO("data1 %p ... %s\n", itemdata, formatb(itemdata + datalen + node->valuesize, node->masksize, buf, 128) );
                }
                */
                if (todata >= enddata) { // at the end of new datablock, must create another datablock
                    DINFO("create a new datablock.\n");
                    DataBlock *newbk2 = mempool_get(g_mpool, datalen);
                    if (NULL == newbk2) {
                        DERROR("hashtable_add error!\n");
                        //mempool_put(g_mpool, newbk, datalen);
                        MEMLINK_EXIT;
                        return MEMLINK_ERR_MEM;
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
				if (dataitem_have_data(node, itemdata, 0)) {
					lastnewbk->visible_count += 1;	
				}else{
					lastnewbk->mask_count ++;
				}
                todata += datalen; 
            }
            itemdata += datalen;
        }
    
        if (dbk->next != newbk->next) { // create two datablock
            DataBlock   *nextbk = dbk->next; 
            DataBlock   *new_nextbk = newbk->next;
            itemdata = nextbk->data;

            if (nextbk && nextbk->visible_count + nextbk->mask_count < g_cf->block_data_count) {
                for (i = 0; i < g_cf->block_data_count; i++) {
                    ret = dataitem_check_data(node, itemdata);
                    //if (dataitem_have_data(node, itemdata, 0)) { 
                    if (ret != MEMLINK_ITEM_REMOVED) {
                        memcpy(todata, itemdata, datalen); 
                        todata += datalen;
                        if (ret == MEMLINK_ITEM_TAGDEL) {
                            new_nextbk->mask_count ++; 
                        }else{
                            new_nextbk->visible_count ++; 
                        }
                    }
                    itemdata += datalen;
                }

                new_nextbk->next = nextbk->next;
                mempool_put(g_runtime->mpool, nextbk, datalen);
            }
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

    return MEMLINK_OK;
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
        return MEMLINK_ERR_NOKEY;
    }
    
    ret = mask_array2binary(node->maskformat, maskarray, masknum, mask);
    if (ret <= 0) {
        DERROR("mask_array2binary error: %d\n", ret);
        return MEMLINK_ERR_MASK;
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
    DataBlock *dbk = NULL;
    char     *item = NULL; 

    char ret = hashtable_find_value(ht, key, value, &node, &dbk, &item);
    if (ret < 0) {
        DWARNING("not found value: %d, %s\n", ret, key);
        return ret;
    }
    
    char *mdata = item + node->valuesize;
    memcpy(mask, mdata, node->masksize);  

    datablock_del(node, dbk, item);

    int retv = hashtable_add_mask_bin(ht, key, value, mask, pos);

    if (retv != MEMLINK_OK) {
        datablock_del_restore(node, dbk, item);
        return retv;
    }
    
    return MEMLINK_OK;
}

/**
 * 通过key, value，在HashTable中找到一条数据的地址，同时返回对应的HashNode
 */
int
hashtable_find_value(HashTable *ht, char *key, void *value, HashNode **node, DataBlock **dbk, char **data)
{
    HashNode *fnode = hashtable_find(ht, key);

    if (NULL == fnode) {
        DWARNING("hashtable_del error: %s\n", key);
        return MEMLINK_ERR_NOKEY;
    }
    if (node) {
        *node = fnode;
    }

    DINFO("find dataitem ...\n");
    char *item = dataitem_lookup(fnode, value, dbk);
    if (NULL == item) {
        DWARNING("dataitem_lookup error: %s, %x\n", key, *(unsigned int*)value);
        return MEMLINK_ERR_NOVAL;
    }
    *data = item;

    return MEMLINK_OK; 
}

/**
 * 在HashTable中删除一条数据。只是把真实删除标记位置1
 */
int 
hashtable_del(HashTable *ht, char *key, void *value)
{
    char        *item = NULL;
    HashNode    *node = NULL;
    DataBlock   *dbk  = NULL;

    DINFO("hashtable_find_value: %s, value: %s\n", key, (char*)value);
    int ret = hashtable_find_value(ht, key, value, &node, &dbk, &item);
    DINFO("hashtable_find_value ret: %d\n", ret);
    if (ret < 0) {
        DERROR("hashtable_del find error!\n");
        return ret;
    }
    char *data = item + node->valuesize;
    unsigned char v = *data & 0x3; 

    *data &= 0xfe;

    if ( (v & 0x01) == 1) {
        if ( (v & 0x02) == 1) {
            dbk->mask_count --;
        }else{
            dbk->visible_count --;
        }
        node->used--;
    }

    return MEMLINK_OK;
}

/**
 * 标记删除/标记恢复。只是把对应的标记删除位置1/0
 */
int
hashtable_tag(HashTable *ht, char *key, void *value, unsigned char tag)
{
    char        *item = NULL;
    HashNode    *node = NULL;
    DataBlock   *dbk  = NULL;

    int ret = hashtable_find_value(ht, key, value, &node, &dbk, &item);
    if (ret < 0) {
        DERROR("not found key: %s\n", key);
        return ret;
    }
    DINFO("tag: %d, %d\n", tag, tag<<1);
    
    char *data = item + node->valuesize;
    unsigned char v = *data & 0x3; 

    DINFO("tag v:%x\n", v);

    if ( (v & 0x01) == 1) {
        DINFO("tag data:%x\n", *data);
        if (tag == 1) {
            *data |= 0x02; 
        }else{
            *data &= 0xfd; 
        }
    
        DINFO("tag data:%x\n", *data);
            
        if ( (v & 0x02) == 1 && tag == 0) {
            dbk->mask_count --;
            dbk->visible_count ++;
        }

        if ( (v & 0x02) == 0 && tag == 1) {
            dbk->mask_count ++;
            dbk->visible_count --;
        }
    }else{
        return MEMLINK_ERR_REMOVED;
    }

    return MEMLINK_OK;
}

/**
 * 修改一条数据的mask值
 */
int
hashtable_mask(HashTable *ht, char *key, void *value, unsigned int *maskarray, int masknum)
{
    char        *item = NULL;
    HashNode    *node = NULL;

    int  ret = hashtable_find_value(ht, key, value, &node, NULL, &item);
    if (ret < 0) {
        return ret;
    }

    char mask[HASHTABLE_MASK_MAX_LEN * sizeof(int)] = {0};
    char maskflag[HASHTABLE_MASK_MAX_LEN * sizeof(int)] = {0};

    int  len = mask_array2binary(node->maskformat, maskarray, masknum, mask);
    if (len <= 0) {
        DERROR("mask array2binary error: %d\n", masknum);
        return MEMLINK_ERR_MASK;
    }
    char buf[128];
    DINFO("array2bin: %s\n", formatb(mask, len, buf, 128));

    int flen = mask_array2flag(node->maskformat, maskarray, masknum, maskflag);
    if (flen <= 0) {
        DERROR("mask array2flag error: %d\n", masknum);
        return MEMLINK_ERR_MASK;
    }
    DINFO("array2flag: %s\n", formatb(maskflag, flen, buf, 128));

    dataitem_copy_mask(node, item, maskflag, mask);

    return MEMLINK_OK;
}

int
hashtable_range(HashTable *ht, char *key, unsigned int *maskarray, int masknum, 
                int frompos, int len, 
                char *data, int *datanum, 
                unsigned char *valuesize, unsigned char *masksize)
{
    char maskval[HASHTABLE_MASK_MAX_LEN * sizeof(int)];
	char maskflag[HASHTABLE_MASK_MAX_LEN * sizeof(int)];
    HashNode    *node;
	int			startn;
    int         masklen = 0;
	int			havemask = 0;

    node = hashtable_find(ht, key);
    if (NULL == node) {
        DERROR("hashtable_add not found node\n");
        return MEMLINK_ERR_NOKEY;
    }

    if (masknum > 0) {
        masklen = mask_array2binary(node->maskformat, maskarray, masknum, maskval);
        if (masklen <= 0) {
            DERROR("mask_string2array error\n");
            return -2;
        }
        maskval[0] = maskval[0] & 0xfc;
		
		int j;
		for (j = 0; j < masknum; j++) {
			havemask += maskval[j];
		}

		masklen = mask_array2flag(node->maskformat, maskarray, masknum, maskflag);
		if (masklen <= 0) {
            DERROR("mask_array2flag error\n");
            return -2;
        }

		for (j = 0; j < masknum; j++) {
			maskflag[j] = ~maskflag[j];
		}
		maskflag[0] = maskflag[0] & 0xfc;

		char buf[64];

		DINFO("maskval:  %s\n", formatb(maskval, node->masksize, buf, 64));
		DINFO("maskflag: %s\n", formatb(maskflag, node->masksize, buf, 64));
    }

    DataBlock *dbk = NULL;
    int datalen = node->valuesize + node->masksize;
	char *addr  = NULL;

    *valuesize = node->valuesize;
    *masksize  = node->masksize;

	DINFO("havemask: %d\n", havemask);
    if (havemask) {
		startn = dataitem_lookup_pos_mask(node, frompos, 1, maskval, maskflag, &dbk, &addr);
		DINFO("dataitem_lookup_pos_mask startn: %d\n", startn);
		if (startn < 0) { // out of range
			*datanum = 0;
			return MEMLINK_OK;
		}
    }else{
		startn = datablock_lookup_pos(node, frompos, 1, &dbk);
		DINFO("datablock_lookup_pos startn: %d\n", startn);
		if (startn < 0) { // out of range
			*datanum = 0;
			return MEMLINK_OK;
		}
	}
	
	int skipn = frompos - startn;
    int idx   = 0;
    int n     = 0; 
    char      maskstr[256];
    unsigned char mlen;

	DINFO("skipn: %d\n", skipn);
    while (dbk) {
        char *itemdata = dbk->data;
        int  i;
        char buf[128];
        for (i = 0; i < g_cf->block_data_count; i++) {
			if (addr) {
				if (itemdata != addr) {
					continue;
				}else{
					addr = NULL;
				}
			}
            if (dataitem_have_data(node, itemdata, 1)) {
				if (havemask) {
					int k;
					char *maskdata = itemdata + node->valuesize;

					for (k = 0; k < node->masksize; k++) {
						if ((maskdata[k] & maskflag[k]) != maskval[k]) {
							break;
						}
					}
					if (k < node->masksize) { // not equal
						//DINFO("not equal.\n");
						continue;
					}

				}
				if (skipn > 0) {
					skipn -= 1;
					itemdata += datalen;
					continue;
				}
				
                mlen = mask_binary2string(node->maskformat, node->masknum, itemdata + node->valuesize, node->masksize, maskstr);
				snprintf(buf, node->valuesize + 1, "%s", itemdata);

				DINFO("ok, copy item ... i:%d, value:%s maskstr:%s, mlen:%d\n", i, buf, maskstr, mlen);
				memcpy(data + idx, itemdata, node->valuesize);
                idx += node->valuesize;
                memcpy(data + idx, &mlen, sizeof(char));
                idx += sizeof(char);
                memcpy(data + idx, maskstr, mlen);
                idx += mlen;
				//memcpy(data + idx, itemdata, datalen);
				//idx += datalen;
				n += 1;

				if (n >= len) {
					goto hashtable_range_over;
				}
            }
            itemdata += datalen;
        }
        dbk = dbk->next;
    }

hashtable_range_over:
	DINFO("count: %d\n", n);
    //*data = retv;
    *datanum = idx;

    return MEMLINK_OK;
}

int
hashtable_clean(HashTable *ht, char *key)
{
    HashNode    *node = hashtable_find(ht, key); 
    if (NULL == node)
        return MEMLINK_ERR_NOKEY;

    DataBlock   *dbk = node->data;
    int         dlen = node->valuesize + node->masksize;

    if (NULL == dbk)
        return MEMLINK_OK;

    if (node->used == 0) {
        DataBlock *tmp;
        while (dbk) {
            tmp = dbk; 
            dbk = dbk->next;
            mempool_put(g_runtime->mpool, dbk, dlen);
        }
        node->all  = 0;
        node->data = NULL;
        return MEMLINK_OK;
    }

    DataBlock   *newroot = NULL, *newlast = NULL;
    char        *itemdata; // = dbk->data;
    int         i, dataall = g_cf->block_data_count;

    DataBlock *newdbk     = mempool_get(g_runtime->mpool, dlen);
    char      *newdbk_end = newdbk->data + g_cf->block_data_count * dlen;
    char      *newdbk_pos = newdbk->data;

    newroot = newdbk;
    
    while (dbk) {
        itemdata = dbk->data;
        for (i = 0; i < g_cf->block_data_count; i++) {
            if (dataitem_have_data(node, itemdata, 0)) {
                /*
                unsigned char v = *(itemdata + node->valuesize) & 0x02;
                memcpy(newdbk_pos, itemdata, dlen);
                newdbk_pos += dlen;

                if (v == 2) {
                    newdbk->mask_count++;
                }else{
                    newdbk->visible_count++;
                }*/

                if (newdbk_pos >= newdbk_end) {
                    newlast = newdbk;

                    newdbk     = mempool_get(g_runtime->mpool, dlen);
                    newdbk_end = newdbk->data + g_cf->block_data_count * dlen;
                    newdbk_pos = newdbk->data;

                    newdbk->next  = NULL;
                    newlast->next = newdbk; 

                    dataall += g_cf->block_data_count;
                }

                unsigned char v = *(itemdata + node->valuesize) & 0x02;
                memcpy(newdbk_pos, itemdata, dlen);
                newdbk_pos += dlen;

                if (v == 2) {
                    newdbk->mask_count++;
                }else{
                    newdbk->visible_count++;
                }
            }
            itemdata += dlen;
        }
        dbk = dbk->next;
    }

    node->data = newroot;
    node->all  = dataall;

    return MEMLINK_OK;
} 

int
hashtable_stat(HashTable *ht, char *key, HashTableStat *stat)
{
    HashNode    *node;

    node = hashtable_find(ht, key);
    if (NULL == node) {
        DERROR("hashtable_stat not found key: %s\n", key);
        return MEMLINK_ERR_NOKEY;
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
    
    DINFO("valuesize:%d, masksize:%d, blocks:%d, data:%d, data_used:%d, mem:%d, mem_used:%d\n", 
            stat->valuesize, stat->masksize, stat->blocks, stat->data, stat->data_used,
            stat->mem, stat->mem_used);

    return MEMLINK_OK;
}

int
hashtable_count(HashTable *ht, char *key, int *visible_count, int *mask_count)
{
    HashNode    *node;

    node = hashtable_find(ht, key);
    if (NULL == node) {
        DERROR("hashtable_stat not found key: %s\n", key);
        return MEMLINK_ERR_NOKEY;
    }

    int vcount = 0, mcount = 0;

    DataBlock *dbk = node->data;
    while (dbk) {
        vcount += dbk->visible_count;
        mcount += dbk->mask_count;
        dbk = dbk->next;
    }

    *visible_count = vcount;
    *mask_count    = mcount;

    return MEMLINK_OK;
}


int 
hashtable_print(HashTable *ht, char *key)
{
    HashNode    *node;

    node = hashtable_find(ht, key);
    if (NULL == node) {
        DERROR("not found key: %s\n", key);
        return MEMLINK_ERR_NOKEY;
    }

    DINFO("------ HashNode key:%s, valuesize:%d, masksize:%d, masknum:%d, all:%d, used:%d\n",
                node->key, node->valuesize, node->masksize, node->masknum, node->all, node->used);

    int i;
    for (i = 0; i < node->masknum; i++) {
        DINFO("maskformat: %d, %d\n", i, node->maskformat[i]);
    }

    DataBlock *dbk = node->data;

    int blocks = 0;
    int datalen = node->valuesize + node->masksize;
    char buf1[128];
	char buf2[128];

    while (dbk) {
        blocks += 1;
        char *itemdata = dbk->data; 
        DINFO("====== dbk %p visible_count: %d, mask_count: %d, block: %d ======\n", dbk, dbk->visible_count, dbk->mask_count, blocks);
        for (i = 0; i < g_cf->block_data_count; i++) {
            if (dataitem_have_data(node, itemdata, 1)) {
                //DINFO("i: %d, value: %s, mask: %s\n", i, formath(itemdata, node->valuesize, buf2, 128), 
                //            formath(itemdata + node->valuesize, node->masksize, buf1, 128));
				memcpy(buf2, itemdata, node->valuesize);
				buf2[node->valuesize] = 0;
                DINFO("i: %d, value: %s, mask: %s\n", i, buf2, 
                            formath(itemdata + node->valuesize, node->masksize, buf1, 128));
            }else if (dataitem_have_data(node, itemdata, 0)) {
				memcpy(buf2, itemdata, node->valuesize);
				buf2[node->valuesize] = 0;
                DINFO("i: %d, value: %s, mask: %s\tdel\n", i, buf2, 
                            formath(itemdata + node->valuesize, node->masksize, buf1, 128));
            }else{
                DINFO("i: %d, no data, mask: %s\n", i, formath(itemdata + node->valuesize, node->masksize, buf1, 128));
            }

            itemdata += datalen;
        }

        dbk = dbk->next;
    }
    DINFO("------ blocks: %d\n", blocks);

    return MEMLINK_OK;
}


