#include "perf.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <pthread.h>
#include "logfile.h"
#include "utils.h"
#include "memlink_client.h"

#define MEMLINK_HOST        "127.0.0.1"
#define MEMLINK_PORT_READ   11001
#define MEMLINK_PORT_WRITE  11002
#define MEMLINK_TIMEOUT     30

int test_create(TestArgs *args)
{
    MemLink *m;
    int ret, i;
    char key[512] = {0};
    
    DINFO("====== create ======\n");

    if (args->longconn) {
        m = memlink_create(MEMLINK_HOST, MEMLINK_PORT_READ, MEMLINK_PORT_WRITE, MEMLINK_TIMEOUT);
        if (NULL == m) {
            DERROR("memlink_create error!\n");
            exit(-1);
            return -1;
        }
        for (i = 0; i < args->testcount; i++) {
            sprintf(key, "%s%d", args->key, i);
            ret = memlink_cmd_create_list(m, key, args->valuesize, args->maskstr);
            if (ret != MEMLINK_OK) {
                DERROR("create list error! ret:%d\n", ret);
                return -2;
            }
        }
        memlink_destroy(m); 
    }else{
        for (i = 0; i < args->testcount; i++) {
            m = memlink_create(MEMLINK_HOST, MEMLINK_PORT_READ, MEMLINK_PORT_WRITE, MEMLINK_TIMEOUT);
            if (NULL == m) {
                DERROR("memlink_create error!\n");
                exit(-1);
                return -1;
            }
            sprintf(key, "%s%d", args->key, i);
            ret = memlink_cmd_create_list(m, key, args->valuesize, args->maskstr);
            if (ret != MEMLINK_OK) {
                DERROR("create list error! ret:%d\n", ret);
                return -2;
            }
            memlink_destroy(m); 
        }
    }

    return 0;
}

int test_insert(TestArgs *args)
{
    MemLink *m;
    int ret, i;
    char value[512] = {0};
    char format[64] = {0};
    
    DINFO("====== insert ======\n");

    if (args->key[0] == 0 || args->valuesize == 0) {
        DERROR("key and valuesize must not null!\n");
        return -1;
    }

    sprintf(format, "%%0%dd", args->valuesize);

    if (args->longconn) {
        m = memlink_create(MEMLINK_HOST, MEMLINK_PORT_READ, MEMLINK_PORT_WRITE, MEMLINK_TIMEOUT);
        if (NULL == m) {
            DERROR("memlink_create error!\n");
            exit(-1);
            return -1;
        }
        for (i = 0; i < args->testcount; i++) {
            sprintf(value, format, i);
            ret = memlink_cmd_insert(m, args->key, value, args->valuesize, args->maskstr, args->pos);
            if (ret != MEMLINK_OK) {
                DERROR("insert error! ret:%d, key:%s,value:%s,mask:%s,pos:%d\n", 
                        ret, args->key, value, args->maskstr, args->pos);
                return -2;
            }
        }
        memlink_destroy(m); 
    }else{
        for (i = 0; i < args->testcount; i++) {
            m = memlink_create(MEMLINK_HOST, MEMLINK_PORT_READ, MEMLINK_PORT_WRITE, MEMLINK_TIMEOUT);
            if (NULL == m) {
                DERROR("memlink_create error!\n");
                exit(-1);
                return -1;
            }
            sprintf(value, format, i);
            ret = memlink_cmd_insert(m, args->key, value, args->valuesize, args->maskstr, args->pos);
            if (ret != MEMLINK_OK) {
                DERROR("insert error! ret:%d, key:%s,value:%s,mask:%s,pos:%d\n", 
                        ret, args->key, value, args->maskstr, args->pos);
                return -2;
            }
            memlink_destroy(m); 
        }
    }

    return 0;
}

