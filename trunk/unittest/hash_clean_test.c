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
	//char charmaskformat[3] = {4, 3, 1};
	//char* maskstr1[] = {"8:3:1", "7:2:1", "6:2:1"};
	int maskarray[6][3] = { { 7, UINT_MAX, 1}, {6, 2, 1}, { 4, 1, UINT_MAX}, 
		                    {8, 3, 1}, {8, 8, 8}, { UINT_MAX, UINT_MAX, UINT_MAX} }; 
	int num  = 100*10 - 1;
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
	for(i = 0; i < 1; i++)
	{
		sprintf(key, "heihei%03d", i);
		hashtable_key_create_mask(ht, key, valuesize, maskformat, masknum);
	}
	for(i = 0; i < 1; i++)
	{
		sprintf(key, "heihei%03d", i);
		HashNode* pNode = hashtable_find(ht, key);
		if(NULL == pNode)
		{
			printf("hashtable_add_info_mask error. can not find %s\n", key);
			return -1;
		}
	}

	///////test : hashtable_add_mask ²åÈënum¸övalue
	HashNode *node = NULL;
	DataBlock *dbk = NULL;
	char	 *item = NULL; 	
	int pos = 0;
	for(i = 0; i < num; i++)
	{
		sprintf(val, "value%03d", i);
		pos = i;
		ret = hashtable_add_mask(ht, key, val, maskarray[i%4], masknum, pos);
		if (ret < 0) {
			printf("add value err: %d, %s\n", ret, val);
			return ret;
		}
		
		ret = hashtable_find_value(ht, key, val, &node, &dbk, &item);
		if (ret < 0) {
			printf("not found value: %d, %s\n", ret, key);
			return ret;
		}
	}
	printf("insert %d values ok!\n", num);

	//////////test 
	HashTableStat stat;
	hashtable_stat(ht, key, &stat);
	if(stat.blocks != 2 && stat.data_used != num && stat.data != 200)
	{
		printf("err stat.blocks:%d, stat.data_used:%d, key:%s\n", stat.blocks, stat.data_used, key);
		return -1;
	}

	///del 500
	for(i = 0; i < 500; i++)
	{
		sprintf(val, "value%03d", i*2);
		hashtable_del(ht, key, val);
	}
	
	hashtable_clean(ht, key);

	HashTableStat stat1;
	hashtable_stat(ht, key, &stat1);
	if(stat1.blocks != 5 || stat1.data_used != 499 || stat1.data != 500)
	{
		printf("err stat.blocks:%d, stat.data_used:%d, stat.data:%d, key:%s\n",
			    stat1.blocks, stat1.data_used, stat1.data, key);
		return -1;
	}

	///del all the rest
	for(i = 0; i < 499; i++)
	{
		sprintf(val, "value%03d", i*2+1);
		hashtable_del(ht, key, val);
	}
	
	hashtable_clean(ht, key);

	//HashTableStat stat1;
	hashtable_stat(ht, key, &stat1);
	if(stat1.blocks != 0 || stat1.data_used != 0 || stat1.data != 0)
	{
		printf("err stat.blocks:%d, stat.data_used:%d, stat.data:%d, key:%s\n",
			    stat1.blocks, stat1.data_used, stat1.data, key);
		return -1;
	}
	
	printf("hashtable clean test end!\n");
	return 0;
}




