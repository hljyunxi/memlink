#include "hashtest.h"

int main()
{
#ifdef DEBUG
	logfile_create("test.log", 3);
#endif
	HashTable* ht;
	char key[64];
	int valuesize = 8;
	char val[64];
	int maskformat[3] = {4, 3, 1};
	char* maskstr1[] = {"8:3:1", "7:2:1", "6:2:1"};
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
	//test1 : hashtable_add_info_mask //Ìí¼Ókey
	for(i = 0; i < num; i++)
	{
		sprintf(key, "heihei%03d", i);
		hashtable_key_create_mask(ht, key, valuesize, maskformat, masknum);
	}
	for(i = 0; i < num; i++)
	{
		sprintf(key, "heihei%03d", i);
		HashNode* pNode = hashtable_find(ht, key);
		if(NULL == pNode)
		{
			printf("hashtable_add_info_mask error. can not find %s\n", key);
			return -1;
		}
	}

///////test2 : hashtable_add_mask ²åÈëvalue
	printf("1 insert 1000 \n");

	int pos = 0;
	HashNode *node = NULL;
	DataBlock *dbk = NULL;
	char	 *item = NULL; 
	num = 1000;
	for(i = 0; i < num; i++)
	{
		sprintf(val, "value%03d", i);
		//pos = i;
		pos = my_rand(num + 50);
		ret = hashtable_add_mask(ht, key, val, maskarray[i%3], 3, pos);
		if (ret < 0) {
			printf("hashtable_add_mask err! ret:%d, val:%s, pos:%d \n", ret, val, pos);
			return ret;
		}
	}

	printf("2 insert 1000 \n");
	//return 0;
	
	for(i = 0; i < num; i++)
	{
		sprintf(val, "value%03d", i);
		ret = hashtable_find_value(ht, key, val, &node, &dbk, &item);
		if (ret < 0) {
			printf("not found value: %d, %s\n", ret, key);
			return ret;
		}
	}
	
	sprintf(val, "value%03d", 50);
	ret = hashtable_del(ht, key, val);
	if (ret < 0) {
		printf("del value: %d, %s\n", ret, val);
		return ret;
	}
	
	ret = hashtable_add_mask(ht, key, val, maskarray[2], masknum, 100);
	if (ret < 0) {
		printf("add value err: %d, %s\n", ret, val);
		return ret;
	}

	ret = hashtable_find_value(ht, key, val, &node, &dbk, &item);
	if (ret < 0) {
		printf("not found value: %d, %s\n", ret, val);
		return ret;
	}
	printf("test: insert ok!\n");

/////////// hashtable_del
	for(i = 0; i < num; i++)
	{
		sprintf(val, "value%03d", i);
		//pos = i;
		ret = hashtable_del(ht, key, val);
		if (ret < 0) {
			printf("hashtable_del err! ret:%d, val:%s \n", ret, val);
			return ret;
		}
	}
	printf("del 1000!\n");

	for(i = 0; i < num; i++)
	{
		sprintf(val, "value%03d", i);
		ret = hashtable_find_value(ht, key, val, &node, &dbk, &item);
		if (ret >= 0) {
			printf("err should not found value: %d, %s\n", ret, key);
			return ret;
		}
	}
	
	printf("test: del ok!\n");
	return 0;
}
