#include <stdio.h>
#include <stdlib.h>
#include <memlink_client.h>
#include "logfile.h"
#include "hashtable.h"
#include "test.h"


int check(MemLinkStat *stat, int vs, int ms, int blocks, int data, int datau, int mem, int memu)
{
	if (stat->valuesize != vs) {
		DERROR("valuesize error: %d\n", stat->valuesize);
	}

	if (stat->masksize != ms) {
		DERROR("masksize error: %d\n", stat->masksize);
	}

	if (stat->blocks != blocks) {
		DERROR("blocks error: %d\n", stat->blocks);
	}
	
	if (stat->data != data)  {
		DERROR("data error: %d\n", stat->data);
	}

	if (stat->data_used != datau) {
		DERROR("data_used error: %d\n", stat->data_used);
	}
	
	if (stat->mem != mem) {
		DERROR("mem  error: %d,  %d\n", stat->mem, mem);
	}

	if (stat->mem_used != memu) {
		DERROR("mem_used error: %d\n", stat->mem_used);
	}
	return 0;
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
	ret = memlink_cmd_create(m, buf, 6, "4:3:1");
	
	if (ret != MEMLINK_OK) {
		DERROR("1 memlink_cmd_create %s error: %d\n", buf, ret);
		return -2;
	}

	MemLinkStat stat;
	
	int ms  = sizeof(HashNode);
	int mus = 0;
	ret = memlink_cmd_stat(m, buf, &stat);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s, ret:%d\n", buf, ret);
		return -3;
	}

	check(&stat, 6, 2, 0, 0, 0, ms, 0);

	char *val		= "111111";
	char *maskstr	= "8:3:1";
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
	ms  = sizeof(HashNode) + (8 * 100 + sizeof(DataBlock));
	mus = 0;

	check(&stat2, 6, 2, 1, 100, 1, ms, mus);

///////////////再顺序插入199个 added by wyx
	int insertnum = 200;
	int i;
	for(i = 1; i < insertnum; i++)
	{
		ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, i*2);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", buf, val, maskstr, i);
			return -5;
		}
	}

	MemLinkStat stat5;
	ret = memlink_cmd_stat(m, buf, &stat5);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s\n", buf);
		return -4;
	}
	if (stat5.data_used != insertnum) {
		DERROR("data_used num error:%d\n", stat5.data_used);
		return -5;
	}	
	if (stat5.data != insertnum) {
		DERROR("data num error:%d\n", stat5.data);
		return -5;
	}

/////空的key
	MemLinkStat stat3; 
	*(buf + 0) = '\0'; 
	ret = memlink_cmd_stat(m, buf, &stat3);
	if (ret == MEMLINK_OK) {
		DERROR("must not stat a NULL key ret:%d\n", ret);
		return -3;
	}
	
/////不存在的key
	MemLinkStat stat4;
	//strcpy(buf, "kkkkk");
	ret = memlink_cmd_stat(m, "KKKKK", &stat4); 
	if (ret == MEMLINK_OK) {
		DERROR("must not stat not exist key ret:%d\n", ret);
		return -3;
	}
///end
	memlink_destroy(m);

	return 0;
}

