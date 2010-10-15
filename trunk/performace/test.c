#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <memlink_client.h>
#include "logfile.h"
#include "utils.h"

int test_insert(int count)
{
	MemLink	*m;
	m = memlink_create("127.0.0.1", 11001, 11002, 30);
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
	
	char val[64];
	char *maskstr = "6:2:1";

	int i;
	struct timeval start, end;

	DINFO("start insert ...\n");
	gettimeofday(&start, NULL);
	for (i = 0; i < count; i++) {
		sprintf(val, "%020d", i);
		ret = memlink_cmd_insert(m, key, val, strlen(val), maskstr, 0);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, i:%d, val:%d, ret:%d\n", i, val, ret);
			return -3;
		}
	}
	gettimeofday(&end, NULL);
	DINFO("end insert ...\n");

	unsigned int tmd = timediff(&start, &end);
	DINFO("use time: %u\n", tmd);
	DINFO("speed: %.2f\n", ((double)count / tmd) * 1000000);
	
	memlink_destroy(m);
	return 0;
}


int main()
{
#ifdef DEBUG
	logfile_create("stdout", 3);
#endif
	test_insert(10000);

	return 0;
}
