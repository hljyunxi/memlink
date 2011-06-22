#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "logfile.h"
#include "hashtable.h"
#include "mem.h"
#include "myconfig.h"
#include "common.h"
#include "hashtest.h"

//#define DEBUG

int main()
{
#ifdef DEBUG
		logfile_create("stdout", 3);
#endif
	HashTable* ht;
	char key[64];
	int  valuesize = 8;
	unsigned int maskformat[3] = {4, 3, 1};
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
	//test1 : hashtable_add_info_mask - create key
	for (i = 0; i < num; i++) {
		sprintf(key, "heihei%03d", i);
		ret = hashtable_key_create_mask(ht, key, valuesize, maskformat, masknum, MEMLINK_LIST, 0);
		if (ret < 0) {
			DERROR("hashtable_add_info_mask error. %s\n", key);
			return -1;
		}
	}
	for (i = 0; i < num; i++) {
		sprintf(key, "heihei%03d", i);
		HashNode* pNode = hashtable_find(ht, key);
		if (NULL == pNode) {
			DERROR("hashtable_add_info_mask error. can not find %s\n", key);
			return -1;
		}
	}

	for (i = 0; i < num; i++) {
		sprintf(key, "heihei%03d", i);
		ret = hashtable_remove_key(ht, key);
		if (ret < 0) {
			DERROR("hashtable_remove_key error. %s\n", key);
			return -1;
		}
	}
	for (i = 0; i < num; i++) {
		sprintf(key, "heihei%03d", i);
		HashNode* pNode = hashtable_find(ht, key);
		if (NULL != pNode) {
			DERROR("hashtable_remove_key error. should not find %s\n", key);
			return -1;
		}
	}
	
	DINFO("test: create and remove keys ok!\n");
	return 0;
}
