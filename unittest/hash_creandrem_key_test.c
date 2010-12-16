#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "logfile.h"
#include "hashtable.h"
#include "mem.h"
#include "myconfig.h"
#include "common.h"
//#include "hash_test.h"

//#define DEBUG
int my_rand(int base)
{
	int i = -1;
	usleep(10);
	srand((unsigned)time(NULL)+rand());//在种子里再加一个随机数
	
	while (i < 0 )
	{
		i = rand()%base;
	}
	return i;
}

Runtime*
my_runtime_create_common(char *pgname)
{
    Runtime *rt = (Runtime*)zz_malloc(sizeof(Runtime));
    if (NULL == rt) {
        DERROR("malloc Runtime error!\n");
        MEMLINK_EXIT;
        return NULL; 
    }
    memset(rt, 0, sizeof(Runtime));
    g_runtime = rt;

    realpath(pgname, rt->home);
    char *last = strrchr(rt->home, '/');  
    if (last != NULL) {
        *last = 0;
    }
    DINFO("home: %s\n", rt->home);

    int ret;
    ret = pthread_mutex_init(&rt->mutex, NULL);
    if (ret != 0) {
        DERROR("pthread_mutex_init error: %s\n", strerror(errno));
        MEMLINK_EXIT;
        return NULL;
    }
    DINFO("mutex init ok!\n");

    rt->mpool = mempool_create();
    if (NULL == rt->mpool) {
        DERROR("mempool create error!\n");
        MEMLINK_EXIT;
        return NULL;
    }
    DINFO("mempool create ok!\n");

    rt->ht = hashtable_create();
    if (NULL == rt->ht) {
        DERROR("hashtable_create error!\n");
        MEMLINK_EXIT;
        return NULL;
    }
    DINFO("hashtable create ok!\n");

    rt->server = NULL;
    rt->synclog = NULL;

    return rt;
}


int main()
{
	HashTable* ht;
	char key[64];
	int valuesize = 8;
	char val[64];
	int maskformat[3] = {4, 3, 1};
	char* maskstr1[] = {"8:3:1", "7:2:1", "6:2:1"};
	int maskarray[3][3] = { {8, 3, 1}, { 7, 2,1}, {6, 2, 1} }; 
	int num  = 200;
	int masknum = 3;
	int i = 0;
	int ret;
	
	char *conffile;
	conffile = "memlink.conf";
	DINFO("config file: %s\n", conffile);
    myconfig_create(conffile);
	
	my_runtime_create_common("memlink");

	ht = g_runtime->ht;


	///////////begin test;
	//test1 : hashtable_add_info_mask //添加key
	for(i = 0; i < num; i++)
	{
		sprintf(key, "heihei%03d", i);
		ret = hashtable_key_create_mask(ht, key, valuesize, maskformat, masknum);
		if(ret < 0)
		{
			DERROR("hashtable_add_info_mask error. %s\n", key);
			return -1;
		}
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

	for(i = 0; i < num; i++)
	{
		sprintf(key, "heihei%03d", i);
		ret = hashtable_remove_key(ht, key);
		if(ret < 0)
		{
			DERROR("hashtable_remove_key error. %s\n", key);
			return -1;
		}
	}
	for(i = 0; i < num; i++)
	{
		sprintf(key, "heihei%03d", i);
		HashNode* pNode = hashtable_find(ht, key);
		if(NULL != pNode)
		{
			DERROR("hashtable_remove_key error. should not find %s\n", key);
			return -1;
		}
	}
	
	printf("test: create and remove keys ok!\n");
	return 0;
}
