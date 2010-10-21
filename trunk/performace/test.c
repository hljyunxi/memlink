#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <memlink_client.h>
#include "logfile.h"
#include "utils.h"

int test_insert_long_conn(int count)
{
	MemLink	*m;
	struct timeval start, end;

	DINFO("====== test_insert ======\n");
	gettimeofday(&start, NULL);
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
		DERROR("create %s error: %d\n", key, ret);
		return -2;
	}
	
	char val[64];
	char *maskstr = "6:2:1";

	int i;

	DINFO("start insert ...\n");
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

int test_insert_short_conn(int count)
{
	MemLink	*m;
	struct timeval start, end;
	int iscreate = 0;

	DINFO("====== test_insert ======\n");
	DINFO("start insert ...\n");
	
	int i, ret;
	char *maskstr = "6:2:1";
	char *key = "haha";

	gettimeofday(&start, NULL);

	for (i = 0; i < count; i++) {
		m = memlink_create("127.0.0.1", 11001, 11002, 30);
		if (NULL == m) {
			DERROR("memlink_create error!\n");
			return -1;
		}

		if (iscreate == 0) {
			ret = memlink_cmd_create(m, key, 6, "4:3:1");

			if (ret != MEMLINK_OK) {
				DERROR("create %s error: %d\n", key, ret);
				return -2;
			}
			iscreate = 1;
		}

		char val[64];

		sprintf(val, "%020d", i);
		ret = memlink_cmd_insert(m, key, val, strlen(val), maskstr, 0);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, i:%d, val:%d, ret:%d\n", i, val, ret);
			return -3;
		}
		memlink_destroy(m);
	}
	gettimeofday(&end, NULL);
	DINFO("end insert ...\n");

	unsigned int tmd = timediff(&start, &end);
	DINFO("use time: %u\n", tmd);
	DINFO("speed: %.2f\n", ((double)count / tmd) * 1000000);
	
	return 0;
}



int test_range_long_conn(int frompos, int rlen, int count)
{
	MemLink	*m;
	struct timeval start, end;
	DINFO("====== test_range ======\n");

	m = memlink_create("127.0.0.1", 11001, 11002, 30);
	if (NULL == m) {
		DERROR("memlink_create error!\n");
		return -1;
	}

	int  ret;
	char key[32];
	char val[32];

	sprintf(key, "haha");
	int i;

	DINFO("start range ...\n");
	gettimeofday(&start, NULL);
	for (i = 0; i < count; i++) {
		sprintf(val, "%020d", i);
		MemLinkResult	result;
		ret = memlink_cmd_range(m, key, "", frompos, rlen, &result);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, i:%d, val:%d, ret:%d\n", i, val, ret);
			return -3;
		}
		memlink_result_free(&result);
	}
	gettimeofday(&end, NULL);
	DINFO("end range ... %d\n", i);

	unsigned int tmd = timediff(&start, &end);
	DINFO("use time: %u\n", tmd);
	DINFO("speed: %.2f\n", ((double)count / tmd) * 1000000);
	
	memlink_destroy(m);

	return 0;
}

int test_range_short_conn(int frompos, int rlen, int count)
{
	MemLink	*m;
	struct timeval start, end;
	int i, ret;
	char *key = "haha";
	DINFO("====== test_range ======\n");

	DINFO("start range ...\n");
	gettimeofday(&start, NULL);

	for (i = 0; i < count; i++) {
		m = memlink_create("127.0.0.1", 11001, 11002, 30);
		if (NULL == m) {
			DERROR("memlink_create error!\n");
			return -1;
		}

		char val[32];

		sprintf(val, "%020d", i);
		MemLinkResult	result;
		ret = memlink_cmd_range(m, key, "", frompos, rlen, &result);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, i:%d, val:%d, ret:%d\n", i, val, ret);
			return -3;
		}
		memlink_result_free(&result);
		memlink_destroy(m);
	}
	gettimeofday(&end, NULL);
	DINFO("end range ... %d\n", i);

	unsigned int tmd = timediff(&start, &end);
	DINFO("use time: %u\n", tmd);
	DINFO("speed: %.2f\n", ((double)count / tmd) * 1000000);
	

	return 0;
}



void* thread_start (void *args)
{

}

int multi_client(int num)
{
	pthread_t	threads[num];
	int			ret, i;
	char		*args;

	for (i = 0; i < num; i++) {
		ret = pthread_create(&threads[i], NULL, mytest, args);
		if (ret != 0) {
			DERROR("pthread_create error! %s\n", strerror(errno));
			return -1;
		}
	}
	

	for (i = 0; i < num; i++) {
		pthread_join(threads[i], NULL);
	}
		
	DINFO("thread all complete!\n");
}
	
int main(int argc, char *argv[])
{
#ifdef DEBUG
	logfile_create("stdout", 3);
#endif

	if (argc != 4) {
		printf("usage: test count range_start range_len\n");
		return 0;
	}
	int count = atoi(argv[1]);
	int range_start = atoi(argv[2]);
	int range_len = atoi(argv[3]);

	test_insert_short_conn(count);
	test_range_short_conn(range_start, range_len, 1000);

	return 0;
}
