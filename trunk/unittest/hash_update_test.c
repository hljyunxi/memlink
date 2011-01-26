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
	char* maskstr1[] = {"8:3:1", "7:2:1", "6:2:1"};
	int maskarray[3][3] = { {8, 3, 1}, { 7, 2,1}, {6, 2, 1} }; 
	int num  = 100;
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
		hashtable_key_create_mask(ht, key, valuesize, maskformat, masknum);
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


	///////test : hashtable_add_mask insert 200 value
	HashNode *node = NULL;
	DataBlock *dbk = NULL;
	char	 *item = NULL; 	
	int pos = 0;
	for(i = 0; i < num; i++)
	{
		sprintf(val, "value%03d", i);
		pos = i;
		hashtable_add_mask(ht, key, val, maskarray[i%3], masknum, pos);
		if (ret < 0) {
			DERROR("add value err: %d, %s\n", ret, val);
			return ret;
		}
		
		char ret = hashtable_find_value(ht, key, val, &node, &dbk, &item);
		if (ret < 0) {
			DERROR("not found value: %d, %s\n", ret, key);
			return ret;
		}
	}

	DNOTE("insert %d  ok!\n", num);

	///////test3 : hashtable_move_

	for(i = 0; i < 100; i++)
	{
		//printf("====================\n");
		int v = my_rand(num);
		sprintf(val, "value%03d", v);
		int pos = 0;
		while(1)
		{
			pos = my_rand(num);
			if(pos != v)
				break;
		}
		DINFO("=== hashtable_move val:%s, pos:%d ===\n", val, pos);
		int ret = hashtable_move(ht, key, val, pos);
		if(MEMLINK_OK != ret)
		{
			DERROR("err hashtable_move value:%s, pos:%d\n", val, pos);
			break;
		}
	    DataBlock *dbk = node->data;
	    //int count = 0;
	    char *posaddr  = NULL;
	    //DataBlock *last = NULL;
        /*
	    posaddr = dataitem_lookup(node, pos, 0, &dbk);
        DINFO("posaddr:%p\n", posaddr);
		if(NULL == posaddr)
		{
			DERROR("not found value:%s, pos:%d, posaddr:%p, dbk:%p\n",val, pos, posaddr, dbk);
			return -1;
		}
		else
		{
			ret = memcmp(val, posaddr, valuesize);
			if(0 == ret)
			{
				DINFO("pos:%d is ok!\n", pos);
			}
			else
			{
				DERROR("err value! pos:%d is not the right value%s!\n", pos, val);
				return -1;
			}
		}*/

	}
	//hashtable_print(ht, key);
	DINFO("hashtable_move test end!\n");
	return 0;
}

