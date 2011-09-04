#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "memlink_client.h"
#include "base/logfile.h"

void* test(void *arg)
{
    MemLink *m;
    char buf[128];
    logfile_create("stdout", LOG_INFO);

    m = memlink_create("127.0.0.1", 11001, 11002, 0);
    if (NULL == m) {
        DINFO("memlink_create error!\n");
    }

    int ret, i;
    char *name = "test";
    DINFO("============== create ===============\n");
    for (i = 0; i < 1; i++) {
        //sprintf(buf, "%s.haha%d", name, i);
        ret = memlink_cmd_create_table_list(m, name, 6, "4:3:1");
        if (ret != MEMLINK_OK) {
            DERROR("memlink_cmd_xx: %d, key:%s\n", ret, buf);
        }
    }

    //ret = memlink_cmd_del(m, "haha", "gogo", 4);

    char *key = "test.haha0";
    while (1) {
        for (i = 0; i < 200; i++) {
            //DINFO("============== insert ===============\n");
            sprintf(buf, "gogo%d", i);
            //DINFO("INSERT haha1 %s\n", buf);
            ret = memlink_cmd_insert(m, key, buf, strlen(buf), "1:2:0", 0);
            if (ret != MEMLINK_OK) {
                DERROR("memlink_cmd_insert error:%d, key:%s, val:%s\n", ret, key, buf);
            }
        }
        for (i = 0; i < 200; i+=3) {
            sprintf(buf, "gogo%d", i);
            ret = memlink_cmd_del(m, key, buf, strlen(buf)); 
            if (ret != MEMLINK_OK) {
                DERROR("del error:%d, key:%s, val:%s\n", ret, key, buf);
            }
        }
        for (i = 1; i < 200; i+=3) {
            sprintf(buf, "gogo%d", i);
            ret = memlink_cmd_del(m, key, buf, strlen(buf)); 
            if (ret != MEMLINK_OK) {
                DERROR("del error:%d, key:%s, val:%s\n", ret, key, buf);
            }
        }
        for (i = 2; i < 200; i+=3) {
            sprintf(buf, "gogo%d", i);
            ret = memlink_cmd_del(m, key, buf, strlen(buf)); 
            if (ret != MEMLINK_OK) {
                DERROR("del error:%d, key:%s, val:%s\n", ret, key, buf);
            }
        }
    }

    /*for (i = 0; i < 1; i++) {
        printf("============== range %d ===============\n", i);
        MemLinkResult result;
        ret = memlink_cmd_range(m, "haha1", 0,  "::", 2, 10, &result);
        DINFO("valuesize:%d, attrsize:%d, count:%d\n", result.valuesize, result.attrsize,
                result.count);
    }*/
    memlink_destroy(m);

    return 0;
}


int main(int argc, char *argv[])
{
    int threadnum = 10;
    pthread_t threads[threadnum];
    int i;

    for (i = 0; i < threadnum; i++) {
        pthread_create(&threads[i], NULL, test, NULL);
    }
    for (i = 0; i < threadnum; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