int test_range(TestArgs *args)
{
    MemLink *m;
    int ret, i;

    DINFO("====== range ======\n");

    if (args->longconn) {
        m = memlink_create(MEMLINK_HOST, MEMLINK_PORT_READ, MEMLINK_PORT_WRITE, MEMLINK_TIMEOUT);
        if (NULL == m) {
            DERROR("memlink_create error!\n");
            exit(-1);
            return -1;
        }
        for (i = 0; i < args->testcount; i++) {
            MemLinkResult   result;
            ret = memlink_cmd_range(m, args->key, args->kind, args->maskstr, args->from, args->len, &result);
            if (ret != MEMLINK_OK) {
                DERROR("range error! ret:%d, kind:%d,mask:%s,from:%d,len:%d\n", 
                        ret, args->kind, args->maskstr, args->from, args->len);
                return -2;
            }
            if (result.count != args->len) {
                DERROR("range result count error: %d, len:%d\n", result.count, args->len);
            }
            memlink_result_free(&result);
        }
        memlink_destroy(m); 
    }else{
        for (i = 0; i < args->testcount; i++) {
            m = memlink_create(MEMLINK_HOST, MEMLINK_PORT_READ, MEMLINK_PORT_WRITE, MEMLINK_TIMEOUT);
            if (NULL == m) {
                DERROR("memlink_create error!\n");
                exit(-1);
                return -1;
            }
            MemLinkResult result;
            ret = memlink_cmd_range(m, args->key, args->kind, args->maskstr, args->from, args->len, &result);
            if (ret != MEMLINK_OK) {
                DERROR("range error! ret:%d, kind:%d,mask:%s,from:%d,len:%d\n", 
                        ret, args->kind, args->maskstr, args->from, args->len);
                return -2;
            }
            if (result.count != args->len) {
                DERROR("range result count error: %d, len:%d\n", result.count, args->len);
            }
            memlink_result_free(&result);
            memlink_destroy(m); 
        }
    }

    return 0;
}

int test_move(TestArgs *args)
{
    MemLink *m;
    int ret, i;
    char value[512] = {0};
    int valint = atoi(args->value);
    char format[64];

    DINFO("====== move ======\n");

    sprintf(format, "%%0%dd", args->valuesize);

    if (args->longconn) {
        m = memlink_create(MEMLINK_HOST, MEMLINK_PORT_READ, MEMLINK_PORT_WRITE, MEMLINK_TIMEOUT);
        if (NULL == m) {
            DERROR("memlink_create error!\n");
            exit(-1);
            return -1;
        }
        for (i = 0; i < args->testcount; i++) {
            sprintf(value, format, valint);
            ret = memlink_cmd_move(m, args->key, value, args->valuesize, args->pos);
            if (ret != MEMLINK_OK) {
                DERROR("move error! ret:%d, key:%s,value:%s,pos:%d\n", ret, args->key, value, args->pos);
                return -2;
            }
            valint++;
        }
        memlink_destroy(m); 
    }else{
        for (i = 0; i < args->testcount; i++) {
            m = memlink_create(MEMLINK_HOST, MEMLINK_PORT_READ, MEMLINK_PORT_WRITE, MEMLINK_TIMEOUT);
            if (NULL == m) {
                DERROR("memlink_create error!\n");
                exit(-1);
                return -1;
            }
            sprintf(value, format, valint);
            ret = memlink_cmd_move(m, args->key, value, args->valuesize, args->pos);
            if (ret != MEMLINK_OK) {
                DERROR("move error! ret:%d, key:%s,value:%s,pos:%d\n", ret, args->key, value, args->pos);
                return -2;
            }
            valint++;
            memlink_destroy(m); 
        }
    }

    return 0;
}
int test_del(TestArgs *args)
{
    return 0;
}
int test_mask(TestArgs *args)
{
    MemLink *m;
    int ret, i;
    
    DINFO("====== mask ======\n");

    if (args->longconn) {
        m = memlink_create(MEMLINK_HOST, MEMLINK_PORT_READ, MEMLINK_PORT_WRITE, MEMLINK_TIMEOUT);
        if (NULL == m) {
            DERROR("memlink_create error!\n");
            exit(-1);
            return -1;
        }
        for (i = 0; i < args->testcount; i++) {
            ret = memlink_cmd_mask(m, args->key, args->value, args->valuelen, args->maskstr);
            if (ret != MEMLINK_OK) {
                DERROR("mask error! ret:%d, key:%s, value:%s, mask:%s\n", ret, args->key, args->value, args->maskstr);
                return -2;
            }
        }
        memlink_destroy(m); 
    }else{
        for (i = 0; i < args->testcount; i++) {
            m = memlink_create(MEMLINK_HOST, MEMLINK_PORT_READ, MEMLINK_PORT_WRITE, MEMLINK_TIMEOUT);
            if (NULL == m) {
                DERROR("memlink_create error!\n");
                exit(-1);
                return -1;
            }
            ret = memlink_cmd_mask(m, args->key, args->value, args->valuelen, args->maskstr);
            if (ret != MEMLINK_OK) {
                DERROR("mask error! ret:%d, key:%s, value:%s, mask:%s\n", ret, args->key, args->value, args->maskstr);
                return -2;
            }
            memlink_destroy(m); 
        }
    }

    return 0;
}

