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
	char charmaskformat[3] = {4, 3, 1};
	char* maskstr1[] = {"8:3:1", "7:2:1", "6:2:1"};
	int maskarray[4][3] = { { 7, UINT_MAX, 1}, {6, 2, 1}, { 4, 1, UINT_MAX}, {8, 3, 1} }; 
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
		hashtable_add_info_mask(ht, key, valuesize, maskformat, masknum);
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

	//////////test 5 :mask
	for(i = 0; i < num; i++)
	{
		HashNode *node = NULL;
		DataBlock *dbk = NULL;
		char	 *item = NULL; 
		//printf("====================\n");
		int v = my_rand(num);		
		sprintf(val, "value%03d", v);
		
		int k = my_rand(4);
		
		//hashtable_find_value(ht, key, val, &node, &dbk, &item);
		ret = hashtable_mask(ht, key, val, maskarray[k], masknum);
		if(MEMLINK_OK != ret)
		{
			printf("err hashtable_mask val:%s, k:%d\n", val, k);
			return ret;
		}
		
		ret = hashtable_find_value(ht, key, val, &node, &dbk, &item);
		if (ret < 0) {
			printf("not found value: %d, %s\n", ret, val);
			return ret;
		}
		char data[HASHTABLE_MASK_MAX_LEN * sizeof(int)] = {0};
		char flag[HASHTABLE_MASK_MAX_LEN * sizeof(int)] = {0};
		ret = mask_array2binary(charmaskformat, maskarray[k], masknum, data); //to binary
		mask_array2flag(charmaskformat, maskarray[k], masknum, flag);   //to flag
		char mask[HASHTABLE_MASK_MAX_LEN * sizeof(int)] = {0};
		char *mdata = item + node->valuesize;
		memcpy(mask, mdata, node->masksize); 
		int j = 0;
		mask[0] &= 0xfc;
		data[0] &= 0xfc;
		for(j = 0; j < ret; j++)
		{
		 	flag[j] = ~flag[j]; 
			if( ( mask[j] & flag[j] ) != data[j] )
				break;
		}
		if(j != ret)
		{
			printf("mask error. %d\n", i);	
			break;
		}

	}
	printf("hashtable mask test end!\n");
	return 0;
}

