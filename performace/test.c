#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>
#include <memlink_client.h>
#include "logfile.h"
#include "utils.h"

int isthread = 0;

int clear_key()
{
	MemLink	*m;
    
    //DINFO("=== rmkey ===\n");
    m = memlink_create("127.0.0.1", 11001, 11002, 30);
	if (NULL == m) {
		DERROR("memlink_create error!\n");
		return -1;
	}

	int  ret;
	char key[32];

	sprintf(key, "haha");
   
    ret = memlink_cmd_rmkey(m, key);
    if (ret != MEMLINK_OK) {
        DERROR("rmkey error! ret:%d\n", ret);
        return -1;
    }
    
    MemLinkStat     stat;

    ret = memlink_cmd_stat(m, key, &stat);
    if (ret != MEMLINK_ERR_NOKEY) {
        DERROR("stat result error! ret:%d\n", ret);
        return -2;
    }

    memlink_destroy(m);

    return 0;
}

int create_key(char *key)
{
	MemLink	*m;
    
    //DINFO("=== rmkey ===\n");
    m = memlink_create("127.0.0.1", 11001, 11002, 30);
	if (NULL == m) {
		DERROR("memlink_create error!\n");
		return -1;
	}

	int  ret;
   
    ret = memlink_cmd_create(m, key, 12, "4:3:1");
    if (ret != MEMLINK_OK) {
        DERROR("create error! ret:%d\n", ret);
        return -1;
    }
    
    memlink_destroy(m);

    return 0;
}


int test_insert_long(int count, int docreate)
{
	MemLink	*m;
	struct timeval start, end;

	//DINFO("=== test_insert ===\n");
	gettimeofday(&start, NULL);
	m = memlink_create("127.0.0.1", 11001, 11002, 30);
	if (NULL == m) {
		DERROR("memlink_create error!\n");
		return -1;
	}

	int  ret;
	char key[32];

	sprintf(key, "haha");
    if (docreate == 1) {
        ret = memlink_cmd_create(m, key, 6, "4:3:1");

        if (ret != MEMLINK_OK) {
            DERROR("create %s error: %d\n", key, ret);
            return -2;
        }
    }
	
	char val[64];
	char *maskstr = "6:2:1";

	int i;
	for (i = 0; i < count; i++) {
		sprintf(val, "%020d", i);
		ret = memlink_cmd_insert(m, key, val, strlen(val), maskstr, 0);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, i:%d, val:%s, ret:%d\n", i, val, ret);
			return -3;
		}
	}
	gettimeofday(&end, NULL);

    double speed = 0;
   
    if (isthread == 0) {
        unsigned int tmd = timediff(&start, &end);
        speed = ((double)count / tmd) * 1000000;
        DINFO("insert long use time: %u, speed: %.2f\n", tmd, speed);
    }

	memlink_destroy(m);

	return (int)speed;
}


int test_insert_short(int count, int docreate)
{
	MemLink	*m;
	struct timeval start, end;
	int iscreate = 0;

	//DINFO("====== test_insert ======\n");
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

		if (docreate == 1 && iscreate == 0) {
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
			DERROR("insert error, i:%d, val:%s, ret:%d\n", i, val, ret);
			return -3;
		}
		memlink_destroy(m);
	}
	gettimeofday(&end, NULL);

    double speed = 0;
    if (isthread == 0) {
        unsigned int tmd = timediff(&start, &end);
        speed = ((double)count / tmd) * 1000000;
        DINFO("insert short use time: %u, speed: %.2f\n", tmd, speed);
    }
	
	return (int)speed;
}



int test_range_long(int frompos, int rlen, int count)
{
	MemLink	*m;
	struct timeval start, end;
	//DINFO("====== test_range ======\n");

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

	//DINFO("start range ...\n");
	gettimeofday(&start, NULL);
	for (i = 0; i < count; i++) {
		sprintf(val, "%020d", i);
		MemLinkResult	result;
		ret = memlink_cmd_range(m, key, "", frompos, rlen, &result);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, i:%d, val:%s, ret:%d\n", i, val, ret);
			return -3;
		}
		memlink_result_free(&result);
	}
	gettimeofday(&end, NULL);
	//DINFO("end range ... %d\n", i);

    double speed = 0;

    if (isthread == 0) {
        unsigned int tmd = timediff(&start, &end);
        speed = ((double)count / tmd) * 1000000;
        DINFO("range long use time: %u, speed: %.2f\n", tmd, speed);
    }
	
	memlink_destroy(m);

	return (int)speed;
}

