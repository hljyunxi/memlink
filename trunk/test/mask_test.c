#include <stdio.h>
#include <stdlib.h>
#include <memlink_client.h>
#include "logfile.h"

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

	char *val		= "111111";
	char *maskstr	= "8:3:1";

	ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, 1);
	if (ret != MEMLINK_OK) {
		DERROR("insert error, key:%s, val:%s, mask:%s, i:%d, ret:%d\n", buf, val, maskstr, 1, ret);
		return -3;
	}

	char *newmask  = "7:2:0";
	ret = memlink_cmd_mask(m, buf, val, strlen(val), newmask);
	if (ret != MEMLINK_OK) {
		DERROR("mask error, key:%s, val:%s, mask:%s, ret:%d\n", buf, val, newmask, ret);
		return -4;
	}

	MemLinkResult	result;
	
	ret = memlink_cmd_range(m, buf, "::", 0, 1, &result);
	if (ret != MEMLINK_OK) {
		DERROR("range error, key:%s, ret:%d\n", buf, ret);
		return -5;
	}

	MemLinkItem	*item = result.root;

	if (NULL == item) {
		DERROR("range not return data, error!, key:%s\n", buf);
		return -6;
	}
	
	if (strcmp(item->mask, maskstr) != 0) {
		DERROR("mask set error, mask:%s\n", item->mask);	
		return -7;
	}
	
	memlink_result_free(&result);

	memlink_destroy(m);

	return 0;
}

