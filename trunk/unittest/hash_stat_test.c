#include "hashtest.h"

int check2(HashTableStat *stat, int vs, int ms, int blocks, int data, int datau, int mem, int memu)
{
	if (stat->valuesize != vs) {
		printf("valuesize error: %d\n", stat->valuesize);
	}

	if (stat->masksize != ms) {
		printf("masksize error: %d\n", stat->masksize);
	}

	if (stat->blocks != blocks) {
		printf("blocks error: %d\n", stat->blocks);
	}
	
	if (stat->data != data)  {
		printf("data error: %d\n", stat->data);
	}

	if (stat->data_used != datau) {
		printf("data_used error: %d\n", stat->data_used);
	}
	
	if (stat->mem != mem) {
		printf("mem  error: %d   %d\n", stat->mem, mem);
	}

	//if (stat->mem_used != memu) {
		//printf("mem_used error: %d      %d\n", stat->mem_used, memu);
	//}
	return 0;
}

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
	int num  = 199;
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
			printf("hashtable_add_info_mask error. can not find %s\n", key);
			return -1;
		}
	}

	///////test : hashtable_add_mask insert num value
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

	//////////test 8:stat
	HashTableStat stat;
	hashtable_stat(ht, key, &stat);
	int ms  = sizeof(HashNode) + (10 * 100 + sizeof(DataBlock)) * 2;
	int mus = 0;
	check2(&stat, valuesize, 2, 2, 200, 199, ms, mus);
	
	printf("hashtable stat test end!\n");
	return 0;
}



