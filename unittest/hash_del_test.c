#include "hashtest.h"

int
find_value_in_block(HashNode *node, DataBlock *dbk, void *value)
{
    int pos = 0;
    int datalen = node->valuesize + node->masksize;
    char *data = dbk->data;
    int i;

    for (i = 0; i < dbk->data_count; i++) {
        if (dataitem_have_data(node, data, 0) && memcmp(value, data, node->valuesize) == 0) {
            if (pos == 0)
                return 1;
            return pos;
        }
        pos++;
        data += datalen;
    }

    return -1;
}

int
build_data_model(HashTable *ht, char *key, int valuesize, unsigned int *maskformat, int masknum, int num)
{
    int ret;

    ret = hashtable_key_create_mask(ht, key, valuesize, maskformat, masknum, MEMLINK_LIST, MEMLINK_VALUE_STRING);
    if (ret != MEMLINK_OK) {
        DERROR("key create error, ret: %d, key: %s\n", ret, key);
        return ret;
    }

    int pos = -1;
    unsigned int maskarray[3] = {4, 4, 4};
    int i;
    char val[64];

    for (i = 0; i < num; i++) {
        snprintf(val, 64, "value%03d", i);
        ret = hashtable_add_mask(ht, key, val, maskarray, 3, pos);
        if (ret != MEMLINK_OK) {
            DERROR("add value error: %d, %s\n", ret, val);
            return ret;
        }
    }
    DINFO("insert %d value ok!\n", num);

    return 0;
}

int
del_test1(HashTable *ht, char *key, int valuesize, unsigned int *maskformat, int masknum)
{
    int ret;
    HashNode *node;
    char val[64];

    ret = build_data_model(ht, key, valuesize, maskformat, masknum, 10);
    if (ret < 0)
        return ret;

    node = hashtable_find(ht, key);
    int blockmax = g_cf->block_data_count[g_cf->block_data_count_items - 1];
    DataBlock *dbk = node->data;
    
    if (dbk->data_count != blockmax || dbk->visible_count + dbk->tagdel_count != dbk->data_count) {
        DERROR("the first block is not full\n");
        return -4;
    }

    int i;
    for (i = 0; i < 4; i++) {
        snprintf(val, 64, "value%03d",  i);
        ret = hashtable_del(ht, key, val);
        if (ret != MEMLINK_OK) {
            DERROR("hashtable_del, ret: %d, key: %s, val: %s\n", ret, key, val);
            return ret;
        }
    }
    hashtable_print(ht, key);    
    if (dbk->data_count != 6) {
        return -5;
    }
    DataBlock *newdbk = node->data;

    if (newdbk != dbk) {
        return -6;
    }
    hashtable_clear_all(ht);
    return 0;
}

int
del_test2(HashTable *ht, char *key, int valuesize, unsigned int *maskformat, int masknum)
{
    int ret;
    HashNode *node;
    char val[64];

    ret = build_data_model(ht, key, valuesize, maskformat, masknum, 10);
    if (ret < 0)
        return ret;

    node = hashtable_find(ht, key);
    int blockmax = g_cf->block_data_count[g_cf->block_data_count_items - 1];
    DataBlock *dbk = node->data;
    
    if (dbk->data_count != blockmax || dbk->visible_count + dbk->tagdel_count != dbk->data_count) {
        DERROR("the first block is not full\n");
        return -4;
    }

    int i;
    for (i = 0; i < 5; i++) {
        snprintf(val, 64, "value%03d",  i);
        ret = hashtable_del(ht, key, val);
        if (ret != MEMLINK_OK) {
            DERROR("hashtable_del, ret: %d, key: %s, val: %s\n", ret, key, val);
            return ret;
        }
    }
    hashtable_print(ht, key);    
    if (dbk->data_count != 5) {
        return -5;
    }
    DataBlock *newdbk = node->data;

    if (newdbk != dbk) {
        return -6;
    }
    hashtable_clear_all(ht);
    return 0;

}

int
del_test3(HashTable *ht, char *key, int valuesize, unsigned int *maskformat, int masknum)
{
    int ret;
    HashNode *node;
    char val[64];

    ret = build_data_model(ht, key, valuesize, maskformat, masknum, 10);
    if (ret < 0)
        return ret;

    node = hashtable_find(ht, key);
    int blockmax = g_cf->block_data_count[g_cf->block_data_count_items - 1];
    DataBlock *dbk = node->data;
    
    if (dbk->data_count != blockmax || dbk->visible_count + dbk->tagdel_count != dbk->data_count) {
        DERROR("the first block is not full\n");
        return -4;
    }

    int i;
    for (i = 0; i < 6; i++) {
        snprintf(val, 64, "value%03d",  i);
        ret = hashtable_del(ht, key, val);
        if (ret != MEMLINK_OK) {
            DERROR("hashtable_del, ret: %d, key: %s, val: %s\n", ret, key, val);
            return ret;
        }
    }
    hashtable_print(ht, key);
    //删除了6个， 还剩4个， 用5的块
    DataBlock *newdbk = node->data;
    DNOTE("newdbk->data_count: %d\n", newdbk->data_count);
    if (newdbk->data_count != 5) {
        return -5;
    }
    if (newdbk == dbk) {
        return -6;
    }
    if (newdbk->visible_count + newdbk->tagdel_count != 4) {
        return -7;
    }
    hashtable_clear_all(ht);
    return 0;


}