int test_range_short(int frompos, int rlen, int count)
{
	MemLink	*m;
	struct timeval start, end;
	int i, ret;
	char *key = "haha";
	//DINFO("====== test_range ======\n");

	//DINFO("start range ...\n");
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
			DERROR("insert error, i:%d, val:%s, ret:%d\n", i, val, ret);
			return -3;
		}
		memlink_result_free(&result);
		memlink_destroy(m);
	}
	gettimeofday(&end, NULL);

    double speed = 0;

    if (isthread == 0) {
        unsigned int tmd = timediff(&start, &end);
        double speed = ((double)count / tmd) * 1000000;
        DINFO("range short use time: %u, speed: %.2f\n", tmd, speed);
    }

	return (int)speed;
}


#define TESTN   4
#define INSERT_TESTS   4
#define RANGE_TESTS   8
#define RANGEN  3

int compare_int(const void *a, const void *b) 
{
    return *(int*)a - *(int*)b;
}

int getmem(int pid)
{
    int     ret = 0;
    FILE    *fp;
    char    path[1024];
    char    line[1024];

    sprintf(path, "/proc/%d/status", pid);

    fp = fopen(path, "r");
    if (NULL == fp) {
        DERROR("open file %s error! %s\n", path, strerror(errno));
        return -1;
    }
    
    while (1) {
        if (fgets(line, 1024, fp) == NULL) {
            break;
        }
        
        if (strncmp(line, "VmRSS", 5) == 0) {
            char buf[64] = {0};
            char *tmp = line + 7;

            while (*tmp == ' ' || *tmp == '\t')
                tmp++;

            int i = 0;
            while (isdigit(*tmp)) {
                buf[i] = *tmp;
                i++;
                tmp++;
            }

            ret = atoi(buf);
            break;
        }
    }

    fclose(fp);

    return ret;
}

typedef int (*insert_func)(int, int);
typedef int (*range_func)(int, int, int);

int alltest()
{
    int testnum[TESTN] = {10000, 100000, 1000000, 10000000};
    int i;
    int insertret[INSERT_TESTS] = {0};
    int n;
    int ret;
    int f;

    insert_func ifuncs[2] = {test_insert_long, test_insert_short};
    range_func  rfuncs[2] = {test_range_long, test_range_short};

    int startmem  = getmem(getpid());
    DINFO("start mem: %d KB\n", startmem);
    int insertmem[TESTN] = {0};

    // test insert
    for (f = 0; f < 2; f++) {
        for (i = 0; i < TESTN - 2; i++) {
            for (n = 0; n < INSERT_TESTS; n++) {
                DINFO("====== insert %d test: %d ======\n", testnum[i], n);
                insert_func func = ifuncs[f];
                ret = func(testnum[i], 1);
                insertret[n] = ret;

                if (n == 0) {
                    insertmem[i] = getmem(getpid());
                    DINFO("mem: %d KB\n", insertmem[i]);
                }

                clear_key();
            }

            qsort(insertret, INSERT_TESTS, sizeof(int), compare_int);

            for (n = 0; n < INSERT_TESTS; n++) {
                printf("n: %d, %d\t", n, insertret[n]);
            }
            printf("\n");

            int sum = 0;

            for (n = 1; n < INSERT_TESTS - 1; n++) {
                sum += insertret[n];
            }

            float av = (float)sum / (INSERT_TESTS - 2);
            DINFO("====== sum: %d, ave: %.2f ======\n", sum, av);

        }
    }
	 
    int rangetest[RANGEN] = {100, 200, 1000};
    //int rangeret[TESTN][RANGEN*2] = {0};
    int rangeret[RANGE_TESTS] = {0};
    int j = 0, k = 0;

    // test range
    for (f = 0; f < 2; f++) {
        for (i = 0; i < TESTN; i++) {
            ret = test_insert_long(testnum[i], 1);
            for (j = 0; j < 2; j++) {
                for (k = 0; k < RANGEN; k++) {
                    int startpos, slen;

                    if (j == 0) {
                        startpos = 0;
                        slen = rangetest[k];
                    }else{
                        startpos = testnum[i] - rangetest[k];
                        slen = rangetest[k];
                    }
                    for (n = 0; n < RANGE_TESTS; n++) {
                        DINFO("====== range %d, from: %d, len:%d, test: %d ======\n", testnum[i], startpos, slen, n);
                        //ret = test_range_long(startpos, slen, 1000);
                        range_func func = rfuncs[f];
                        ret = func(startpos, slen, 1000);

                        //DINFO("====== range %d result: %d ======\n", testnum[i], ret);
                        rangeret[n] = ret;
                    }


                    qsort(rangeret, RANGE_TESTS, sizeof(int), compare_int);

                    for (n = 0; n < RANGE_TESTS; n++) {
                        printf("n: %d, %d\t", n, rangeret[n]);
                    }
                    printf("\n");

                    int sum = 0;

                    for (n = 1; n < RANGE_TESTS - 1; n++) {
                        sum += rangeret[n];
                    }

                    float av = (float)sum / (RANGE_TESTS - 2);
                    DINFO("====== sum: %d, ave: %.2f ======\n", sum, av);
                }
            }

            clear_key();
        }
    }

    return 0;
}

