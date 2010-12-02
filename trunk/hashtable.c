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

int
dataitem_check_data(HashNode *node, char *itemdata)
{
    char *maskdata  = itemdata + node->valuesize;
    int  delm = *maskdata & (unsigned char)0x01;
    int  tagm = *maskdata & (unsigned char)0x02;
   
    if (delm == 0)
        return MEMLINK_ITEM_REMOVED;

    if (tagm == 2)
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
		DINFO("root: %p, data: %p, next: %p\n", root, data, root->next);
        //DINFO("data: %p\n", data);
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


int
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
                /*
				char buf[64], buf2[64];
				DINFO("i: %d, maskdata: %s maskflag: %s\n", i, formatb(maskdata, node->masksize, buf, 64), formatb(maskflag, node->masksize, buf2, 64));
                */
				for (k = 0; k < node->masksize; k++) {
					DINFO("check k:%d, maskdata:%x, maskflag:%x, maskval:%x\n", k, maskdata[k], maskflag[k], maskval[k]);
					if ((maskdata[k] & maskflag[k]) != maskval[k]) {
						break;
					}
				}
				if (k < node->masksize) { // not equal
					DINFO("not equal.\n");
			        itemdata += datalen;
					continue;
				}

				if (n >= pos) {
					*dbk  = root;
					*addr = itemdata;
					return startn;
				}
				n += 1;
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
 * find a insert pos in DataBlock link. skip DataBlock
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

        // if last block
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
    
    //DERROR("must not run. pos:%d\n", pos);
    return -2;
}

