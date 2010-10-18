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
	char *maskstr = "6:3:1";
    int  insertnum = 20;

	for (i = 0; i < insertnum; i++) {
		sprintf(val, "%06d", i);
		ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, i * 2);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s,val:%s, ret:%d\n", buf, val, ret);
			return -3;
		}
	}


	for (i = 0; i < insertnum; i++) {
		sprintf(val, "%06d", i);
        //DINFO("====== insert i:%d\n", i);
		ret = memlink_cmd_update(m, buf, val, strlen(val), 0);
		if (ret != MEMLINK_OK) {
			DERROR("update error, key:%s, val:%s, ret:%d\n", buf, val, ret);
			return -4;
		}

		MemLinkResult result;

		ret = memlink_cmd_range(m, buf, "", 0, 1, &result);
		if (ret != MEMLINK_OK) {
			DERROR("range error, key:%s, ret:%d\n", buf, ret);
			return -5;
		}
		MemLinkItem *item = result.root;
			
		if (item == NULL) {
			DERROR("item is null, key:%s\n", buf);
			return -6;
		}
        //DINFO("item value: %s\n", item->value);
		if (strcmp(item->value, val) != 0) {
			DERROR("after update, first line error! item->value:%s value:%s\n", item->value, val);
			return -7;
		}

		memlink_result_free(&result);
	}

    MemLinkStat     stat;

    ret = memlink_cmd_stat(m, buf, &stat);
    if (ret != MEMLINK_OK) {
        DERROR("stat error, key:%s, ret:%d\n", buf, ret);
        return -8;
    }

	
	memlink_destroy(m);

	return 0;
}
