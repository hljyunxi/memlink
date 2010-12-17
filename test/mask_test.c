#include <stdio.h>
#include <stdlib.h>
#include <memlink_client.h>
#include "logfile.h"
#include "test.h"

int check_mask(MemLink *m, char *key, char *newmask)
{
	MemLinkResult	result;
	int				ret;

	ret = memlink_cmd_range(m, key, "::", 0, 1, &result);
	if (ret != MEMLINK_OK) {
		DERROR("range error, key:%s, ret:%d\n", key, ret);
		return -5;
	}

	MemLinkItem	*item = result.root;

	if (NULL == item) {
		DERROR("range not return data, error!, key:%s\n", key);
		return -6;
	}
	if (strcmp(item->mask, newmask) != 0) {
		DERROR("mask set error, item->mask:%s, newmask:%s\n", item->mask, newmask);	
		return -7;
	}
	
	memlink_result_free(&result);

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
	char key[32];

	sprintf(key, "haha");
	ret = memlink_cmd_create(m, key, 6, "4:3:1");
	
	if (ret != MEMLINK_OK) {
		DERROR("1 memlink_cmd_create %s error: %d\n", key, ret);
		return -2;
	}

	char *val		= "111111";
	char *maskstr	= "8:3:1";
	int i;
	char value[64];
	for(i = 0; i < 20; i++)
	{
		sprintf(value, "%06d", i);
		ret = memlink_cmd_insert(m, key, val, strlen(val), maskstr, 0);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s, val:%s, mask:%s, i:%d, ret:%d\n", key, val, maskstr, 1, ret);
			return -3;
		}
	}
	
//////added by wyx 
	maskstr = "1:1:1";
	for(i = 0; i < 20; i++)
	{
		sprintf(value, "%06d", i);
		ret = memlink_cmd_mask(m, key, "xxxx", 4, maskstr);
		if (ret != MEMLINK_ERR_NOVAL) {
			DERROR("mask error, must novalue, key:%s, val:%s, ret:%d\n", key, "xxxx", ret);
			return -4;
		}
	}

	ret = memlink_cmd_mask(m, key, "xxxx", 4, maskstr);
	if (ret != MEMLINK_ERR_NOVAL) {
		DERROR("mask error, must novalue, key:%s, val:%s, ret:%d\n", key, "xxxx", ret);
		return -4;
	}
    ret = memlink_cmd_mask(m, "xxxxxx", "xxxx", 4, maskstr);
	if (ret != MEMLINK_ERR_NOKEY) {
		DERROR("mask error, must nokey, key:%s\n", key);
		return -4;
	}
////////////end

	char *newmask  = "7:2:1";
	ret = memlink_cmd_mask(m, key, val, strlen(val), newmask);
	if (ret != MEMLINK_OK) {
		DERROR("mask error, key:%s, val:%s, mask:%s, ret:%d\n", key, val, newmask, ret);
		return -4;
	}
	check_mask(m, key, newmask);


	newmask  = "7:2:0";
	ret = memlink_cmd_mask(m, key, val, strlen(val), newmask);
	if (ret != MEMLINK_OK) {
		DERROR("mask error, key:%s, val:%s, mask:%s, ret:%d\n", key, val, newmask, ret);
		return -4;
	}
	check_mask(m, key, newmask);

	memlink_destroy(m);

	return 0;
}

