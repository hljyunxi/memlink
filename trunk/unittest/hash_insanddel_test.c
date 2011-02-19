#include "hashtest.h"

int main()
{
#ifdef DEBUG
	logfile_create("stdout", 3);
#endif
	HashTable* ht;
	char key[64];
	int valuesize = 8;
	char val[64];
	int maskformat[3] = {4, 3, 1};
	//char* maskstr1[] = {"8:3:1", "7:2:1", "6:2:1"};
	int maskarray[3][3] = { {8, 3, 1}, { 7, 2,1}, {6, 2, 1} }; 
	int num  = 200;
	int masknum = 3;
	int ret;
	int i = 0;

	char *conffile;
	conffile = "memlink.conf";
	DINFO("config file: %s\n", conffile);
	myconfig_create(conffile);

	my_runtime_create_common("memlink");

	ht = g_runtime->ht;


	///////////begin test;
	//test1 : hashtable_add_info_mask - create key
	for(i = 0; i < num; i++)
	{
		sprintf(key, "heihei%03d", i);
		hashtable_key_create_mask(ht, key, valuesize, maskformat, masknum, MEMLINK_LIST, 0);
	}
	for(i = 0; i < num; i++)
	{
		sprintf(key, "heihei%03d", i);
		HashNode* pNode = hashtable_find(ht, key);
		if(NULL == pNode)
		{
			DERROR("hashtable_add_info_mask error. can not find %s\n", key);
			return -1;
		}
	}

///////test : hashtable_add_mask insert num value
	DINFO("1 insert 1000 \n");

	int pos = 0;
	HashNode *node = NULL;
	DataBlock *dbk = NULL;
	char	 *item = NULL; 
	num = 200;

	for(i = 0; i < num; i++)
	{
		sprintf(val, "value%03d", i);
		pos = i;
		//pos = my_rand(num + 50);

		//DINFO("<<<<<<<hashtable_print>>>>>>>>>>>>\n");
		//hashtable_print(g_runtime->ht, key);
		//DINFO("<<<<<<hashtable_add_mask>>>> val:%s, pos:%d \n", val, pos);
		ret = hashtable_add_mask(ht, key, val, maskarray[i%3], 3, pos);
		if (ret < 0) {
			DERROR("hashtable_add_mask err! ret:%d, val:%s, pos:%d \n", ret, val, pos);
			return ret;
		}
	}

	DINFO("2 insert %d\n", num);
    
    MemLinkStat stat;
    ret = hashtable_stat(ht, key, &stat);
    if (ret != MEMLINK_OK) {
        DERROR("stat error!\n");
    }

    DINFO("blocks:%d, data_used:%d\n", stat.blocks, stat.data_used);

	//return 0;
	//DINFO("<<<<<<<hashtable_end>>>>>>>>>>>>\n");
	//hashtable_print(g_runtime->ht, key);

	for(i = 0; i < num; i++)
	{
		sprintf(val, "value%03d", i);
		ret = hashtable_find_value(ht, key, val, &node, &dbk, &item);
		if (ret < 0) {
			DERROR("not found value: %d, %s\n", ret, val);
			return ret;
		}
	}
	
	sprintf(val, "value%03d", 50);
	ret = hashtable_del(ht, key, val);
	if (ret < 0) {
		DERROR("del value: %d, %s\n", ret, val);
		return ret;
	}
    DINFO("deleted %s\n", val);	
	//hashtable_print(g_runtime->ht, key);

	ret = hashtable_add_mask(ht, key, val, maskarray[2], masknum, 100);
	if (ret < 0) {
		DERROR("add value err: %d, %s\n", ret, val);
		return ret;
	}

	//hashtable_print(g_runtime->ht, key);

	ret = hashtable_find_value(ht, key, val, &node, &dbk, &item);
	if (ret < 0) {
		DERROR("not found value: %d, %s\n", ret, val);
		return ret;
	}
	DINFO("test: insert ok!\n");

    /////////// hashtable_del
	for(i = 0; i < num; i++)
	{
		sprintf(val, "value%03d", i);
		//pos = i;
		ret = hashtable_del(ht, key, val);
		if (ret < 0) {
			DERROR("hashtable_del err! ret:%d, val:%s \n", ret, val);
			return ret;
		}
	}
	DINFO("del %d!\n", num);

	for(i = 0; i < num; i++)
	{
		sprintf(val, "value%03d", i);
		ret = hashtable_find_value(ht, key, val, &node, &dbk, &item);
		if (ret >= 0) {
			DERROR("err should not found value: %d, %s\n", ret, key);
			return ret;
		}
	}
	
	DINFO("test: del ok!\n");
	return 0;
}