#define TEST_THREAD_NUM 10
int thread_create_count = 0;
pthread_mutex_t	lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t	cond = PTHREAD_COND_INITIALIZER;

typedef struct {
	int		type;
	void	*func;
	int		startpos;
	int		slen;
	int		count;
}ThreadArgs;

void* thread_start (void *args)
{
	ThreadArgs	*a = (ThreadArgs*)args;
	int			ret;

	pthread_mutex_lock(&lock);
	while (thread_create_count < TEST_THREAD_NUM) {
		pthread_cond_wait(&cond, &lock);
        DINFO("cond wait return ...\n");
	}

	pthread_mutex_unlock(&lock);
	
	DINFO("thread run %u\n", (unsigned int)pthread_self());

	if (a->type == 1) {  //insert
		insert_func	func = a->func;	
		ret = func(a->count, 0);
	}else{ // range
		range_func	func = a->func;	
		ret = func(a->startpos, a->slen, a->count);
	}

    free(args);

    return (void*)ret;
}

int alltest_thread()
{
    int testnum[TESTN] = {10000, 100000, 1000000, 10000000};
    int i;
    int insertret[INSERT_TESTS] = {0};
    int n;
    int ret;
    int f;
    int t;
    insert_func ifuncs[2] = {test_insert_long, test_insert_short};
    range_func  rfuncs[2] = {test_range_long, test_range_short};

    int startmem  = getmem(getpid());
    int insertmem[TESTN] = {0};
    pthread_t   threads[TEST_THREAD_NUM];
    struct timeval  start, end;

    isthread = 1;

    // test insert
    for (f = 0; f < 2; f++) {
        for (i = 0; i < TESTN; i++) {
            for (n = 0; n < INSERT_TESTS; n++) {
                DINFO("====== insert %d test: %d ======\n", testnum[i], n);
                create_key("haha");
				
                int thnum = testnum[i] / TEST_THREAD_NUM;
                for (t = 0; t < TEST_THREAD_NUM; t++) {
					ThreadArgs *ta = (ThreadArgs*)malloc(sizeof(ThreadArgs));
					memset(ta, 0, sizeof(ThreadArgs));
					ta->type = 1;
					ta->func = ifuncs[f];
					ta->count = thnum;

                    ret = pthread_create(&threads[t], NULL, thread_start, ta);
                    if (ret != 0) {
                        DERROR("pthread_create error! %s\n", strerror(errno));
                        return -1;
                    }
					thread_create_count += 1;
                }

                ret = pthread_cond_broadcast(&cond);
                if (ret != 0) {
                    DERROR("pthread_cond_signal error: %s\n", strerror(errno));
                }

                gettimeofday(&start, NULL);
                for (t = 0; t < TEST_THREAD_NUM; t++) {
                    pthread_join(threads[t], NULL);
                }
                gettimeofday(&end, NULL);
                
                unsigned int tmd = timediff(&start, &end);
                double speed = ((double)testnum[i] / tmd) * 1000000;
                if (f == 0) {
                    DINFO("thread insert long use time: %u, speed: %.2f\n", tmd, speed);
                }else{
                    DINFO("thread insert short use time: %u, speed: %.2f\n", tmd, speed);
                }
                
                insertret[n] = speed;
                clear_key();
            }

            qsort(insertret, INSERT_TESTS, sizeof(int), compare_int);

            for (n = 0; n < INSERT_TESTS; n++) {
                printf("n: %d, %d\t", n, insertret[n]);
            }
            printf("\n");

            int sum = 0;
            for (n = 1; n < INSERT_TESTS - 1; n++) {
                sum += insertret[n];
            }

            float av = (float)sum / (INSERT_TESTS - 2);
            DINFO("====== sum: %d, ave: %.2f ======\n", sum, av);

        }
    }

    int rangetest[RANGEN] = {100, 200, 1000};
    //int rangeret[TESTN][RANGEN*2] = {0};
    int rangeret[RANGE_TESTS] = {0};
    int j = 0, k = 0;
    int range_test_num  = 10000;

    // test range
    for (f = 0; f < 2; f++) {
        for (i = 0; i < TESTN; i++) {
            DINFO("====== insert %d ======\n", testnum[i]);
            ret = test_insert_long(testnum[i], 1);
            for (j = 0; j < 2; j++) {
                for (k = 0; k < RANGEN; k++) {
                    int startpos, slen;

                    if (j == 0) {
                        startpos = 0;
                        slen = rangetest[k];
                    }else{
                        startpos = testnum[i] - rangetest[k];
                        slen = rangetest[k];
                    }
                    
                    int thnum = range_test_num / TEST_THREAD_NUM;
                    for (n = 0; n < RANGE_TESTS; n++) {
                        for (t = 0; t < TEST_THREAD_NUM; t++) {
                            ThreadArgs *ta = (ThreadArgs*)malloc(sizeof(ThreadArgs));
                            memset(ta, 0, sizeof(ThreadArgs));
                            ta->type = 2;
                            ta->func = rfuncs[f];
                            ta->count = thnum;

                            ret = pthread_create(&threads[t], NULL, thread_start, ta);
                            if (ret != 0) {
                                DERROR("pthread_create error! %s\n", strerror(errno));
                                return -1;
                            }
					        thread_create_count += 1;
                        }
                            
                        ret = pthread_cond_broadcast(&cond);
                        if (ret != 0) {
                            DERROR("pthread_cond_signal error: %s\n", strerror(errno));
                        }

                        gettimeofday(&start, NULL);
                        for (t = 0; t < TEST_THREAD_NUM; t++) {
                            pthread_join(threads[t], NULL);
                        }
                        gettimeofday(&end, NULL);
                       
                        unsigned int tmd = timediff(&start, &end);
                        double speed = ((double)range_test_num / tmd) * 1000000;
                        if (f == 0) {
                            DINFO("\33[1mthread range long %d from:%d len:%d time:%u speed:%.2f\33[0m\n", testnum[i], startpos, slen, tmd, speed);
                        }else{
                            DINFO("\33[1mthread range short %d from:%d len:%d time:%u speed:%.2f\33[0m\n", testnum[i], startpos, slen, tmd, speed);
                        }
                        rangeret[n] = speed;
                    }
                    qsort(rangeret, RANGE_TESTS, sizeof(int), compare_int);

                    for (n = 0; n < RANGE_TESTS; n++) {
                        printf("n: %d, %d\t", n, rangeret[n]);
                    }
                    printf("\n");

                    int sum = 0;
                    for (n = 1; n < RANGE_TESTS - 1; n++) {
                        sum += rangeret[n];
                    }
                    float av = (float)sum / (RANGE_TESTS - 2);
                    DINFO("\33[31m====== sum: %d, speed: %.2f ======\33[0m\n", sum, av);
                }
            }
            clear_key();
        }
    }

    return 0;
}


int main(int argc, char *argv[])
{
#ifdef DEBUG
	logfile_create("stdout", 3);
#endif
    if (argc == 1) {
        alltest_thread();
        return 0;
    }

	if (argc != 4) {
		printf("usage: test count range_start range_len\n");
		return 0;
	}

	int count = atoi(argv[1]);
	int range_start = atoi(argv[2]);
	int range_len = atoi(argv[3]);

	test_insert_short(count, 1);
	test_range_short(range_start, range_len, 1000);

	return 0;
}
