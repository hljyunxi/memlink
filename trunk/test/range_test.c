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
	
	int i;
	char val[64];
	char *maskstr = "7:1:1";
	int  insertnum = 100;

	for (i = 0; i < insertnum; i++) {
		sprintf(val, "%06d", i);
		ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, i);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s, val:%s, ret:%d\n", buf, val, ret);
			return -3;
		}
	}

	MemLinkResult	result;
	int				reterr = 0;
	int				range_start = insertnum - 20;
	int				range_count = 10;

	ret = memlink_cmd_range(m, buf, "::", range_start, range_count, &result);
	if (ret != MEMLINK_OK) {
		DERROR("range error, key:%s, ret:%d\n", buf, ret);
		return -4;
	}

	DINFO("range return count: %d\n", result.count);
	if (result.count != range_count) {
		DERROR("range count error, count:%d, key:%s\n", result.count, buf);
		reterr++;
	}

	if (result.valuesize != 6) {
		DERROR("range valuesize error, valuesize:%d, key:%s\n", result.valuesize, buf);
		reterr++;
	}

	if (result.masksize != 2) {
		DERROR("range masksize erro, masksize:%d, key:%s\n", result.masksize, buf);
		reterr++;
	}
		
	MemLinkItem	*item = result.root;
	char testbuf[64];
	int  testi = range_start;

	while (item) {
		sprintf(testbuf, "%06d", testi);
		DINFO("range item, value:%s, mask:%s\n", item->value, item->mask);
		if (strcmp(item->value, testbuf) != 0) {
			DERROR("range value error, value:%s, testvalue:%s\n", item->value, testbuf);
		}
		if (strcmp(item->mask, maskstr) != 0) {
			DERROR("range mask error, mask:%s\n", item->mask);
			reterr++;	
		}

		testi++;
		item = item->next;
	}

	memlink_result_free(&result);
	memlink_destroy(m);

	return reterr;
}
