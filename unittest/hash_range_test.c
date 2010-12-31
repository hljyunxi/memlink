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
	int maskarray[4][3] = { { 7, 2, 1}, {6, 2, 1}, { 4, 1, UINT_MAX}, {8, 3, 1} }; 
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

	///////test : hashtable_add_mask ²åÈënum¸övalue
	HashNode *node = NULL;
	DataBlock *dbk = NULL;
	char	 *item = NULL; 	
	int pos = 0;
	for(i = 0; i < num; i++)
	{
		sprintf(val, "value%03d", i);
		pos = i;
		ret = hashtable_add_mask(ht, key, val, maskarray[3], masknum, pos); //{8, 3, 1}
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

	for(i = 0; i < 100; i++)
	{
		sprintf(val, "value%03d", i*2);
		ret = hashtable_mask(ht, key, val, maskarray[2], masknum); //{ 4, 1, UINT_MAX}
		if(MEMLINK_OK != ret)
		{
			printf("err hashtable_mask val:%s, i:%d\n", val, i);
			return ret;
		}
	}
	
	//////////test 5 :range
	int maskarray2[6][3] = { 
							 { 8, 3, 1}, {8, UINT_MAX, 1}, 
							 { UINT_MAX, 3, 1}, {4, 1, 1} , 
							 {UINT_MAX, UINT_MAX, 1},  {UINT_MAX, UINT_MAX, UINT_MAX}
						   }; 
	for(i = 0; i < 50; i++)
	{
		int k; 
		if(i < 25)
		{
			k  = 1;  //{8, UINT_MAX, 1} masknum:3		
			num = 100;
		}
		else
		{	
			k = 5;  //{UINT_MAX, UINT_MAX, UINT_MAX} masknum:0	
			num = 200;
		}
		int frompos = my_rand(num);
		int len = my_rand(num);
		char vsize;
		char msize;
		char data[4096*2];
		int retnum = 0;
		int realnum;
		if(frompos < num)
		{
			int tmp = num - frompos;
			(len < tmp)?(realnum = len):(realnum = tmp);
		}
		else
			realnum = 0;
		printf("frompos:%d, len:%d, i:%d\n", frompos, len, i);
		
		ret = hashtable_range(ht, key, MEMLINK_VALUE_VISIBLE, maskarray2[k], masknum, 
			            frompos, len, 
			            data, &retnum,
			            &vsize, &msize);
		
		if( 0 != ret)
		{
			printf("ret:%d, len:%d\n", ret, len);
		}
		if(retnum != 0)
			retnum = (retnum - sizeof(char) - masknum)/(vsize + msize);
		if(retnum != realnum)
		{			
			printf("error!! realnum:%d, retnum:%d, i:%d\n", realnum, retnum, i);
			//return -1;
		}
		//else
		{
			int j;
			char* item = data + sizeof(char) + masknum;
			for(j = frompos; j < frompos + realnum; j++)
			{
				int jj;
				if(i < 25)
					jj = j*2 + 1;
				else
					jj = j;
				sprintf(val, "value%03d", jj);
				int ret = memcmp(item, val, valuesize);
				if(ret != 0)
				{
					printf("pos:%d, val:%s\n", jj, val);
					return -1;
				}
				item = item + vsize + msize;
			}
		}
	}


	printf("hashtable range test end!\n");
	return 0;
}

