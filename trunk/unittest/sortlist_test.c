#include "hashtest.h"
#include "datablock.h"
#include "utils.h"
#include <stdio.h>
#include <assert.h>

int check_values(int *values, int values_count, HashNode *node)
{
    DINFO("<<<<<< check values, used:%d, value count:%d >>>>>>\n", node->used, values_count);
    assert(node->used == values_count); 
    DataBlock *dbk = node->data;

    int i, vi = 0;
    int datalen = node->valuesize + node->masksize;
    while (dbk) {
        char *itemdata = dbk->data;
        for (i = 0; i < dbk->data_count; i++) {
            if (dataitem_have_data(node, itemdata, MEMLINK_VALUE_VISIBLE)){
                DINFO("value cmp: %d, %d\n", values[vi], *(int*)itemdata);
                assert(values[vi] == *(int*)itemdata);
                vi++;
            }
            itemdata += datalen; 
        }
        dbk = dbk->next;
    }
    return 0;
}

int add_one_value(HashTable *ht, HashNode *node, char *key, int value, char *mask, 
                    int *values, int *values_count)
{
    DINFO("add one: %d\n", value);
    //hashtable_print(ht, key);
    //check_values(values, *values_count, node);
    assert(node->used == *values_count);
   
    //int value = 5;
    int ret = hashtable_sortlist_add_mask_bin(ht, key, &value, mask);
    if (ret != MEMLINK_OK) {
        DERROR("add error:%d, key:%s, value:%d\n", ret, key, value);
    }
    values[*values_count] = value;
    (*values_count)++;
    qsort(values, *values_count, sizeof(int), compare_int);
    /*int i;
    for (i = 0; i < *values_count; i++) {
        DINFO("i:%d, value:%d\n", i, values[i]);
    }*/
    DINFO("hashtable print %s ...\n", key);
    hashtable_print(ht, key);
    DINFO("check values ...\n");
    check_values(values, *values_count, node);

    return 0;
}

int main()
{
#ifdef DEBUG
	logfile_create("stdout", 3);
#endif
    HashTable *ht;

    myconfig_create("memlink.conf");
	my_runtime_create_common("memlink");
	ht = g_runtime->ht;

    char key[128];
    int  valuesize = 4;
    int  masknum = 0; 
    int  i = 0, ret;
    unsigned int maskformat[4] = {0};

    sprintf(key, "haha%03d", i);
    hashtable_key_create_mask(ht, key, valuesize, maskformat, masknum, 
                              MEMLINK_SORTLIST, MEMLINK_VALUE_UINT4);
   
    char mask[10] = {0x01};
    int value;
    int values[1024] = {0};
    int values_count = 0;
    HashNode *node = hashtable_find(ht, key);

    for (i = 10; i < 20; i++) {
        value = i * 2;
        ret = hashtable_sortlist_add_mask_bin(ht, key, &value, mask);
        if (ret != MEMLINK_OK) {
            DERROR("add error:%d, key:%s, value:%d\n", ret, key, i);
        }
        values[values_count] = value;
        values_count++;
        assert(node->used == values_count);

        hashtable_print(ht, key);
    }
    qsort(values, values_count, sizeof(int), compare_int);
    /*for (i = 0; i < values_count; i++) {
        DINFO("i:%d, value:%d\n", i, values[i]);
    }*/
    //hashtable_print(ht, key);
    check_values(values, values_count, node);
    assert(node->used == 10);
   
    value = 100;
    DINFO("add value:%d\n", value);
    add_one_value(ht, node, key, value, mask, values, &values_count);

    value = 1;
    DINFO("add value:%d\n", value);
    add_one_value(ht, node, key, value, mask, values, &values_count);

    return 0;
}
