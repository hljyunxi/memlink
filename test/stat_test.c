#include <stdio.h>
#include <stdlib.h>
#include <memlink_client.h>
#include "logfile.h"
#include "hashtable.h"
#include "test.h"


int check(MemLinkStat *stat, int vs, int ms, int blocks, int data, int datau, int mem, int memu)
{
	int ret = 0;
	if (stat->valuesize != vs) {
		DERROR("valuesize error: %d\n", stat->valuesize);
		ret =  -1;
	}

	if (stat->masksize != ms) {
		DERROR("masksize error: %d\n", stat->masksize);
		ret =  -1;
	}

	if (stat->blocks != blocks) {
		DERROR("blocks error: %d\n", stat->blocks);
		ret =  -1;
	}
	
	if (stat->data != data)  {
		DERROR("data error: %d\n", stat->data);
		ret =  -1;
	}

	if (stat->data_used != datau) {
		DERROR("data_used error: %d\n", stat->data_used);
		ret =  -1;
	}
	
	if (stat->mem != mem) {
		DERROR("mem  error: %d,  %d\n", stat->mem, mem);
		ret =  -1;
	}

	/*if (stat->mem_used != memu) {
		DERROR("mem_used error: %d\n", stat->mem_used);
	}*/
	return ret;
}


int main()
{
	MemLink	*m;
#ifdef DEBUG
	logfile_create("stdout", 3);
#endif
	m = memlink_create("127.0.0.1", MEMLINK_READ_PORT, MEMLINK_WRITE_PORT, 30);
	if (NULL == m) {
		DERROR("memlink_create error!\n");
		return -1;
	}

	int  ret;
	char buf[32];

	sprintf(buf, "haha");
	ret = memlink_cmd_create_list(m, buf, 6, "4:3:1");
	
	if (ret != MEMLINK_OK) {
		DERROR("1 memlink_cmd_create %s error: %d\n", buf, ret);
		return -2;
	}

	MemLinkStat stat;
	
	int ms  = sizeof(HashNode);
	int mus = 0;
	int block = 0;
	int data = 0;
	int data_used = 0;
	ret = memlink_cmd_stat(m, buf, &stat);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s, ret:%d\n", buf, ret);
		return -3;
	}
	check(&stat, 6, 2, 0, 0, 0, ms, 0);

	char *val		= "111111";
	char *maskstr	= "8:3:1";
	
	///insert 1 value       1
	ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, 0);
	if (ret != MEMLINK_OK) {
		DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", buf, val, maskstr, 0);
		return -5;
	}
	
	MemLinkStat stat2;
	ret = memlink_cmd_stat(m, buf, &stat2);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s, ret:%d\n", buf, ret);
		return -3;
	}
	ms  = sizeof(HashNode) + (8 * 1 + sizeof(DataBlock));
	mus = 0;
	ret = check(&stat2, 6, 2, 1, 1, 1, ms, mus);
	if(ret != 0)
	{
		DERROR("ERROR!\n");
		return -1;
	}
	
	///insert 1 value       2 
	ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, 0);
	if (ret != MEMLINK_OK) {
		DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", buf, val, maskstr, 0);
		return -5;
	}
	MemLinkStat stat3;
	ret = memlink_cmd_stat(m, buf, &stat3);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s, ret:%d\n", buf, ret);
		return -3;
	}
	block = 1;
	data = 100;
	data_used = 2;
	ms  = sizeof(HashNode) + (8 * 100 + sizeof(DataBlock)) * block;
	mus = 0;
	ret = check(&stat3, 6, 2, block, data, data_used, ms, mus);
	if(ret != 0)
		{
			DERROR("ERROR!\n");
			return -1;
		}

///////////////再顺序插入199个 added by wyx: insert 198 value     200
	int insertnum = 200;
	int i;
	for(i = 2; i < insertnum; i++)
	{
		ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, i);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", buf, val, maskstr, i);
			return -5;
		}
	}
	MemLinkStat stat4;
	ret = memlink_cmd_stat(m, buf, &stat4);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s, ret:%d\n", buf, ret);
		return -3;
	}
	block = 2;
	data = 200;
	data_used = 200;
	ms  = sizeof(HashNode) + (8 * 100 + sizeof(DataBlock)) * block;
	ret = check(&stat4, 6, 2, block, data, data_used, ms, mus);
	if(ret != 0)
	{
		DERROR("ERROR!\n");
		return -1;
	}
	
	///insert 1 value       201
	ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, 198);
	if (ret != MEMLINK_OK) {
		DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", buf, val, maskstr, 0);
		return -5;
	}
	MemLinkStat stat5;
	ret = memlink_cmd_stat(m, buf, &stat5);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s, ret:%d\n", buf, ret);
		return -3;
	}
	block = 3;
	data = 201;
	data_used = 201;
	ms  = sizeof(HashNode) + (8 * 100 + sizeof(DataBlock)) * (block - 1);
	ms += 8 * 1 + sizeof(DataBlock);
	ret = check(&stat5, 6, 2, block, data, data_used, ms, mus);
	if(ret != 0)
	{
		DERROR("ERROR!\n");
		return -1;
	}

	///insert 1 value		202
	insertnum = 50;
	for(i = 201; i < 200 + insertnum; i++)
	{
		ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, i);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", buf, val, maskstr, i);
			return -5;
		}
	}
	MemLinkStat stat8;
	ret = memlink_cmd_stat(m, buf, &stat8);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s, ret:%d\n", buf, ret);
		return -3;
	}
	block = 3;
	data = 300;
	data_used = 250;
	ms	= sizeof(HashNode) + (8 * 100 + sizeof(DataBlock)) * (block);
	ret = check(&stat8, 6, 2, block, data, data_used, ms, mus);
	if(ret != 0)
	{
		DERROR("ERROR!\n");
		return -1;
	}

/////空的key
	MemLinkStat stat6; 
	*(buf + 0) = '\0'; 
	ret = memlink_cmd_stat(m, buf, &stat6);
	if (ret == MEMLINK_OK) {
		DERROR("must not stat a NULL key ret:%d\n", ret);
		return -3;
	}
	
/////不存在的key
	MemLinkStat stat7;
	//strcpy(buf, "kkkkk");
	ret = memlink_cmd_stat(m, "KKKKK", &stat7); 
	if (ret == MEMLINK_OK) {
		DERROR("must not stat not exist key ret:%d\n", ret);
		return -3;
	}
///end
	memlink_destroy(m);

	return 0;
}

