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

	int  i;
	char val[64];
	char *maskstr = "8:3:1";
	int	 insertnum = 200;

	for (i = 0; i < insertnum; i++) {
		sprintf(val, "%06d", i);
		ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, 0);	
		if (ret != MEMLINK_OK) {
			DERROR("insert error, ret:%d, key:%s, val:%s, mask:%s, i:%d\n", ret, buf, val, maskstr, i);
			return -3;
		}
	}

	MemLinkStat	stat;
	ret = memlink_cmd_stat(m, buf, &stat);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s, ret:%d\n", buf, ret);
		return -4;
	}
	
	//DINFO("stat blocks:%d data:%d, data_used:%d\n", stat.blocks, stat.data, stat.data_used);

	if (stat.data_used != insertnum) {
		DERROR("insert num error, data_used:%d\n", stat.data_used);
		return -5;
	}

	MemLinkResult   result;

	ret = memlink_cmd_range(m, buf, "::", 0, insertnum, &result);
	if (ret != MEMLINK_OK) {
		DERROR("range error, ret:%d\n", ret);
		return -6;
	}

	MemLinkItem	*item = result.root;

	if (NULL == item) {
		DERROR("range must not null\n");
		return -7;
	}

	i = 200;
	while (item) {
		i--;
		sprintf(val, "%06d", i);
		if (strcmp(item->value, val) != 0) {
			DERROR("range value error, value:%s\n", val);
			return -8;
		}
		item = item->next;
	}
	
	memlink_result_free(&result);

	sprintf(val, "%06d", 300);

	ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, 100);	
	if (ret != MEMLINK_OK) {
		DERROR("insert error, ret:%d\n", ret);
		return -9;
	}

	MemLinkResult   result2;

	ret = memlink_cmd_range(m, buf, "::", 0, insertnum + 1, &result2);
	if (ret != MEMLINK_OK) {
		DERROR("range error, ret:%d\n", ret);
		return -6;
	}

	item = result2.root;

	if (NULL == item) {
		DERROR("range must not null\n");
		return -7;
	}

	i = 200;
	while (item) {
		i--;
		if (i == 99) {
			sprintf(val, "%06d", 300);
			if (strcmp(item->value, val) != 0) {
				DERROR("range value error, item->value:%s, value:%s\n", item->value, val);
				return -8;
			}
			item = item->next;
		}

		sprintf(val, "%06d", i);
		if (strcmp(item->value, val) != 0) {
			DERROR("range value error, value:%s\n", val);
			return -8;
		}
		item = item->next;
	}
	
	memlink_result_free(&result2);

	for (i = 0; i < insertnum; i++) {
		sprintf(val, "%06d", i);
		ret = memlink_cmd_del(m, buf, val, strlen(val));
		if (ret != MEMLINK_OK) {
			DERROR("del error, val:%s, ret:%d\n", val, ret);
			return -9;
		}
	}

	// =======================================================

	for (i = 0; i < insertnum; i++) {
		sprintf(val, "%06d", i);
		ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, i*2 + 1);	
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", buf, val, maskstr, i);
			return -3;
		}
	}

	ret = memlink_cmd_stat(m, buf, &stat);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s\n", buf);
		return -4;
	}
	
	if (stat.data_used != insertnum + 1) {
		DERROR("insert num error, data_used:%d\n", stat.data_used);
		return -5;
	}


	MemLinkResult   result3;

	ret = memlink_cmd_range(m, buf, "::", 0, insertnum + 1, &result3);
	if (ret != MEMLINK_OK) {
		DERROR("range error, ret:%d\n", ret);
		return -6;
	}
	//DINFO("result3 count: %d\n", result3.count);
	item = result3.root;

	if (NULL == item) {
		DERROR("range must not null\n");
		return -7;
	}

	sprintf(val, "%06d", 300);
	if (strcmp(item->value, val) != 0) {
		DERROR("first value error, item->value:%s, val:%s\n", item->value, val);
		return -8;
	}
	item = item->next;

	i = 0;
	while (item) {
		sprintf(val, "%06d", i);
		if (strcmp(item->value, val) != 0) {
			DERROR("range value error, item->value:%s, value:%s\n", item->value, val);
			return -8;
		}
		i++;
		item = item->next;
	}
	
	memlink_result_free(&result3);

	memlink_destroy(m);

	return 0;
}
