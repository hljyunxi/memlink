#include <stdio.h>
#include <stdlib.h>
#include <memlink_client.h>
#include "logfile.h"
#include "test.h"

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
	
	int i;
	char val[64];
	char *maskstr = "6:3:1";
    int  insertnum = 10;

	for (i = 0; i < insertnum; i++) {
		sprintf(val, "%06d", i);
        //DINFO("====== insert:%s ======\n", val);
		ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, 0);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s,val:%s, ret:%d\n", buf, val, ret);
			return -3;
		}
	}

	sprintf(val, "%06d", insertnum / 2);
	ret = memlink_cmd_tag(m, buf, val, strlen(val), MEMLINK_TAG_DEL);
	if (ret != MEMLINK_OK) {
		DERROR("tag error, ret:%d\n", ret);
		return -4;
	}
	
	{
		MemLinkCount	count2;
		ret = memlink_cmd_count(m, buf, "", &count2);
		if (ret != MEMLINK_OK) {
			DERROR("count error, ret:%d\n", ret);
			return -4;
		}
		if (count2.visible_count != insertnum - 1) {
			DERROR("count visible_count error: %d\n", count2.visible_count);
			return -5;
		}
	}
	
	MemLinkResult	result;
	ret = memlink_cmd_range(m, buf, "", 0, 200, &result);
	if (ret != MEMLINK_OK) {
		DERROR("range error, ret:%d\n", ret);
		return -5;
	}
	
	MemLinkItem	*item = result.root;

	while (item) {
		if (strcmp(item->value, val) == 0) {
			DERROR("tag del must not display! error, value:%s\n", val);
			return -6;
		}
		item = item->next;
	}
	
	memlink_result_free(&result);

	ret = memlink_cmd_tag(m, buf, val, strlen(val), MEMLINK_TAG_RESTORE);
	if (ret != MEMLINK_OK) {
		DERROR("tag error, ret:%d\n", ret);
		return -7;
	}
	
	{
		MemLinkCount	count2;
		ret = memlink_cmd_count(m, buf, "", &count2);
		if (ret != MEMLINK_OK) {
			DERROR("count error, ret:%d\n", ret);
			return -4;
		}
		if (count2.visible_count != insertnum) {
			DERROR("count visible_count error: %d\n", count2.visible_count);
			return -5;
		}
	}

	ret = memlink_cmd_range(m, buf, "::", 0, 200, &result);
	if (ret != MEMLINK_OK) {
		DERROR("range error, ret:%d\n", ret);
		return -8;
	}
	
	item = result.root;

	int found = 0;
	while (item) {
		if (strcmp(item->value, val) == 0) {
			found = 1;
		}
		item = item->next;
	}
		
	if (found != 1) {
		DERROR("tag restore must display! error, value:%s\n", val);
		return -9;
	}
	
	memlink_result_free(&result);

	memlink_destroy(m);

	return 0;
}
