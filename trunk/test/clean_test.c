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
	char key[32];

	sprintf(key, "haha");
	ret = memlink_cmd_create(m, key, 6, "4:3:1");
	
	if (ret != MEMLINK_OK) {
		DERROR("1 memlink_cmd_create %s error: %d\n", key, ret);
		return -2;
	}
	//DINFO("create %s ok!\n", key);

	int		i;
	char	val[64];
	char	*maskstr = "8:3:1";
    int     insertnum = 200;
	for (i = 0; i < insertnum; i++) {
		sprintf(val, "%06d", i);
		//DINFO("====== try insert %s %s %d======\n", key, val, i);
		ret = memlink_cmd_insert(m, key, val, strlen(val), maskstr, i*2);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s, val:%s, mask:%s, i:%d, ret:%d\n", key, val, maskstr, i, ret);

			return -3;
		}
	}

	//DINFO("insert ok!\n");
/*
	MemLinkStat	stat;
	ret = memlink_cmd_stat(m, key, &stat);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s, ret:%d\n", key, ret);
		return -4;
	}
*/	
    for (i = 0; i < insertnum/2; i++) {
        sprintf(val, "%06d", i);
		//DINFO("====== try del %s %s %d======\n", key, val, i);
		ret = memlink_cmd_del(m, key, val, strlen(val));
		if (ret != MEMLINK_OK) {
			DERROR("del error, key:%s, val:%s, i:%d, ret:%d\n", key, val, i, ret);
			return -3;
		}
    }
	
	MemLinkStat	stat;

	ret = memlink_cmd_stat(m, key, &stat);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s, ret:%d\n", key, ret);
		return -4;
	}

	ret = memlink_cmd_clean(m, "HAHAHAH");
	if (ret == MEMLINK_OK) {
		DERROR("must not clean not exist key. ret:%d\n", ret);
		return -4;
	}

	ret = memlink_cmd_clean(m, key);
	if (ret != MEMLINK_OK) {
		DERROR("clean error, key:%s, ret:%d\n", key, ret);
		return -5;
	}

	MemLinkStat	stat2;
	ret = memlink_cmd_stat(m, key, &stat2);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s, ret:%d\n", key, ret);
		return -6;
	}

	if (stat2.data_used != stat.data_used || stat.data_used != insertnum/2) {
		DERROR("stat data_used error, must equal. %d, %d\n", stat.data_used, stat2.data_used);
		return -7;
	}

	//DINFO("stat.data: %d, stat2.data: %d\n", stat.data, stat2.data);
	if (stat2.data >= stat.data) {
		DERROR("stat data error! stat2 must smaller than stat.\n");
		return -8;
	}

	memlink_destroy(m);	

	return 0;
}

