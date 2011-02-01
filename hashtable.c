#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include "zzmalloc.h"
#include "logfile.h"
#include "hashtable.h"
#include "serial.h"
#include "utils.h"
#include "datablock.h"
#include "common.h"

int
hashtable_find_value(HashTable *ht, char *key, void *value, HashNode **node, DataBlock **dbk, char **data)
{
    HashNode *fnode = hashtable_find(ht, key);

    if (NULL == fnode) {
        return MEMLINK_ERR_NOKEY;
    }
    if (node) {
        *node = fnode;
    }

    //DINFO("find dataitem ... node: %p\n", fnode);
    char *item = dataitem_lookup(fnode, value, dbk);
    if (NULL == item) {
        //DWARNING("dataitem_lookup error: %s, %x\n", key, *(unsigned int*)value);
        return MEMLINK_ERR_NOVAL;
    }
	//DINFO("find dataitem ok!\n");
    *data = item;

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
    memset(ht, 0, sizeof(HashTable));    
    //ht->bunks = (HashNode**)zz_malloc(sizeof(HashNode*) * HASHTABLE_BUNKNUM);
    //memset(ht->bunks, 0, sizeof(HashNode*) * HASHTABLE_BUNKNUM);

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
hashtable_key_create(HashTable *ht, char *key, int valuesize, char *maskstr, unsigned char listtype, unsigned char valuetype)
{
    unsigned int format[HASHTABLE_MASK_MAX_ITEM] = {0};
    int  masknum = mask_string2array(maskstr, format);
    int  i; 
    
    if (masknum > HASHTABLE_MASK_MAX_ITEM || masknum <= 0) {
        return MEMLINK_ERR_MASK;
    }

    for (i = 0; i < masknum; i++) {
        if (format[i] == UINT_MAX || format[i] > HASHTABLE_MASK_MAX_BIT) {
            DINFO("maskformat error: %s\n", maskstr);
            return MEMLINK_ERR_MASK;
        }
    }

    return hashtable_key_create_mask(ht, key, valuesize, format, masknum, listtype, valuetype);
}

int
hashtable_key_create_mask(HashTable *ht, char *key, int valuesize, 
                        unsigned int *maskarray, char masknum, unsigned char listtype, unsigned char valuetype)
{
    int             keylen = strlen(key);
    unsigned int    hash   = hashtable_hash(key, keylen);
    HashNode        *node  = ht->bunks[hash];
  
    DINFO("hashtable_key_create_mask call ... %s, %p, hash: %d\n", key, node, hash);
    while (node) {
        //DINFO("node->key: %p\n", node);
        //DINFO("node->key: %p, valuesize: %d\n", node, node->valuesize);
        if (strcmp(node->key, key) == 0) {
            return MEMLINK_ERR_EKEY;
        }
        node = node->next;
    }
    
    node = (HashNode*)zz_malloc(sizeof(HashNode));
    memset(node, 0, sizeof(HashNode));
    
    node->key  = zz_strdup(key);
    node->data = NULL;
    node->next = ht->bunks[hash];
    node->valuesize = valuesize;
    node->type = listtype;
    node->valuetype = valuetype;

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
        for (i = 0; i < node->masknum; i++) {
            node->maskformat[i] = maskarray[i];
        }
    }
    //DINFO("node key: %s, format: %d,%d,%d\n", node->key, node->maskformat[0], node->maskformat[1], node->maskformat[2]);
    ht->bunks[hash] = node;
    //DINFO("ht key:%s, hash:%d\n", ht->bunks[hash]->key, hash);

    return MEMLINK_OK;    
}

int
hashtable_remove_list(HashTable *ht, char *key)
{
    int             keylen; // = strlen(key);
    unsigned int    hash; //   = hashtable_hash(key, keylen);
    HashNode        *node; //  = ht->bunks[hash];

	if (NULL == key) {
		return MEMLINK_ERR_KEY;
	}
	keylen = strlen(key);
	hash   = hashtable_hash(key, keylen);
	node   = ht->bunks[hash];

	node = hashtable_find(ht, key);
	if (NULL == node) {
		return MEMLINK_ERR_NOKEY;
	}
	
	DataBlock	*dbk = node->data;
	DataBlock	*tmp;
	int			datalen = node->valuesize + node->masksize;

	node->data      = NULL;
	node->data_tail = NULL;
	node->used      = 0;
	node->all       = 0;
	
	while (dbk) {
		tmp = dbk;
		dbk = dbk->next;
		mempool_put(g_runtime->mpool, tmp, sizeof(DataBlock) + tmp->data_count * datalen);	
	}
	return MEMLINK_OK;
}

