#include "hashtest.h"

int main()
{
#ifdef DEBUG
	logfile_create("test.log", 3);
#endif
	HashTable* ht;
	char key[64];
	int  valuesize = 8;
	char val[64];
	unsigned int attrformat[3] = {4, 3, 1};
	unsigned int attrarray[3][3] = { {8, 3, 1}, { 7, 2,1}, {6, 2, 1} }; 
	int num  = 100;
	int attrnum = 3;
	int ret;
	int i = 0;
	char *conffile;

	conffile = "memlink.conf";
	DINFO("config file: %s\n", conffile);
	myconfig_create(conffile);

	my_runtime_create_common("memlink");
	ht = g_runtime->ht;

	///////////begin test;
	//test1 : hashtable_add_info_attr - create key
	for (i = 0; i < num; i++) {
		sprintf(key, "heihei%03d", i);
		hashtable_key_create_attr(ht, key, valuesize, attrformat, attrnum, MEMLINK_LIST, 0);
	}
	for (i = 0; i < num; i++) {
		sprintf(key, "heihei%03d", i);
		HashNode* pNode = hashtable_find(ht, key);
		if (NULL == pNode) {
			DERROR("hashtable_add_info_attr error. can not find %s\n", key);
			return -1;
		}
	}

	///////test : hashtable_add_attr insert num value
	HashNode    *node = NULL;
	DataBlock   *dbk  = NULL;
	char	    *item = NULL; 	
	int         pos   = 0;

	for (i = 0; i < num; i++) {
		sprintf(val, "value%03d", i);
		pos = i;
		ret = hashtable_add_attr(ht, key, val, attrarray[i%3], attrnum, pos);
		if (ret < 0) {
			DERROR("add value err: %d, key:%s, value:%s, pos:%d\n", ret, key, val, pos);
			return ret;
		}
		
		ret = hashtable_find_value(ht, key, val, &node, &dbk, &item);
		if (ret < 0) {
			DERROR("not found value: %d, %s\n", ret, key);
			return ret;
		}
	}
	DINFO("insert %d values ok!\n", num);

	//////////test 4 :tag
	for (i = 0; i < num; i++) {
		HashNode *node = NULL;
		DataBlock *dbk = NULL;
		char	 *item = NULL; 
		int v = my_rand(num);		
		sprintf(val, "value%03d", v);
		
		int tag = my_rand(2);
		
		//printf("val:%s, tag:%d \n", val, tag);
		//hashtable_find_value(ht, key, val, &node, &dbk, &item);
		int ret = hashtable_tag(ht, key, val, tag);
		if (MEMLINK_OK != ret) {
			DERROR("err hashtable_tag val:%s, tag:%d\n", val, tag);
			return ret;
		}
		
		ret = hashtable_find_value(ht, key, val, &node, &dbk, &item);
		if (ret < 0) {
			DERROR("not found value: %d, %s\n", ret, key);
			return ret;
		}
		char attr[HASHTABLE_MASK_MAX_ITEM * HASHTABLE_MASK_MAX_BYTE] = {0};
		char *mdata = item + node->valuesize;
		memcpy(attr, mdata, node->attrsize); 

		char flag = *(attr + 0) & 0x02;
		flag = flag >> 1;
		//printf("flag:%d, tag:%d \n", flag, tag);
		if (flag != tag) {
			DERROR("tag err val:%s, flag:%d, tag:%d!\n", val, flag, tag);			
			break;
		}
	}
	DINFO("hashtable_tag test end!\n");

	return 0;
}