int
del_test4(HashTable *ht, char *key, int valuesize, unsigned int *maskformat, int masknum)
{
    int ret;
    HashNode *node;
    char val[64];

    ret = build_data_model(ht, key, valuesize, maskformat, masknum, 10);
    if (ret < 0)
        return ret;

    node = hashtable_find(ht, key);
    int blockmax = g_cf->block_data_count[g_cf->block_data_count_items - 1];
    DataBlock *dbk = node->data;
    
    if (dbk->data_count != blockmax || dbk->visible_count + dbk->tagdel_count != dbk->data_count) {
        DERROR("the first block is not full\n");
        return -4;
    }

    int i;
    for (i = 0; i < 7; i++) {
        snprintf(val, 64, "value%03d",  i);
        ret = hashtable_del(ht, key, val);
        if (ret != MEMLINK_OK) {
            DERROR("hashtable_del, ret: %d, key: %s, val: %s\n", ret, key, val);
            return ret;
        }
    }
    hashtable_print(ht, key);
    //删除了7个value, 还剩下3个， 用5的块     
    DataBlock *newdbk = node->data;
    if (newdbk->data_count != 5) {
        return -5;
    }

    if (newdbk != dbk) {
        return -6;
    }
    if (newdbk->visible_count + newdbk->tagdel_count != 3) {
        return -7;
    }
    hashtable_clear_all(ht);

    return 0;
}

int
del_test5(HashTable *ht, char *key, int valuesize, unsigned int *maskformat, int masknum)
{
    int ret;
    HashNode *node;
    char val[64];

    ret = build_data_model(ht, key, valuesize, maskformat, masknum, 10);
    if (ret < 0)
        return ret;

    node = hashtable_find(ht, key);
    int blockmax = g_cf->block_data_count[g_cf->block_data_count_items - 1];
    DataBlock *dbk = node->data;
    
    if (dbk->data_count != blockmax || dbk->visible_count + dbk->tagdel_count != dbk->data_count) {
        DERROR("the first block is not full\n");
        return -4;
    }

    int i;
    for (i = 0; i < 8; i++) {
        snprintf(val, 64, "value%03d",  i);
        ret = hashtable_del(ht, key, val);
        if (ret != MEMLINK_OK) {
            DERROR("hashtable_del, ret: %d, key: %s, val: %s\n", ret, key, val);
            return ret;
        }
    }
    //删除了8个value, 还剩下2个， 用2的块     
    hashtable_print(ht, key);
    DataBlock *newdbk = node->data;
    if (newdbk->data_count != 2) {
        return -5;
    }

    if (newdbk == dbk) {
        return -6;
    }
    if (newdbk->visible_count + newdbk->tagdel_count != 2) {
        return -7;
    }
    hashtable_clear_all(ht);

    return 0;
}

int
del_test6(HashTable *ht, char *key, int valuesize, unsigned int *maskformat, int masknum)
{
    int ret;
    HashNode *node;
    char val[64];

    ret = build_data_model(ht, key, valuesize, maskformat, masknum, 10);
    if (ret < 0)
        return ret;

    node = hashtable_find(ht, key);
    int blockmax = g_cf->block_data_count[g_cf->block_data_count_items - 1];
    DataBlock *dbk = node->data;
    
    if (dbk->data_count != blockmax || dbk->visible_count + dbk->tagdel_count != dbk->data_count) {
        DERROR("the first block is not full\n");
        return -4;
    }

    int i;
    for (i = 0; i < 9; i++) {
        snprintf(val, 64, "value%03d",  i);
        ret = hashtable_del(ht, key, val);
        if (ret != MEMLINK_OK) {
            DERROR("hashtable_del, ret: %d, key: %s, val: %s\n", ret, key, val);
            return ret;
        }
    }
    hashtable_print(ht, key);
    //删除了9个value, 还剩下1个， 用1的块     
    DataBlock *newdbk = node->data;
    DNOTE("newdbk->data_count: %d\n", newdbk->data_count);
    if (newdbk->data_count != 1) {
        return -5;
    }

    if (newdbk == dbk) {
        return -6;
    }
    if (newdbk->visible_count + newdbk->tagdel_count != 1) {
        return -7;
    }
    hashtable_clear_all(ht);

    return 0;
}