int
hashtable_remove_key(HashTable *ht, char *key)
{
    int             keylen; // = strlen(key);
    unsigned int    hash; //   = hashtable_hash(key, keylen);
    HashNode        *node; //  = ht->bunks[hash];
	HashNode		*last  = NULL;

	if (NULL == key) {
		return MEMLINK_ERR_KEY;
	}
	keylen = strlen(key);
	hash   = hashtable_hash(key, keylen);
	node   = ht->bunks[hash];

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
    zz_free(node->maskformat);
	zz_free(node);
	
	while (dbk) {
		tmp = dbk;
		dbk = dbk->next;
		mempool_put(g_runtime->mpool, tmp, sizeof(DataBlock) + tmp->data_count * datalen);	
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

int
hashtable_add(HashTable *ht, char *key, void *value, char *maskstr, int pos)
{
    unsigned int maskarray[HASHTABLE_MASK_MAX_ITEM] = {0};
    char masknum;

    masknum = mask_string2array(maskstr, maskarray);
    if (masknum <= 0 || masknum > HASHTABLE_MASK_MAX_ITEM) {
        DINFO("mask_string2array error: %s\n", maskstr);
        return MEMLINK_ERR_MASK;
    }

    return hashtable_add_mask(ht, key, value, maskarray, masknum, pos);
}

int
hashtable_add_mask_bin(HashTable *ht, char *key, void *value, void *mask, int pos)
{
    HashNode *node = hashtable_find(ht, key);
    if (NULL == node) {
        DINFO("hashtable_add_mask_bin not found node for key:%s\n", key);
        return MEMLINK_ERR_NOKEY;
    }
    /*char bufv[512] = {0}; 
    memcpy(bufv, value, node->valuesize);
    DINFO("add_mask_bin: %s, %s\n", key, bufv);
    */

    DataBlock *dbk = node->data;
	DataBlock *newbk = NULL;
    int datalen = node->valuesize + node->masksize;
    int i, startn, skipn;
    DataBlock *prev = NULL;

	int ret = 0;
    int dbkpos = 0; 

    if (pos >= node->used) {
        dbk = node->data_tail;
        if (dbk == NULL)
            ret = -1;
    }else{
        ret = datablock_lookup_valid_pos(node, pos, 0, &dbk);
        dbkpos = dataitem_skip2pos(node, dbk, pos, MEMLINK_VALUE_ALL);

        startn = ret;
        skipn = pos - startn;
    }

    if (dbk) {
        prev = dbk->prev;
    }

	DINFO("lookup pos, ret:%d, pos:%d, dbk:%p, prev:%p\n", ret, pos, dbk, prev);
	if (ret == -1) { // no datablock, create a small datablock
		DINFO("create first small dbk.\n");
		newbk = datablock_new_copy_small(node, value, mask);
        newbk->prev = NULL;
		node->data = newbk;
        node->data_tail = newbk;
		node->used++;
		node->all += newbk->data_count;
		return MEMLINK_OK;
	}

    if (dbk->next == NULL) { // last datablock
		DINFO("last dbk! %p\n", dbk);
		if (dbk->data_count == 1) { // last is a small datablock
			DINFO("dbk is small.\n");
			if (dbk->visible_count + dbk->tagdel_count == 0) { // datablock have no data
				DINFO("dbk is null, only copy!\n");
				dataitem_copy(node, dbk->data, value, mask);
				dbk->visible_count = 1;
				node->used++;

				return MEMLINK_OK;
			}
			DINFO("create a bigger dbk.\n");
			// datablock have data, create a bigger datablock
			//newbk = datablock_new_copy(node, dbk, skipn, value, mask);
			newbk = datablock_new_copy_pos(node, dbk, dbkpos, value, mask);
            node->data_tail = newbk;
			node->all -= dbk->data_count;
			node->all += newbk->data_count;
			mempool_put(g_runtime->mpool, dbk, sizeof(DataBlock) + dbk->data_count * datalen);
		}else{ // last datablock is not a small one
			if (dbk->visible_count + dbk->tagdel_count == dbk->data_count) { // datablock is full
				DINFO("last dbk is full, append a new small\n");
				// create a new small datablock	
				//if (skipn >= dbk->data_count) { // pos out of block
                if (dbkpos > 0) {
					newbk = datablock_new_copy_small(node, value, mask);
                    newbk->prev = dbk;
                    dbk->next = newbk;
                    node->data_tail = newbk;
					node->all += newbk->data_count;
					prev = dbk; // only append
				}else{
					//newbk = datablock_new_copy(node, dbk, skipn, value, mask);
					newbk = datablock_new_copy_pos(node, dbk, dbkpos, value, mask);
					DataBlock *newbk2 = datablock_new_copy_small(node, NULL, NULL);
					memcpy(newbk2->data, dbk->data + (dbk->data_count - 1) * datalen, datalen);
                    newbk2->prev = newbk;
					newbk2->visible_count = 1; //add by wyx 1/5 18:00
					newbk->next = newbk2;
					node->all += newbk2->data_count;
				    node->data_tail = newbk2;	
					mempool_put(g_runtime->mpool, dbk, sizeof(DataBlock) + dbk->data_count * datalen);
				}
			}else{ // datablock not full
				DINFO("last dbk not full, copy and mix!\n");
				//if (datablock_check_idle(node, dbk, skipn, value, mask)) {
				if (datablock_check_idle_pos(node, dbk, dbkpos, value, mask)) {
					DINFO("dbk pos is null, only copy\n");
					return MEMLINK_OK;
				}

				//newbk = datablock_new_copy(node, dbk, skipn, value, mask);
				newbk = datablock_new_copy_pos(node, dbk, dbkpos, value, mask);
                node->data_tail = newbk;
				mempool_put(g_runtime->mpool, dbk, sizeof(DataBlock) + dbk->data_count * datalen);
			}
		}
		
		node->used++;
		if (NULL == prev) {
			node->data = newbk;
		}else{
			prev->next = newbk;
		}

		return MEMLINK_OK;
	}
	
    DataBlock   *newbk2 = NULL;
	// not last datablock
	DINFO("not last dbk.\n");
	if (dbk->visible_count + dbk->tagdel_count == dbk->data_count) { // datablock is full
		DINFO("dbk is full.\n");
	    //newbk = datablock_new_copy(node, dbk, skipn, value, mask);
	    newbk = datablock_new_copy_pos(node, dbk, dbkpos, value, mask);
		DataBlock *nextbk = dbk->next;		

		if (dataitem_have_data(node, nextbk->data, 0) == MEMLINK_FALSE) { // first data in next block is null
			DINFO("nextdbk first data is null. %p\n", nextbk);
			//dataitem_copy(node, nextbk->data, value, mask);
			memcpy(nextbk->data, dbk->data + (dbk->data_count - 1) * datalen, datalen);
			nextbk->visible_count++;
		}else{
			DINFO("nextdbk first not null\n");
			newbk2 = mempool_get(g_runtime->mpool, sizeof(DataBlock) + g_cf->block_data_count * datalen);
			newbk2->data_count = g_cf->block_data_count;
            newbk2->prev = newbk;
            newbk2->next = newbk->next;
            newbk->next = newbk2;
			DINFO("create newbk2:%p\n", newbk2);
			memcpy(newbk2->data, dbk->data + (dbk->data_count - 1) * datalen, datalen);
			newbk2->visible_count = 1;

			newbk->next = newbk2;

			if (nextbk->data_count != 1 && nextbk->visible_count + nextbk->tagdel_count == nextbk->data_count) { // nextbk is full
				DINFO("nextdbk is full.\n");
				// create a new datablock, and only one data in first position
				node->all += newbk2->data_count;
				newbk2->next = nextbk;
			}else{ // nextbk not full
				char *todata     = newbk2->data + datalen;
				//char *to_enddata = newbk2->data + newbk2->data_count * datalen;
				char *fromdata   = nextbk->data;

				//DINFO("nextdbk is not full. %d\n", (to_enddata - todata) / datalen);
				//int ret;
				for (i = 0; i < nextbk->data_count; i++) {
					ret = dataitem_check_data(node, fromdata);
					if (ret != MEMLINK_VALUE_REMOVED) {
						memcpy(todata, fromdata, datalen);
						todata += datalen;

						if (ret == MEMLINK_VALUE_VISIBLE) {
							newbk2->visible_count++;
						}else{
							newbk2->tagdel_count++;
						}
					}
					fromdata += datalen;
				}
				node->all -= nextbk->data_count;
				node->all += newbk2->data_count;
				newbk2->next = nextbk->next;

                if (newbk2->next == NULL) {
                    node->data_tail = newbk2;
                }
				mempool_put(g_runtime->mpool, nextbk, sizeof(DataBlock) + nextbk->data_count * datalen);
			}
		}

	}else{ // datablock is not full
		DINFO("dbk not null\n");
        //if (datablock_check_idle(node, dbk, skipn, value, mask)) {
        if (datablock_check_idle_pos(node, dbk, dbkpos, value, mask)) {
			DINFO("dbk pos is null, only copy\n");
            return MEMLINK_OK;
        }
	    //newbk = datablock_new_copy(node, dbk, skipn, value, mask);
	    newbk = datablock_new_copy_pos(node, dbk, dbkpos, value, mask);
    }

	node->used++;
	if (NULL == prev) {
		node->data = newbk;
	}else{
		prev->next = newbk;
	}
    
    if (newbk2) {
        if (newbk2->next) {
            newbk2->next->prev = newbk2;
        }
    }else{
        if (newbk->next) {
            newbk->next->prev = newbk; 
        }
    }

	mempool_put(g_runtime->mpool, dbk, sizeof(DataBlock) + dbk->data_count * datalen);

    return MEMLINK_OK;
}

/**
 * add new value at a specially position
 */
int
hashtable_add_mask(HashTable *ht, char *key, void *value, unsigned int *maskarray, char masknum, int pos)
{
    char mask[HASHTABLE_MASK_MAX_ITEM * HASHTABLE_MASK_MAX_BYTE] = {0};
    int  ret;
    HashNode *node = hashtable_find(ht, key);
    if (NULL == node) {
        DINFO("hashtable_add_mask not found node for key:%s\n", key);
        return MEMLINK_ERR_NOKEY;
    }
   
	if (node->masknum != masknum) {
		return MEMLINK_ERR_MASK;
	}
    ret = mask_array2binary(node->maskformat, maskarray, masknum, mask);
    if (ret <= 0) {
        DINFO("mask_array2binary error: %d\n", ret);
        return MEMLINK_ERR_MASK;
    }

    //printh(mask, node->masksize);
    return hashtable_add_mask_bin(ht, key, value, mask, pos);
}


int
hashtable_sortlist_add_mask_bin(HashTable *ht, char *key, void *value, void *mask)
{
    return 0;
}

int
hashtable_sortlist_add_mask(HashTable *ht, char *key, void *value, unsigned int *maskarray, char masknum)
{
    char mask[HASHTABLE_MASK_MAX_ITEM * HASHTABLE_MASK_MAX_BYTE] = {0};
    int  ret;
    HashNode *node = hashtable_find(ht, key);
    if (NULL == node) {
        DINFO("hashtable_add_mask not found node for key:%s\n", key);
        return MEMLINK_ERR_NOKEY;
    }
   
	if (node->masknum != masknum) {
		return MEMLINK_ERR_MASK;
	}
    ret = mask_array2binary(node->maskformat, maskarray, masknum, mask);
    if (ret <= 0) {
        DINFO("mask_array2binary error: %d\n", ret);
        return MEMLINK_ERR_MASK;
    }

    return hashtable_sortlist_add_mask_bin(ht, key, value, mask);
}

/**
 * move value to another position
 */
int
hashtable_move(HashTable *ht, char *key, void *value, int pos)
{
    char mask[HASHTABLE_MASK_MAX_ITEM * HASHTABLE_MASK_MAX_BYTE] = {0};
    HashNode *node = NULL;
    DataBlock *dbk = NULL, *prev = NULL;
    char     *item = NULL; 
	
    char ret = hashtable_find_value(ht, key, value, &node, &dbk, &item);
    if (ret < 0) {
        //DWARNING("hashtable_move not found value, ret:%d, key:%s\n", ret, key);
        return ret;
    }
	
	DINFO("move find dbk:%p, prev:%p\n", dbk, prev);
    char *mdata = item + node->valuesize;
    memcpy(mask, mdata, node->masksize);  

    datablock_del(node, dbk, item);

    int retv = hashtable_add_mask_bin(ht, key, value, mask, pos);
    if (retv != MEMLINK_OK) {
        datablock_del_restore(node, dbk, item);
        return retv;
    }
   
    // remove null block
	//DINFO("dbk:%p, count:%d, prev:%p, dbk->next:%p\n", dbk, dbk->tagdel_count + dbk->visible_count, prev, dbk->next);
	if (dbk->tagdel_count + dbk->visible_count == 0) {
        if (dbk->prev) {
            dbk->prev->next = dbk->next;
        }else{
            node->data = dbk->next;		
        }
        if (dbk->next) {
            dbk->next->prev = dbk->prev;
        }else{
            node->data_tail = dbk->prev;
        }
        DINFO("move release null block: %p\n", dbk);
        node->all -= dbk->data_count;

        mempool_put(g_runtime->mpool, dbk, sizeof(DataBlock) + dbk->data_count * (node->valuesize + node->masksize));
	}
	
    return MEMLINK_OK;
}


/**
 * remove a value, only write a flag to 0
 */
int 
hashtable_del(HashTable *ht, char *key, void *value)
{
    char        *item = NULL;
    HashNode    *node = NULL;
    DataBlock   *dbk  = NULL, *prev = NULL;

    DINFO("hashtable_find_value: %s, value: %s\n", key, (char*)value);
    int ret = hashtable_find_value(ht, key, value, &node, &dbk, &item);
    //DINFO("hashtable_find_value ret: %d, dbk:%p, prev:%p\n", ret, dbk, prev);
    if (ret < 0) {
        DINFO("hashtable_del find value error! %s\n", key);
        return ret;
    }
    char *data = item + node->valuesize;
    unsigned char v = *data & 0x3; 

    *data &= 0xfe;
    if ( (v & 0x01) == 1) {
        if ( (v & 0x02) == 2) {
            dbk->tagdel_count --;
        }else{
            dbk->visible_count --;
        }
        node->used--;
    }

	if (dbk->tagdel_count + dbk->visible_count == 0) {
		DINFO("del release null block: %p\n", dbk);
        prev = dbk->prev;
		if (prev) {
			prev->next = dbk->next;
		}else{
			node->data = dbk->next;		
		}
        if (dbk->next) {
            dbk->next->prev = prev;
        }else{
            node->data_tail = prev;
        }
		//modified by wyx 1/5 16:20
		node->all -= dbk->data_count;
		mempool_put(g_runtime->mpool, dbk, sizeof(DataBlock) + dbk->data_count * (node->valuesize + node->masksize));
	}

    return MEMLINK_OK;
}

/**
 * tag delete/restore, only change the flag to 1 or 0
 */
int
hashtable_tag(HashTable *ht, char *key, void *value, unsigned char tag)
{
    char        *item = NULL;
    HashNode    *node = NULL;
    DataBlock   *dbk  = NULL;

    int ret = hashtable_find_value(ht, key, value, &node, &dbk, &item);
    if (ret < 0) {
        DINFO("hashtable_tag not found key: %s\n", key);
        return ret;
    }
    //DINFO("tag: %d, %d\n", tag, tag<<1);
    
    char *data = item + node->valuesize;
    unsigned char v = *data & 0x3; 

    //DINFO("tag v:%x\n", v);
    if ( (v & 0x01) == 1) { // data not real delete
        //DINFO("tag data:%x\n", *data);
        if (tag == MEMLINK_TAG_DEL) {
            *data |= 0x02; 
        }else{
            *data &= 0xfd; 
        }
    
        //DINFO("tag data:%x\n", *data);
        if ( (v & 0x02) == 2 && tag == MEMLINK_TAG_RESTORE) {
            dbk->tagdel_count --;
            dbk->visible_count ++;
        }
        if ( (v & 0x02) == 0 && tag == MEMLINK_TAG_DEL) {
            dbk->tagdel_count ++;
            dbk->visible_count --;
        }
    }else{
        return MEMLINK_ERR_REMOVED;
    }

    return MEMLINK_OK;
}

/**
 * change mask for a value
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

    char mask[HASHTABLE_MASK_MAX_ITEM * HASHTABLE_MASK_MAX_BYTE] = {0};
    char maskflag[HASHTABLE_MASK_MAX_ITEM * HASHTABLE_MASK_MAX_BYTE] = {0};

    int  len = mask_array2binary(node->maskformat, maskarray, masknum, mask);
    if (len <= 0) {
        DINFO("mask array2binary error: %d\n", masknum);
        return MEMLINK_ERR_MASK;
    }
    //char buf[128];
    //DINFO("array2bin: %s\n", formatb(mask, len, buf, 128));

    int flen = mask_array2flag(node->maskformat, maskarray, masknum, maskflag);
    if (flen <= 0) {
        DINFO("mask array2flag error: %d\n", masknum);
        return MEMLINK_ERR_MASK;
    }
    //DINFO("array2flag: %s\n", formatb(maskflag, flen, buf, 128));
    dataitem_copy_mask(node, item, maskflag, mask);

    return MEMLINK_OK;
}


int
hashtable_range(HashTable *ht, char *key, unsigned char kind, 
				unsigned int *maskarray, int masknum, 
                int frompos, int len, Conn *conn)
{
    char maskval[HASHTABLE_MASK_MAX_ITEM * HASHTABLE_MASK_MAX_BYTE] = {0};
	char maskflag[HASHTABLE_MASK_MAX_ITEM * HASHTABLE_MASK_MAX_BYTE] = {0};

    HashNode    *node;
	int			startn;
    int         idx   = 0;
    char        *wbuf = NULL;
    int         ret   = 0;
	
	if (len <= 0) {
		ret =  MEMLINK_ERR_RANGE_SIZE;
        goto hashtable_range_over;
	}

    node = hashtable_find(ht, key);
    if (NULL == node) {
        DINFO("hashtable_find not found node for key:%s\n", key);
        ret = MEMLINK_ERR_NOKEY;
        goto hashtable_range_over;
    }
    if (masknum > 0) {
        masknum = mask_array2_binary_flag(node->maskformat, maskarray, masknum, maskval, maskflag);
    }

    int  wlen = CMD_REPLY_HEAD_LEN + 3 + node->masknum + (node->valuesize + node->masksize) * len;
    DINFO("range wlen: %d\n", wlen);
    wbuf = conn_write_buffer(conn, wlen);
    idx += CMD_REPLY_HEAD_LEN;
    //DINFO("valuesize:%d, masksize:%d, masknum:%d\n", node->valuesize, node->masksize, node->masknum);
    memcpy(wbuf + idx, &node->valuesize, sizeof(char));
    idx += sizeof(char);
    memcpy(wbuf + idx, &node->masksize, sizeof(char));
    idx += sizeof(char);
    memcpy(wbuf + idx, &node->masknum, sizeof(char));
    idx += sizeof(char);
    memcpy(wbuf + idx, node->maskformat, node->masknum);
    idx += node->masknum;

    DataBlock *dbk = NULL;
    int datalen = node->valuesize + node->masksize;
    int dbkpos = 0;

    if (masknum > 0) {
		startn = dataitem_lookup_pos_mask(node, frompos, kind, maskval, maskflag, &dbk, &dbkpos);
		DINFO("dataitem_lookup_pos_mask startn:%d, dbkpos:%d\n", startn, dbkpos);
		if (startn < 0) { // out of range
			ret = MEMLINK_OK;
            goto hashtable_range_over;
		}
    }else{
		startn = datablock_lookup_pos(node, frompos, kind, &dbk);
		DINFO("datablock_lookup_pos startn:%d, dbk:%p\n", startn, dbk);
		if (startn < 0) { // out of range
			ret = MEMLINK_OK;
            goto hashtable_range_over;
		}
        dbkpos = dataitem_skip2pos(node, dbk, frompos - startn, kind);
	}
    DINFO("dbkpos: %d, kind:%d\n", dbkpos, kind);
	
    int n = 0; 
    while (dbk) {
		//DINFO("dbk:%p, count:%d\n", dbk, dbk->data_count);
		if (dbk->data_count == 0) {
			DINFO("data_count is 0, dbk:%p\n", dbk);
			break;
		}

        char *itemdata = dbk->data + dbkpos * datalen;
        int  i;
        for (i = dbkpos; i < dbk->data_count; i++) {
            if (dbkpos > 0)
                dbkpos = 0;

			//modify by lanwenhong
            if (dataitem_have_data(node, itemdata, kind)) {
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
#ifdef RANGE_MASK_STR
				char            maskstr[256];
				unsigned char   mlen;

                mlen = mask_binary2string(node->maskformat, node->masknum, maskdata, node->masksize, maskstr);
                /*
				char buf[128];
				snprintf(buf, node->valuesize + 1, "%s", itemdata);
				DINFO("ok, copy item ... i:%d, value:%s maskstr:%s, mlen:%d\n", i, buf, maskstr, mlen);
                */
				memcpy(wbuf + idx, itemdata, node->valuesize);
                idx += node->valuesize;
                memcpy(wbuf + idx, &mlen, sizeof(char));
                idx += sizeof(char);
                memcpy(wbuf + idx, maskstr, mlen);
                idx += mlen;
#else
                /*char buf[128];
				snprintf(buf, node->valuesize + 1, "%s", itemdata);
				DINFO("\tok, copy item ... i:%d, value:%s\n", i, buf);
				*/ 
				memcpy(wbuf + idx, itemdata, datalen);
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
    ret = MEMLINK_OK;
	DINFO("count: %d\n", n);

hashtable_range_over:
    conn_write_buffer_head(conn, ret, idx);

    return ret;
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
    if (node->used == 0) { // remove all datablock
        DataBlock *tmp;
        while (dbk) {
            tmp = dbk; 
            dbk = dbk->next;
            mempool_put(g_runtime->mpool, tmp, sizeof(DataBlock) + tmp->data_count * dlen);
        }
        node->all  = 0;
        node->data = NULL;
        return MEMLINK_OK;
    }

    DataBlock   *newroot = NULL, *newlast = NULL;
	DataBlock	*linklast = NULL;
    DataBlock   *oldbk;

    char        *itemdata; // = dbk->data;
    int         i, dataall = 0;

    DataBlock *newdbk     = NULL;
    char      *newdbk_end = NULL;
    char      *newdbk_pos = NULL;
	int		  count = 0;
	int		  ret;

    while (dbk) {
        itemdata = dbk->data;
        for (i = 0; i < dbk->data_count; i++) {
			ret = dataitem_check_data(node, itemdata);
			if (ret != MEMLINK_VALUE_REMOVED) {
                if (newdbk == NULL || newdbk_pos >= newdbk_end) {
                    newlast    = newdbk;
                    newdbk     = mempool_get(g_runtime->mpool, sizeof(DataBlock) + g_cf->block_data_count * dlen);
                    newdbk_end = newdbk->data + g_cf->block_data_count * dlen;
                    newdbk_pos = newdbk->data;

					newdbk->data_count = g_cf->block_data_count;
                    newdbk->next	   = NULL;
                    newdbk->prev       = newlast;

					if (newlast) {
						newlast->next = newdbk; 
					}else{
						newroot = newdbk;
					}
                    dataall += g_cf->block_data_count;
					count += 1;
                }
                /*char buf[16] = {0};
                memcpy(buf, itemdata, node->valuesize);
                DINFO("clean copy item: %s\n", buf);
				*/
                memcpy(newdbk_pos, itemdata, dlen);
                newdbk_pos += dlen;
                if (ret == MEMLINK_VALUE_TAGDEL) {
                    newdbk->tagdel_count++;
                }else{
                    newdbk->visible_count++;
                }
            }
            itemdata += dlen;
        }
        if (count > 0 && count % g_cf->block_clean_num == 0) { // clean num arrivied
            newdbk->next = dbk->next;
            if (dbk->next) { 
                dbk->next->prev = newdbk;
            }
            if (linklast == NULL) { // first
                oldbk = node->data;
                node->data = newroot;
            }else{ // skip the last block, link to linklast->prev
                //oldbk = linklast->next; 
                oldbk = linklast; 
                linklast->prev->next = newroot;
                newroot->prev = linklast->prev;
            }
            linklast = newdbk;
            
            datablock_free(oldbk, dbk, dlen);

            // copy last datablock content to new link
            DataBlock *newdbk2  = mempool_get(g_runtime->mpool, sizeof(DataBlock) + g_cf->block_data_count * dlen);
            newdbk2->data_count = g_cf->block_data_count;
            datablock_copy(newdbk2, newdbk, dlen);
            newdbk_pos = newdbk2->data + (newdbk2->tagdel_count + newdbk2->visible_count) * dlen;
            newdbk_end = newdbk2->data + g_cf->block_data_count * dlen;

			newdbk  = newdbk2;
			newroot = newdbk2;
            newlast = NULL;
        }
        dbk = dbk->next;
    }

    if (linklast == NULL) {
        oldbk = node->data;
        node->data = newroot;
    }else{
        //oldbk = linklast->next;
        oldbk = linklast;
		linklast->prev->next = newroot;
        newroot->prev = linklast->prev;
    }
    node->data_tail = newdbk;

    datablock_free(oldbk, dbk, dlen);
    node->all = dataall;

    return MEMLINK_OK;
} 

int
hashtable_stat_sys(HashTable *ht, HashTableStatSys *stat)
{
	HashNode	*node;
	int			datalen = 0;
	int			node_block = 0;
	int			i;
	int			keys	= 0;
	int			blocks	= 0;
	int			data	= 0;
	int			datau   = 0;
	int			memu    = 0;

	//DINFO("sizeof DataBlock:%d, HashNode:%d\n", sizeof(DataBlock), sizeof(HashNode));
	for (i = 0; i < HASHTABLE_BUNKNUM; i++) {
		node = ht->bunks[i];
		if (node) {
			memu += strlen(node->key) + 1 + node->masknum;
		}
		while (node) {
			keys++;	
			node_block = node->all / g_cf->block_data_count;
			blocks += node_block;
			data   += node->all;
			datau  += node->used;
			
			datalen = node->masksize + node->valuesize;
			//DINFO("datalen: %d, key len:%d\n", datalen, strlen(node->key));
			memu   += node_block * (sizeof(DataBlock) + datalen * g_cf->block_data_count) + sizeof(HashNode);
			if (node->all % g_cf->block_data_count != 0) {
				memu += sizeof(DataBlock) + datalen;
				blocks++;
			}
			
			node = node->next;
		}
	}

	stat->keys   = keys;
	stat->values = datau;
	stat->blocks = blocks;
	stat->data   = data;
	stat->data_used    = datau;
	stat->block_values = g_cf->block_data_count;
	stat->ht_mem	   = memu;
	stat->pool_blocks  = g_runtime->mpool->blocks;

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

    int blockmem = sizeof(DataBlock) + (node->masksize + node->valuesize) * g_cf->block_data_count;
    stat->mem = stat->blocks * blockmem + sizeof(HashNode);

	// the last one is small datablock
	if (node->all % g_cf->block_data_count == 1) {
		stat->mem += sizeof(DataBlock) + node->masksize + node->valuesize;
		stat->blocks += 1;
	}

    DINFO("valuesize:%d, masksize:%d, blocks:%d, data:%d, data_used:%d, mem:%d\n", 
            stat->valuesize, stat->masksize, stat->blocks, stat->data, stat->data_used,
            stat->mem);

    return MEMLINK_OK;
}

int
hashtable_count(HashTable *ht, char *key, unsigned int *maskarray, int masknum, int *visible_count, int *tagdel_count)
{
    char maskval[HASHTABLE_MASK_MAX_ITEM * HASHTABLE_MASK_MAX_BYTE] = {0};
	char maskflag[HASHTABLE_MASK_MAX_ITEM * HASHTABLE_MASK_MAX_BYTE] = {0};
    HashNode    *node;
    int         vcount = 0, mcount = 0;

    node = hashtable_find(ht, key);
    if (NULL == node) {
        DINFO("hashtable_count not found node for key:%s\n", key);
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
			if (dbk->data_count == 0) {
				break;
			}
            char *itemdata = dbk->data;
            for (i = 0; i < dbk->data_count; i++) {
				//DINFO("dbk:%p node:%p, itemdata:%p\n", dbk, node, itemdata);
                ret = dataitem_check_data(node, itemdata);
                if (ret != MEMLINK_VALUE_REMOVED) {
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
                    if (ret == MEMLINK_VALUE_VISIBLE) {
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
            mcount += dbk->tagdel_count;
            dbk = dbk->next;
        }
    }

    *visible_count = vcount;
    *tagdel_count  = mcount;

    return MEMLINK_OK;
}

int 
hashtable_lpush(HashTable *ht, char *key, void *value, unsigned int *maskarray, char masknum)
{
    return hashtable_add_mask(ht, key, value, maskarray, masknum, 0);
}

int 
hashtable_rpush(HashTable *ht, char *key, void *value, unsigned int *maskarray, char masknum)
{
    return hashtable_add_mask(ht, key, value, maskarray, masknum, INT_MAX);
}

// pop value from head. queue not support tagdel, mask
int
hashtable_lpop(HashTable *ht, char *key, int num, Conn *conn)
{
    HashNode    *node;
    char        *wbuf = NULL;
    int         idx = 0;
    int         ret = MEMLINK_OK;
	
	if (num <= 0) {
		ret =  MEMLINK_ERR_PARAM;
        goto hashtable_lpop_end;
	}

    node = hashtable_find(ht, key);
    if (NULL == node) {
        DINFO("hashtable_find not found node for key:%s\n", key);
        ret = MEMLINK_ERR_NOKEY;
        goto hashtable_lpop_end;
    }

    int  wlen = CMD_REPLY_HEAD_LEN + 3 + node->masknum + (node->valuesize + node->masksize) * num;
    DINFO("range wlen: %d\n", wlen);
    if (conn) {
        //DINFO("conn is true.\n");
        wbuf = conn_write_buffer(conn, wlen);
    }
    idx += CMD_REPLY_HEAD_LEN;

    DataBlock *dbk = node->data;
    DataBlock *newdbk = NULL;
    DataBlock *startdbk = dbk, *enddbk = NULL;
    char      *itemdata;
    int datalen = node->valuesize + node->masksize;
    int n     = 0; 
    int i;
    int rmn   = 0;  // delete value count in whole block

    if (wbuf) {
        memcpy(wbuf + idx, &node->valuesize, sizeof(char));
        idx += sizeof(char);
        memcpy(wbuf + idx, &node->masksize, sizeof(char));
        idx += sizeof(char);
        memcpy(wbuf + idx, &node->masknum, sizeof(char));
        idx += sizeof(char);
        memcpy(wbuf + idx, node->maskformat, node->masknum);
        idx += node->masknum;
    }else{
        idx += 3 + node->masknum;
    }

    while (dbk) {
        itemdata = dbk->data;
        int dbkcpnv = 0;
        for (i = 0; i < dbk->data_count; i++) {
            ret = dataitem_check_data(node, itemdata);
            if (ret == MEMLINK_VALUE_VISIBLE) {
                /*char buf[128];
				snprintf(buf, node->valuesize + 1, "%s", itemdata);
				DINFO("\tok, copy item ... i:%d, value:%s\n", i, buf);
			    */	
                if (wbuf) {
				    memcpy(wbuf + idx, itemdata, datalen);
                }
				idx += datalen;
				n += 1;
                dbkcpnv++;
				if (n >= num) { // copy complete!
                    if (i < dbk->data_count - 1) {
                        newdbk = mempool_get(g_runtime->mpool, sizeof(DataBlock) + dbk->data_count * datalen);
                        newdbk->data_count    = dbk->data_count;
                        newdbk->visible_count = dbk->visible_count - dbkcpnv; 
                        char *todata = newdbk->data + datalen * (i + 1);
                        node->data   = newdbk;
                        newdbk->next = dbk->next;
                        if (dbk->next) {
                            dbk->next->prev = newdbk;
                        }
                        DINFO("copy todata from: %d, count:%d\n", i, dbk->data_count - i - 1);
                        memcpy(todata, itemdata + datalen, (dbk->data_count - i - 1) * datalen);
                        //enddbk = dbk;
                    }else{ // current datablock is all poped
                        rmn += dbk->data_count;
                        node->data = dbk->next;
                        if (dbk->next) {
                            dbk->next->prev = NULL;
                        }
                    }
                    enddbk = dbk->next; 

                    if (dbk->next == NULL) {
                        node->data_tail = newdbk;
                    }
                    node->all  -= rmn;
                    node->used -= n;
                    ret = MEMLINK_OK;
					goto hashtable_lpop_over;
				}
            }
            itemdata += datalen;
        }
        rmn += dbk->data_count;
        dbk = dbk->next;
    }
    enddbk     = NULL;
    node->all  = 0;
    node->used = 0;
    node->data = node->data_tail = NULL;
    ret = MEMLINK_OK;
    
hashtable_lpop_over:
	DINFO("count: %d\n", n);
    if (n > 0) {
        datablock_free(startdbk, enddbk, datalen);
    }

hashtable_lpop_end:
    if (conn) {
        conn_write_buffer_head(conn, ret, idx);
        //char bufw[4096] = {0};
        //DINFO("bufw:%s\n", formath(conn->wbuf, conn->wlen, bufw, 4096));
    }

    return ret;
}

int
hashtable_rpop(HashTable *ht, char *key, int num, Conn *conn)
{
    HashNode    *node;
    char        *wbuf = NULL;
    int         idx = 0;
    int         ret = MEMLINK_OK;
	
	if (num <= 0) {
		ret =  MEMLINK_ERR_PARAM;
        goto hashtable_rpop_end;
	}

    node = hashtable_find(ht, key);
    if (NULL == node) {
        DINFO("hashtable_find not found node for key:%s\n", key);
        ret = MEMLINK_ERR_NOKEY;
        goto hashtable_rpop_end;
    }

    int  wlen = CMD_REPLY_HEAD_LEN + 3 + node->masknum + (node->valuesize + node->masksize) * num;
    DINFO("range wlen: %d\n", wlen);
    if (conn) {
        //DINFO("conn is true.\n");
        wbuf = conn_write_buffer(conn, wlen);
    }
    idx += CMD_REPLY_HEAD_LEN;

    DataBlock *dbk = node->data_tail;
    DataBlock *newdbk = NULL;
    DataBlock *startdbk = dbk, *enddbk = NULL;
    char      *itemdata;
    int datalen = node->valuesize + node->masksize;
    int n     = 0; 
    int i;
    int rmn   = 0;  // delete value count in whole block

    if (wbuf) {
        memcpy(wbuf + idx, &node->valuesize, sizeof(char));
        idx += sizeof(char);
        memcpy(wbuf + idx, &node->masksize, sizeof(char));
        idx += sizeof(char);
        memcpy(wbuf + idx, &node->masknum, sizeof(char));
        idx += sizeof(char);
        memcpy(wbuf + idx, node->maskformat, node->masknum);
        idx += node->masknum;
    }else{
        idx += 3 + node->masknum;
    }

    while (dbk) {
        itemdata = dbk->data + ((dbk->data_count - 1) * datalen);
        int dbkcpnv = 0;
        //for (i = 0; i < dbk->data_count; i++) {
        for (i = dbk->data_count - 1; i >= 0; i--) {
            ret = dataitem_check_data(node, itemdata);
            if (ret == MEMLINK_VALUE_VISIBLE) {
                /*char buf[128];
				snprintf(buf, node->valuesize + 1, "%s", itemdata);
				DINFO("\tok, copy item ... i:%d, value:%s\n", i, buf);*/
			    	
                if (wbuf) {
				    memcpy(wbuf + idx, itemdata, datalen);
                }
				idx += datalen;
				n += 1;
                dbkcpnv++;
				if (n >= num) { // copy complete!
                    if (i > 0) {
                        newdbk = mempool_get(g_runtime->mpool, sizeof(DataBlock) + dbk->data_count * datalen);
                        newdbk->data_count    = dbk->data_count;
                        newdbk->visible_count = dbk->visible_count - dbkcpnv; 
                        node->data_tail = newdbk;
                        newdbk->prev = dbk->prev;
                        if (dbk->prev) {
                            dbk->prev->next = newdbk;
                        }
                        DINFO("copy todata count:%d\n", i);
                        memcpy(newdbk->data, dbk->data, i * datalen);
                    }else{ // current datablock is all poped
                        rmn += dbk->data_count;
                        node->data_tail = dbk->prev;
                        if (dbk->prev) {
                            dbk->prev->next = NULL;
                        }
                    }
                    enddbk = dbk->prev; 

                    if (dbk->prev == NULL) {
                        node->data = NULL;
                    }
                    node->all  -= rmn;
                    node->used -= n;
                    ret = MEMLINK_OK;
					goto hashtable_rpop_over;
				}
            }
            itemdata -= datalen;
        }
        rmn += dbk->data_count;
        dbk = dbk->prev;
    }
    enddbk     = NULL;
    node->all  = 0;
    node->used = 0;
    node->data = node->data_tail = NULL;
    ret = MEMLINK_OK;
    
hashtable_rpop_over:
	DINFO("count: %d\n", n);
    if (n > 0) {
        datablock_free_inverse(startdbk, enddbk, datalen);
    }

hashtable_rpop_end:
    if (conn) {
        conn_write_buffer_head(conn, ret, idx);
        //char bufw[4096] = {0};
        //DINFO("bufw:%s\n", formath(conn->wbuf, conn->wlen, bufw, 4096));
    }
    return ret;
}


int 
hashtable_print(HashTable *ht, char *key)
{
    HashNode    *node;

    node = hashtable_find(ht, key);
    if (NULL == node) {
        DINFO("not found key: %s\n", key);
        return MEMLINK_ERR_NOKEY;
    }

    DINFO("------ HashNode key:%s, valuesize:%d, masksize:%d, masknum:%d, all:%d, used:%d\n",
                node->key, node->valuesize, node->masksize, node->masknum, node->all, node->used);

    int i;
    for (i = 0; i < node->masknum; i++) {
        DINFO("maskformat: %d, %d\n", i, node->maskformat[i]);
    }

    DataBlock *dbk  = node->data;
    DataBlock *prev = NULL;
    int blocks = 0;
    
    zz_check(node);
    zz_check(node->key);

    while (dbk) {
		zz_check(dbk);

        blocks += 1;
        DINFO("====== dbk %p visible: %d, tagdel: %d, block: %d, count:%d ======\n", dbk, dbk->visible_count, dbk->tagdel_count, blocks, dbk->data_count);
        datablock_print(node, dbk);
        
        if (prev != dbk->prev) {
            DERROR("prev link error! blocks:%d, %p, prev:%p, dbk->prev:%p\n", blocks, dbk, prev, dbk->prev);
        }

        prev = dbk;
        dbk = dbk->next;
    }
    DINFO("------ blocks: %d\n", blocks);

    return MEMLINK_OK;
}

//add by lanwenhong
int 
hashtable_del_by_mask(HashTable *ht, char *key, unsigned int *maskarray, int masknum)
{

	int i, k;
	int count = 0;
	int datalen = 0;
	char maskval[HASHTABLE_MASK_MAX_ITEM * HASHTABLE_MASK_MAX_BYTE] = {0};
	char maskflag[HASHTABLE_MASK_MAX_ITEM * HASHTABLE_MASK_MAX_BYTE] = {0};
	HashNode *node;
	
	node = hashtable_find(ht, key);
	if (NULL == node) {
		DINFO("hashtable_del_by_mask not found node for key:%s\n", key);
		return MEMLINK_ERR_NOKEY;
	}
	
	if (node->masknum != masknum) {
		return MEMLINK_ERR_MASK;
	}
	datalen = node->valuesize + node->masksize;
	if (masknum > 0) {
		mask_array2_binary_flag(node->maskformat, maskarray, masknum, maskval, maskflag);
	}
	DataBlock *root = node->data;

    while (root) {
        char *itemdata = root->data;

        for (i = 0; i < root->data_count; i++) {
            if (dataitem_have_data(node, itemdata, 0)) {
                char *maskdata = itemdata + node->valuesize;

                for (k = 0; k < node->masksize; k++) {
                    if ((maskdata[k] & maskflag[k]) != maskval[k]) {
                        break;
                    }
                }
                //not equal
                if (k < node->masksize) {
                    itemdata += datalen;
                    continue;
                } else {//equal
					if ((maskdata[0] & 0x03) == 1) {
						root->visible_count--;
					}
					if ((maskdata[0] & 0x03) == 3) {
						root->tagdel_count--;
					}
                    maskdata[0] = maskdata[0] & 0x00;
                    count++;
                }
            }
            itemdata += datalen;
        }
        root = root->next;
    }

    return count;
}

