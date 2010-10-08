#include <stdio.h>
#include <stdlib.h>
#include <memlink_client.h>
#include "logfile.h"
#include "hashtable.h"

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
		DERROR("mem  error: %d\n", stat->mem);
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
	m = memlink_create("127.0.0.1", 11001, 11002, 30);
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

	ret = memlink_cmd_stat(m, buf, &stat);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s, ret:%d\n", buf, ret);
		return -3;
	}

	check(&stat, 6, 2, 0, 0, 0, 0, 0);

	char *val		= "111111";
	char *maskstr	= "8:3:1";

	ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, 1);
	if (ret != MEMLINK_OK) {
		DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", buf, val, maskstr, 1);
		return -5;
	}

	MemLinkStat stat2;

	ret = memlink_cmd_stat(m, buf, &stat2);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s, ret:%d\n", buf, ret);
		return -3;
	}
	int ms  = sizeof(HashNode) + 8 * 100;
	int mus = sizeof(HashNode) + 8;

	check(&stat2, 6, 2, 1, 100, 1, ms, mus);

	memlink_destroy(m);

	return 0;
}