int
del_test7(HashTable *ht, char *key, int valuesize, unsigned int *maskformat, int masknum)
{
    int ret;
    HashNode *node;
    char val[64];

    ret = build_data_model(ht, key, valuesize, maskformat, masknum, 10);
    if (ret < 0)
        return ret;

    node = hashtable_find(ht, key);
    int blockmax = g_cf->block_data_count[g_cf->block_data_count_items - 1];
    DataBlock *dbk = node->data;
    
    if (dbk->data_count != blockmax || dbk->visible_count + dbk->tagdel_count != dbk->data_count) {
        DERROR("the first block is not full\n");
        return -4;
    }

    int i;
    for (i = 0; i < 10; i++) {
        snprintf(val, 64, "value%03d",  i);
        ret = hashtable_del(ht, key, val);
        if (ret != MEMLINK_OK) {
            DERROR("hashtable_del, ret: %d, key: %s, val: %s\n", ret, key, val);
            return ret;
        }
    }
    //删除了7个value, 还剩下3个， 用5的块     
    DataBlock *newdbk = node->data;
    if (newdbk != NULL) {
        return -5;
    }

    return 0;
}

int
del_test8(HashTable *ht, char *key, int valuesize, unsigned int *maskformat, int masknum)
{
    int ret;
    HashNode *node;
    char val[64];
    int pos;

    ret = build_data_model(ht, key, valuesize, maskformat, masknum, 30);
    if (ret < 0)
        return ret;
    
    node = hashtable_find(ht, key);

    DataBlock *first = node->data;
    if (first == NULL)
        return -4;
    
    DataBlock *second = node->data;
    if (second == NULL)
        return -5;

    DataBlock *third = node->data;
    if (third == NULL)
        return -6;

   
    int i;
    //第一块删除7个数据,还剩下3个
    for (i = 3; i < 10; i++) {
        snprintf(val, 64, "value%03d", i);
        ret = hashtable_del(ht, key, val);
        if (ret != MEMLINK_OK) {
            DERROR("hashtable_del, ret: %d, key: %s, val: %s\n", ret, key, val);
            return ret;
        }
    }

    DNOTE("first: %d\n", first->visible_count + first->tagdel_count);

    //第三快删除7个数据，还剩下3个
    for (i = 23; i < 30; i++) {
        snprintf(val, 64, "value%03d", i);
        ret = hashtable_del(ht, key, val);
        if (ret != MEMLINK_OK) {
            DERROR("hashtable_del, ret: %d, key: %s, val: %s\n", ret, key, val);
            return ret;
        }
    }
    DNOTE("third: %d\n", third->visible_count + third->tagdel_count);
    
    //第二个快删除7个数据，还剩下3个
    for (i = 13; i < 20; i++) {
        snprintf(val, 64, "value%03d", i);
        ret = hashtable_del(ht, key, val);
        if (ret != MEMLINK_OK) {
            DERROR("hashtable_del, ret: %d, key: %s, val: %s\n", ret, key, val);
            return ret;
        }
    }
    
    DataBlock *newdbk = node->data;
    
    hashtable_print(ht, key);
    if (newdbk->next != NULL)
        return -7;
    DNOTE("newdbk->data_count: %d\n", newdbk->data_count);
    DNOTE("newdbk->visible_count + newdbk->tagdel_count: %d\n", newdbk->visible_count + newdbk->tagdel_count);
    if (newdbk->data_count != 10 || (newdbk->visible_count + newdbk->tagdel_count != 9)) {
        return -8;
    }

    for (i = 0 ; i < 3; i++) {
        snprintf(val, 64, "value%03d", i);
        pos = find_value_in_block(node, newdbk, val);
        if (pos < 0) {
            return -9;
        }
    }

    for (i = 10; i < 13; i++) {
        snprintf(val, 64, "value%03d", i);
        pos = find_value_in_block(node, newdbk, val);
        if (pos < 0)
            return -10;
    }

    for (i = 20; i < 23; i++) {
        snprintf(val, 64, "value%03d", i);
        pos = find_value_in_block(node, newdbk, val);
        if (pos < 0)
            return -10;
    }

    return 0;
}

int main(int argc, char **argv)
{
#ifdef DEBUG
    logfile_create("test.log", 4);
#endif
    HashTable *ht;
    char *conffile = "memlink.conf";

    myconfig_create(conffile);
    my_runtime_create_common("memlink");
    ht = g_runtime->ht;

    char key[64];
    int valuesize = 8;
    unsigned int maskarray[3] = {4, 4, 4};

    snprintf(key, 64, "%s", "test");
    
    int ret;

    ret = del_test8(ht, key, valuesize, maskarray, 3);
    DNOTE("del_test8: %d\n", ret);
    if (ret < 0)
        return ret;

    return 0;

}