int test_tag(TestArgs *args)
{
    return 0;
}

int test_count(TestArgs *args)
{
    return 0;
}

volatile int    thread_create_count = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;

void* thread_start(void *args)
{
    ThreadArgs  *ta = (ThreadArgs*)args;
    struct timeval start, end;  

    pthread_mutex_lock(&lock);
    while (thread_create_count < ta->threads) {
        pthread_cond_wait(&cond, &lock);
        DINFO("cond wait return ...\n");
    }   
    pthread_mutex_unlock(&lock);

    gettimeofday(&start, NULL);
    ta->func(ta->args);
    gettimeofday(&end, NULL);

    unsigned int tmd = timediff(&start, &end);
    double speed = ((double)ta->args->testcount / tmd) * 1000000;
    DINFO("thread test use time:%u, speed:%.2f\n", tmd, speed);

    return NULL;
}

int single_start(ThreadArgs *ta)
{
    struct timeval start, end;  
	
	gettimeofday(&start, NULL);
    ta->func(ta->args);
    gettimeofday(&end, NULL);

    unsigned int tmd = timediff(&start, &end);
    double speed = ((double)ta->args->testcount / tmd) * 1000000;
    DINFO("thread test use time:%u, speed:%.2f\n", tmd, speed);

	return 0;
}

int test_start(TestConfig *cf)
{
    pthread_t       threads[THREAD_MAX_NUM] = {0};
    struct timeval  start, end;
    int i, ret;

    DINFO("====== test start with thread:%d ======\n", cf->threads);
    for (i = 0; i < cf->threads; i++) {
        ThreadArgs  *ta = (ThreadArgs*)malloc(sizeof(ThreadArgs));
        ta->func = cf->func;
        ta->args = &cf->args;
        ta->threads = cf->threads;
        
        ret = pthread_create(&threads[i], NULL, thread_start, ta); 
        if (ret != 0) {
            DERROR("pthread_create error! %s\n", strerror(errno));
            return -1;
        }
        thread_create_count += 1;
    }

    gettimeofday(&start, NULL);
    ret = pthread_cond_broadcast(&cond);
    if (ret != 0) {
        DERROR("pthread_cond_broadcase error: %s\n", strerror(errno));
    }
    for (i = 0; i < cf->threads; i++) {
        pthread_join(threads[i], NULL);
    }
    gettimeofday(&end, NULL);

    unsigned int tmd = timediff(&start, &end);
    double speed = (((double)cf->args.testcount * cf->threads)/ tmd) * 1000000;
    DINFO("thread all test use time:%u, speed:%.2f\n", tmd, speed);

    return 0;
}

TestFuncLink gfuncs[] = {{"create", test_create}, 
                         {"insert", test_insert},
                         {"range", test_range},
                         {"move", test_move},
                         {"del", test_del},
                         {"mask", test_mask},
                         {"tag", test_tag},
                         {"count", test_count},
                         {NULL, NULL}};

TestFunc funclink_find(char *name)
{
    int i = 0;
    TestFuncLink    *flink;

    while (1) {
        flink = &gfuncs[i];
        if (flink->name == NULL)
            break;
        if (strcmp(flink->name, name) == 0) {
            return flink->func;
        }
        i++;
    }
    return NULL;
}


int show_help()
{
	printf("usage:\n\tperf [options]\n");
    printf("options:\n");
    printf("\t--help,-h\tshow help information\n");
    printf("\t--thread,-t\tthread count for test\n");
    printf("\t--count,-n\tinsert number\n");
    printf("\t--from,-f\trange from position\n");
    printf("\t--len,-l\trange length\n");
    printf("\t--do,-d\t\tcreate/insert/range/move/del/mask/tag/count\n");
    printf("\t--key,-k\tkey\n");
    printf("\t--value,-v\tvalue\n");
    printf("\t--valuesize,-s\tvaluesize for create\n");
    printf("\t--pos,-p\tposition for insert\n");
    printf("\t--popnum,-o\tpop number\n");
    printf("\t--kind,-i\tkind for range. all/visible/tagdel\n");
    printf("\t--mask,-m\tmask str\n");
    printf("\t--longconn,-c\tuse long connection for test. default 1\n");
    printf("\n\n");

	exit(0);
}

