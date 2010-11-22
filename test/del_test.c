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
	char buf[64];
	
	sprintf(buf, "haha");
	ret = memlink_cmd_create(m, buf, 6, "4:3:2");
	if (ret != MEMLINK_OK) {
		DERROR("memlink_cmd_create %s error: %d\n", buf, ret);
		return -2;
	}

	int i;
	char *maskstr = "8:3:1";
	char val[64];

	for (i = 0; i < 100; i++) {
		sprintf(val, "%06d", i);

		ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, i);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s, value:%s, mask:%s, i:%d\n", buf, val, maskstr, i);
			return -3;
		}
	}
	
	char buf2[64];

	ret = memlink_cmd_del(m, buf, "xxxx", 4);
	if (ret != MEMLINK_ERR_NOVAL) {
		DERROR("del error, must novalue, key:%s, val:%s, ret:%d\n", buf, "xxxx", ret);
		return -4;
	}

    ret = memlink_cmd_del(m, "xxxxxx", "xxxx", 4);
	if (ret != MEMLINK_ERR_NOKEY) {
		DERROR("del error, must nokey, key:%s\n", buf);
		return -4;
	}

    //goto memlink_over;

	for (i = 0; i < 10; i++) {
		sprintf(val, "%06d", i*2);
		
		ret = memlink_cmd_del(m, buf, val, strlen(val));
		if (ret != MEMLINK_OK) {
			DERROR("del error, key:%s, val:%s\n", buf, val);
			return -5;
		}

		MemLinkStat	stat;

		ret = memlink_cmd_stat(m, buf, &stat);
		if (ret != MEMLINK_OK) {
			DERROR("stat error, key:%s\n", buf);
		}
        
        //DINFO("stat.data_used:%d, i:%d, v:%d\n", stat.data_used, i, 100-i-1);
		if (stat.data_used != 100 - i - 1) {
			DERROR("del not remove item! key:%s, val:%s\n", buf, val);
		}

		MemLinkResult	result;

		ret = memlink_cmd_range(m, buf, "", 0, 100, &result);
		if (ret != MEMLINK_OK) {
			DERROR("range error, ret:%d\n", ret);
			return -6;
		}
        //DINFO("result.count:%d, i:%d \n", result.count, i);
        
		MemLinkItem		*item = result.root;

		if (NULL == item) {
			DERROR("range result must not null.\n");
			return -7;
		}

		while (item) {
			if (strcmp(item->value, val) == 0) {
				DERROR("found delete item!, del error. val:%s\n", val);
				return -8;
			}
			item = item->next;
		}
	
		memlink_result_free(&result);
	}

	i = 99;
	sprintf(val, "%06d", i);
	ret = memlink_cmd_del(m, buf, val, strlen(val));		
	ret = memlink_cmd_del(m, buf, val, strlen(val));
	//DINFO("ret:%d, val:%d \n", ret, i);
	if (ret == MEMLINK_OK) {
		DERROR("del error, key:%s, val:%s, ret: %d\n", buf, val, ret);
		return -5;
	}
	
memlink_over:
	memlink_destroy(m);

	return 0;
}