int
dataitem_lookup_pos(HashNode *node, int pos, int visible, DataBlock **dbk, DataBlock **prev, char **data)
{
    int ret;

    ret = datablock_lookup_valid_pos(node, pos, visible, dbk, prev);
    DINFO("datablock_lookup_pos, pos:%d, dbk:%p, prev:%p, ret:%d\n", pos, *dbk, *prev, ret);
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
    //if (skipn >= g_cf->block_data_count) {
	int n = 0;

	if (*dbk) {
		if (visible) {
			n = (*dbk)->visible_count;
		}else{
			n = (*dbk)->visible_count + (*dbk)->mask_count;
		}
	}

    // position out of block used count, find last idle address
	if (skipn >= n) {
        char *enddata = item + datalen * g_cf->block_data_count;
        item = enddata;
        
        for (i = 0; i < g_cf->block_data_count; i++) {
            item  -= datalen;
            //if (dataitem_have_data(node, item, 0)) {
            if (dataitem_have_data(node, item, visible)) {
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
		//if (dataitem_have_data(node, item, 0)) {
		if (dataitem_have_data(node, item, visible)) {
			skipn -= 1;
		}
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
    //char buf[128];
    //DINFO("copy_mask before: %s\n", formatb(addr, node->masksize, buf, 128)); 

    for (i = 0; i < node->masksize; i++) {
        maskaddr[i] = (maskaddr[i] & maskflag[i]) | mask[i];
    }
    //memcpy(addr + node->valuesize, mask, node->masksize);
    //DINFO("copy_mask after: %s\n", formatb(addr, node->masksize, buf, 128)); 

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
			zz_free(ht->bunks[i]->key);
			zz_free(ht->bunks[i]->maskformat);
        }
    }

    zz_free(ht);
}

void
hashtable_clear_all(HashTable *ht)
{
    if (NULL == ht)
        return;

    int i;

    for (i = 0; i < HASHTABLE_BUNKNUM; i++) {
		//DINFO("clear: %d, %p\n", i, ht->bunks[i]);
        if (ht->bunks[i] != NULL) {
            DataBlock   *dbk = ht->bunks[i]->data;
            DataBlock   *tmp; 
            while (dbk) {
                tmp = dbk;
                dbk = dbk->next;
                zz_free(tmp);
            }
			zz_free(ht->bunks[i]->key);
			zz_free(ht->bunks[i]->maskformat);
        }
    }
	memset(ht->bunks, 0, sizeof(ht->bunks));
}

/**
 * 创建HashTable中一个key的结构信息
 */
int
hashtable_add_info(HashTable *ht, char *key, int valuesize, char *maskstr)
{
    unsigned int format[HASHTABLE_MASK_MAX_ITEM] = {0};
    int  masknum = mask_string2array(maskstr, format);
    int  i; 
    
    if (masknum > HASHTABLE_MASK_MAX_ITEM) {
        return MEMLINK_ERR_MASK;
    }

    for (i = 0; i < masknum; i++) {
        if (format[i] == UINT_MAX || format[i] > HASHTABLE_MASK_MAX_BIT) {
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
    int             keylen = strlen(key);
    unsigned int    hash   = hashtable_hash(key, keylen);
    //HashNode        *root  = ht->bunks[hash];
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

    if (last) {
        last->next = node->next;
    }else{
        ht->bunks[hash] = node->next; 
    }

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
    unsigned int maskarray[HASHTABLE_MASK_MAX_ITEM];
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
    char mask[HASHTABLE_MASK_MAX_ITEM * sizeof(int)];
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
    unsigned int maskarray[HASHTABLE_MASK_MAX_ITEM];
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
    /*
    if (ret < 0) {
        DERROR("dataitem_lookup_pos not find pos.\n");
        //return -2;
    }*/

    DINFO("dataitem_lookup_pos, dbk:%p, last:%p, posaddr:%p\n", dbk, last, posaddr);
	
    // create new block for copy item
    /*DataBlock *newbk = mempool_get(g_mpool, datalen);
    if (NULL == newbk) {
        DERROR("hashtable_add get new DataBlock error!\n");
        MEMLINK_EXIT;
        return MEMLINK_ERR_MEM;
    }
    DINFO("create newbk:%p, dbk:%p\n", newbk, dbk);

    node->all += g_cf->block_data_count;
	*/

    if (posaddr == NULL) { // position out of link, only create a new block
		// create new block for copy item
		DataBlock *newbk = mempool_get(g_mpool, datalen);
		if (NULL == newbk) {
			DERROR("hashtable_add get new DataBlock error!\n");
			MEMLINK_EXIT;
			return MEMLINK_ERR_MEM;
		}
		DINFO("create newbk:%p, dbk:%p\n", newbk, dbk);

		node->all += g_cf->block_data_count;
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
        if (dataitem_have_data(node, posaddr, 0) == MEMLINK_FALSE) {
            DINFO("posaddr have no data, insert !\n");
            dataitem_copy(node, posaddr, value, mask);
            dbk->visible_count++;
			node->used++;
            return MEMLINK_OK;
        }
		
		DataBlock *newbk = mempool_get(g_mpool, datalen);
		if (NULL == newbk) {
			DERROR("hashtable_add get new DataBlock error!\n");
			MEMLINK_EXIT;
			return MEMLINK_ERR_MEM;
		}
		DINFO("create newbk:%p, dbk:%p\n", newbk, dbk);
		node->all += g_cf->block_data_count;

        char *todata  = newbk->data;
        char *enddata = todata + g_cf->block_data_count * datalen;
        itemdata    = dbk->data;
        newbk->next = dbk->next;

        DINFO("found exist hash, newbk: %p, dbk: %p, todata:%p, enddata:%p\n", newbk, dbk, todata, enddata);
        DataBlock   *lastnewbk = newbk;
        for (i = 0; i < g_cf->block_data_count; i++) {
            //char buf[128] = {0};
            if (itemdata == posaddr) { // position for insert, if last position, may be null
                DINFO("insert pos, go\n");
                dataitem_copy(node, todata, value, mask);
                lastnewbk->visible_count ++;
                todata += datalen; 
            }

            if (dataitem_have_data(node, itemdata, 0)) { 
                if (todata >= enddata) { // at the end of new datablock, must create another datablock
                    DataBlock *newbk2 = mempool_get(g_mpool, datalen);
                    DINFO("create a new datablock. %p\n", newbk2);
                    if (NULL == newbk2) {
                        DERROR("hashtable_add error!\n");
                        //mempool_put(g_mpool, newbk, datalen);
                        MEMLINK_EXIT;
                        return MEMLINK_ERR_MEM;
                    }
                    newbk2->next = newbk->next;
                    newbk->next  = newbk2;
					//DINFO("newbk: %p, next: %p\n", newbk2, newbk2->next);
                    lastnewbk = newbk2;
                    node->all += g_cf->block_data_count;
                    todata  = newbk2->data;
                    enddata = todata + g_cf->block_data_count * datalen;
                }
                // only copy data
                    
                //DINFO("data2 %p ... %s\n", itemdata, formatb(itemdata + node->valuesize, node->masksize, buf, 128) );
                //DINFO("copy data ...  %s\n", formatb(itemdata + node->valuesize, node->masksize, buf, 128) );
                memcpy(todata, itemdata, datalen);
				if (dataitem_have_data(node, itemdata, 1)) {
					lastnewbk->visible_count += 1;	
				}else{
					lastnewbk->mask_count ++;
				}
                todata += datalen; 
            }
            itemdata += datalen;
        }

        // insert to last
		DINFO("pasaddr:%p, end:%p\n", posaddr, dbk->data + g_cf->block_data_count * datalen);
        if (posaddr == dbk->data + g_cf->block_data_count * datalen) {
            dataitem_copy(node, todata, value, mask);
            lastnewbk->visible_count ++;
            todata += datalen; 
        }
    
        if (dbk->next != newbk->next) { // create two datablock
            DataBlock   *nextbk = dbk->next;  // after 1
            DataBlock   *new_nextbk = newbk->next; // new 2
            itemdata = nextbk->data;
			
			DINFO("merge block: %p %p\n", newbk->next, newbk->next->next);
            if (nextbk && nextbk->visible_count + nextbk->mask_count < g_cf->block_data_count) {
                for (i = 0; i < g_cf->block_data_count; i++) {
                    ret = dataitem_check_data(node, itemdata);
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
				//DINFO("put to pool: %p\n", nextbk);
                mempool_put(g_runtime->mpool, nextbk, datalen);
				node->all -= g_cf->block_data_count;	
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
    char mask[HASHTABLE_MASK_MAX_ITEM * sizeof(int)] = {0};
    int  ret;
    HashNode *node = hashtable_find(ht, key);
    if (NULL == node) {
        DERROR("hashtable_add not found node\n");
        return MEMLINK_ERR_NOKEY;
    }
   
	if (node->masknum != masknum) {
		return MEMLINK_ERR_MASK;
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
    char mask[HASHTABLE_MASK_MAX_ITEM * sizeof(int)] = {0};
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

    DINFO("find dataitem ... node: %p\n", fnode);
    char *item = dataitem_lookup(fnode, value, dbk);
    if (NULL == item) {
        DWARNING("dataitem_lookup error: %s, %x\n", key, *(unsigned int*)value);
        return MEMLINK_ERR_NOVAL;
    }
	DINFO("find dataitem ok!\n");
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
        if ( (v & 0x02) == 2) {
            dbk->mask_count --;
        }else{
            dbk->visible_count --;
        }
        node->used--;
    }

	if (dbk->mask_count + dbk->visible_count == 0) {
		// Fixme: remove the null block
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

    if ( (v & 0x01) == 1) { // data not real delete
        DINFO("tag data:%x\n", *data);
        if (tag == MEMLINK_TAG_DEL) {
            *data |= 0x02; 
        }else{
            *data &= 0xfd; 
        }
    
        DINFO("tag data:%x\n", *data);
            
        if ( (v & 0x02) == 2 && tag == MEMLINK_TAG_RESTORE) {
            dbk->mask_count --;
            dbk->visible_count ++;
        }

        if ( (v & 0x02) == 0 && tag == MEMLINK_TAG_DEL) {
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
	
	if (node->masknum != masknum) {
		return MEMLINK_ERR_MASK;
	}

    char mask[HASHTABLE_MASK_MAX_ITEM * sizeof(int)] = {0};
    char maskflag[HASHTABLE_MASK_MAX_ITEM * sizeof(int)] = {0};

    int  len = mask_array2binary(node->maskformat, maskarray, masknum, mask);
    if (len <= 0) {
        DERROR("mask array2binary error: %d\n", masknum);
        return MEMLINK_ERR_MASK;
    }
    //char buf[128];
    //DINFO("array2bin: %s\n", formatb(mask, len, buf, 128));

    int flen = mask_array2flag(node->maskformat, maskarray, masknum, maskflag);
    if (flen <= 0) {
        DERROR("mask array2flag error: %d\n", masknum);
        return MEMLINK_ERR_MASK;
    }
    //DINFO("array2flag: %s\n", formatb(maskflag, flen, buf, 128));

    dataitem_copy_mask(node, item, maskflag, mask);

    return MEMLINK_OK;
}

static int
mask_array2_binary_flag(unsigned char *maskformat, unsigned int *maskarray, int masknum, char *maskval, char *maskflag)
{
    int j;
    for (j = 0; j < masknum; j++) {
        if (maskarray[j] != UINT_MAX)
            break;
    }
    //DINFO("j: %d, masknum: %d\n", j, masknum);
    
    int masklen;
    if (j < masknum) {
        masklen = mask_array2binary(maskformat, maskarray, masknum, maskval);
        if (masklen <= 0) {
            DERROR("mask_string2array error\n");
            return -2;
        }
        maskval[0] = maskval[0] & 0xfc;

        //char buf[128];
        //DINFO("2binary: %s\n", formatb(maskval, masklen, buf, 128));
        masklen = mask_array2flag(maskformat, maskarray, masknum, maskflag);
        if (masklen <= 0) {
            DERROR("mask_array2flag error\n");
            return -2;
        }
        //DINFO("2flag: %s\n", formatb(maskflag, masklen, buf, 128));
        for (j = 0; j < masknum; j++) {
            maskflag[j] = ~maskflag[j];
        }
        maskflag[0] = maskflag[0] & 0xfc;

        //DINFO("^2flag: %s\n", formatb(maskflag, masklen, buf, 128));
    }else{
        masknum = 0;
    }

    return masknum;
}



int
hashtable_range(HashTable *ht, char *key, unsigned int *maskarray, int masknum, 
                int frompos, int len, 
                char *data, int *datanum, 
                unsigned char *valuesize, unsigned char *masksize)
{
    char maskval[HASHTABLE_MASK_MAX_ITEM * sizeof(int)];
	char maskflag[HASHTABLE_MASK_MAX_ITEM * sizeof(int)];
    HashNode    *node;
	int			startn;
	
	if (len <= 0) {
		return MEMLINK_ERR_RANGE_SIZE;
	}

    node = hashtable_find(ht, key);
    if (NULL == node) {
        DERROR("hashtable_find not found node\n");
        return MEMLINK_ERR_NOKEY;
    }

    if (masknum > 0) {
        masknum = mask_array2_binary_flag(node->maskformat, maskarray, masknum, maskval, maskflag);
    }

    DataBlock *dbk = NULL;
    int datalen = node->valuesize + node->masksize;
	char *addr  = NULL;

    *valuesize = node->valuesize;
    *masksize  = node->masksize;

    if (masknum > 0) {
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

    memcpy(data, &node->masknum, sizeof(char));
    idx += sizeof(char);
    memcpy(data + idx, node->maskformat, node->masknum);
    idx += node->masknum;

	DINFO("skipn: %d\n", skipn);
    while (dbk) {
        char *itemdata = dbk->data;
        int  i;
        for (i = 0; i < g_cf->block_data_count; i++) {
			if (addr) {
				if (itemdata != addr) {
					skipn -= 1;
					itemdata += datalen;
					continue;
				}else{
					addr = NULL;
				}
			}
            if (dataitem_have_data(node, itemdata, 1)) {
				char *maskdata = itemdata + node->valuesize;

				if (masknum > 0) {
					int k;
					for (k = 0; k < node->masksize; k++) {
						if ((maskdata[k] & maskflag[k]) != maskval[k]) {
							break;
						}
					}
					if (k < node->masksize) { // not equal
						//DINFO("not equal.\n");
						itemdata += datalen;
						continue;
					}
				}
				if (skipn > 0) {
					DINFO("== skipn: %d\n", skipn);
					skipn -= 1;
					itemdata += datalen;
					continue;
				}
			
#ifdef RANGE_MASK_STR
				char            maskstr[256];
				unsigned char   mlen;

                mlen = mask_binary2string(node->maskformat, node->masknum, maskdata, node->masksize, maskstr);

                /*
				char buf[128];
				snprintf(buf, node->valuesize + 1, "%s", itemdata);
				DINFO("ok, copy item ... i:%d, value:%s maskstr:%s, mlen:%d\n", i, buf, maskstr, mlen);
                */

				memcpy(data + idx, itemdata, node->valuesize);
                idx += node->valuesize;
                memcpy(data + idx, &mlen, sizeof(char));
                idx += sizeof(char);
                memcpy(data + idx, maskstr, mlen);
                idx += mlen;
#else
                /*
                char buf[128];
				snprintf(buf, node->valuesize + 1, "%s", itemdata);
				DINFO("ok, copy item ... i:%d, value:%s\n", i, buf);
                */
				
				memcpy(data + idx, itemdata, datalen);
				idx += datalen;
#endif
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
		
	DINFO("=== hashtable clean: %s ===\n", key);
    if (node->used == 0) {
        DataBlock *tmp;
        while (dbk) {
            tmp = dbk; 
            dbk = dbk->next;
            mempool_put(g_runtime->mpool, tmp, dlen);
        }
        node->all  = 0;
        node->data = NULL;
        return MEMLINK_OK;
    }

    DataBlock   *newroot = NULL, *newlast = NULL;
	DataBlock	*linklast = NULL;
    DataBlock   *oldbk, *tmp;

    char        *itemdata; // = dbk->data;
    int         i, dataall = 0;

    DataBlock *newdbk     = NULL;
    char      *newdbk_end = NULL;
    char      *newdbk_pos = NULL;
	int		  count = 0;

    while (dbk) {
        itemdata = dbk->data;
        for (i = 0; i < g_cf->block_data_count; i++) {
            if (dataitem_have_data(node, itemdata, 0)) {
                if (newdbk == NULL || newdbk_pos >= newdbk_end) {
                    newlast    = newdbk;
                    newdbk     = mempool_get(g_runtime->mpool, dlen);
                    newdbk_end = newdbk->data + g_cf->block_data_count * dlen;
                    newdbk_pos = newdbk->data;

                    newdbk->next  = NULL;

					if (newlast) {
						newlast->next = newdbk; 
					}else{
						newroot = newdbk;
					}

                    dataall += g_cf->block_data_count;
					count += 1;
                }
                unsigned char v = *(itemdata + node->valuesize) & 0x02;
                memcpy(newdbk_pos, itemdata, dlen);

                /*char buf[16] = {0};
                memcpy(buf, itemdata, node->valuesize);
                DINFO("clean copy item: %s\n", buf);
				*/
                newdbk_pos += dlen;
                if (v == 2) {
                    newdbk->mask_count++;
                }else{
                    newdbk->visible_count++;
                }
            }
            itemdata += dlen;
        }
        if (count > 0 && count % g_cf->block_clean_num == 0) {
            newdbk->next = dbk->next;
            if (linklast == NULL) { 
                oldbk = node->data;
                node->data = newroot;
            }else{
                oldbk = linklast->next; 
                linklast->next = newroot;
            }
            linklast = newdbk;

            while (oldbk && oldbk != dbk) {
                tmp = oldbk->next;
                mempool_put(g_runtime->mpool, oldbk, dlen);
                oldbk = tmp;
            }
			newdbk  = NULL;
			newroot = NULL;
        }
        dbk = dbk->next;
    }

    if (linklast == NULL) {
        oldbk = node->data;
        node->data = newroot;
    }else{
        oldbk = linklast->next;
		linklast->next = newroot;
    }
    while (oldbk && oldbk != dbk) {
        tmp = oldbk->next;
        mempool_put(g_runtime->mpool, oldbk, dlen);
        oldbk = tmp;
    }

    node->all = dataall;

    return MEMLINK_OK;
} 

int
hashtable_stat(HashTable *ht, char *key, HashTableStat *stat)
{
    HashNode    *node;

    node = hashtable_find(ht, key);
    if (NULL == node) {
        DWARNING("hashtable_stat not found key: %s\n", key);
        return MEMLINK_ERR_NOKEY;
    }

    DINFO("node all: %d, used: %d\n", node->all, node->used);
    stat->valuesize = node->valuesize;
    stat->masksize  = node->masksize;
    stat->blocks    = node->all / g_cf->block_data_count;
    stat->data      = node->all;
    stat->data_used = node->used;
    stat->mem       = 0;
    //stat->mem_used  = 0;

    int blockmem = sizeof(DataBlock) + (node->masksize + node->valuesize) * g_cf->block_data_count;
    stat->mem = stat->blocks * blockmem + sizeof(HashNode);

	/*
    DataBlock *dbk = node->data;
    while (dbk) {
        stat->blocks += 1;
        dbk = dbk->next;
    }*/

    //stat->mem_used = stat->blocks * sizeof(DataBlock) + (node->masksize + node->valuesize) * stat->data_used;
    
    DINFO("valuesize:%d, masksize:%d, blocks:%d, data:%d, data_used:%d, mem:%d\n", 
            stat->valuesize, stat->masksize, stat->blocks, stat->data, stat->data_used,
            stat->mem);

    return MEMLINK_OK;
}

int
hashtable_count(HashTable *ht, char *key, unsigned int *maskarray, int masknum, int *visible_count, int *mask_count)
{
    char maskval[HASHTABLE_MASK_MAX_ITEM * sizeof(int)];
	char maskflag[HASHTABLE_MASK_MAX_ITEM * sizeof(int)];
    HashNode    *node;
    int         vcount = 0, mcount = 0;

    node = hashtable_find(ht, key);
    if (NULL == node) {
        DERROR("hashtable_add not found node\n");
        return MEMLINK_ERR_NOKEY;
    }

    if (masknum > 0) {
        masknum = mask_array2_binary_flag(node->maskformat, maskarray, masknum, maskval, maskflag);
    }

    if (masknum > 0) {
        int i, ret, k;
        int datalen = node->valuesize + node->masksize;
        DataBlock *dbk = node->data;
        while (dbk) {
            char *itemdata = dbk->data;
            for (i = 0; i < g_cf->block_data_count; i++) {
				//DINFO("dbk:%p node:%p, itemdata:%p\n", dbk, node, itemdata);
                ret = dataitem_check_data(node, itemdata);
                if (ret != MEMLINK_ITEM_REMOVED) {
                    char *maskdata = itemdata + node->valuesize;
                    // check mask equal
                    for (k = 0; k < node->masksize; k++) {
                        if ((maskdata[k] & maskflag[k]) != maskval[k]) {
                            break;
                        }
                    }
                    if (k < node->masksize) { // not equal
						itemdata += datalen;
                        continue;
                    }
                    if (ret == MEMLINK_ITEM_VISIBLE) {
                        vcount ++;
                    }else{
                        mcount ++;
                    }
                }
                itemdata += datalen;
            }
            dbk = dbk->next;
        }
    }else{
        DataBlock *dbk = node->data;
        while (dbk) {
            vcount += dbk->visible_count;
            mcount += dbk->mask_count;
            dbk = dbk->next;
        }
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