int main(int argc, char *argv[])
{
	logfile_create("stdout", 4);

	int     optret;
    int     optidx = 0;
    char    *optstr = "ht:n:f:l:k:v:s:p:o:i:a:m:d:c:";
    struct option myoptions[] = {{"help", 0, NULL, 'h'},
                                 {"thread", 0, NULL, 't'},
                                 {"count", 0, NULL, 'n'},
                                 {"frompos", 0, NULL, 'f'},
                                 {"len", 0, NULL, 'l'},
                                 {"key", 0, NULL, 'k'},
                                 {"value", 0, NULL, 'v'},
                                 {"valuesize", 0, NULL, 's'},
                                 {"pos", 0, NULL, 'p'},
                                 {"popnum", 0, NULL, 'o'},
                                 {"kind", 0, NULL, 'i'},
                                 {"tag", 0, NULL, 'a'},
                                 {"mask", 0, NULL, 'm'},
                                 {"do", 0, NULL, 'd'},
                                 {"longconn", 0, NULL, 'c'},
                                 {NULL, 0, NULL, 0}};


    if (argc < 2) {
        show_help();
        return 0;
    }
	
	TestConfig tcf;
    memset(&tcf, 0, sizeof(TestConfig));

    while(optret = getopt_long(argc, argv, optstr, myoptions, &optidx)) {
        if (0 > optret) {
            break;
        }

        switch (optret) {
        case 'f':
			tcf.args.from = atoi(optarg);	
            break;
		case 't':
			tcf.threads = atoi(optarg);
			break;
		case 'n':
			tcf.args.testcount   = atoi(optarg);
			break;
		case 'c':
			tcf.args.longconn = atoi(optarg);
			break;
		case 'l':
			tcf.args.len = atoi(optarg);
			break;		
		case 'd':
			snprintf(tcf.action, 64, "%s", optarg);
            tcf.func = funclink_find(tcf.action);
            if (tcf.func == NULL) {
                printf("--do/-d error: %s\n", optarg);
                exit(0);
            }
			break;
        case 'k':
			snprintf(tcf.args.key, 255, "%s", optarg);
            break;
        case 'v':
			snprintf(tcf.args.value, 255, "%s", optarg);
            tcf.args.valuelen = strlen(optarg);
            break;
        case 's':
            tcf.args.valuesize = atoi(optarg);
            break;
        case 'p':
            tcf.args.pos = atoi(optarg);
            break;
        case 'o':
            tcf.args.popnum = atoi(optarg);
            break;
        case 'i':
            if (strcmp(optarg, "all") == 0) {
                tcf.args.kind = MEMLINK_VALUE_ALL;
            }else if (strcmp(optarg, "visible") == 0) {
                tcf.args.kind = MEMLINK_VALUE_VISIBLE;
            }else if (strcmp(optarg, "tagdel") == 0) {
                tcf.args.kind = MEMLINK_VALUE_TAGDEL;
            }else{
                printf("--kind/-i set error! %s\n", optarg);
                exit(0);
            }
            break;
        case 'a':
            if (strcmp(optarg, "del") == 0) {
                tcf.args.tag = MEMLINK_TAG_DEL;
            }else if (strcmp(optarg, "restore") == 0) {
                tcf.args.tag = MEMLINK_TAG_RESTORE;
            }else{
                printf("--tag/-a set error! %s\n", optarg);
                exit(0);
            }
            break;
        case 'm':
			snprintf(tcf.args.maskstr, 255, "%s", optarg);
            break;
        case 'h':
            show_help();
            break;
        default:
			printf("error option: %c\n", optret);
			exit(-1);
            break;
        }
    }
	
	if (tcf.threads > 0) {
		test_start(&tcf);
	}else{
		ThreadArgs  ta;// = (ThreadArgs*)malloc(sizeof(ThreadArgs));
		ta.func = tcf.func;
		ta.args = &tcf.args;
		ta.threads = tcf.threads;
        
		single_start(&ta);
	}

	return 0;
}
