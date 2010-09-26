#include <stdio.h>
#include <string.h>
#include "memlink_client.h"
#include "logfile.h"

int main(int argc, char *argv[])
{
    MemLink *m;
    char buf[128];
    logfile_create("stdout", 3);

    m = memlink_create("127.0.0.1", 11001, 11002, 0);
    if (NULL == m) {
        printf("memlink_create error!\n");
    }

    int ret;
    //ret = memlink_cmd_dump(m);
    //DINFO("memlink_dump: %d\n", ret);

    //ret = memlink_cmd_clean(m, "haha");

    //ret = memlink_cmd_stat(m, "haha");
    //DINFO("memlink_cmd_xx: %d\n", ret);

    printf("=============================\n");
    ret = memlink_cmd_create(m, "haha", 6, "4:3:1");
    DINFO("memlink_cmd_xx: %d\n", ret);

    //ret = memlink_cmd_del(m, "haha", "gogo", 4);
    int i;
    for (i = 0; i < 3; i++) {
        printf("=============================\n");
        sprintf(buf, "gogo%d", i);
        DINFO("INSERT haha %s\n", buf);
        ret = memlink_cmd_insert(m, "haha", buf, strlen(buf), "1:2:0", 0);
        DINFO("memlink_cmd_xx: %d\n", ret);
    }

    printf("=============================\n");
    ret = memlink_cmd_stat(m, "haha");
    DINFO("memlink_cmd_xx: %d\n", ret);

    /*
    printf("=============================\n");
    ret = memlink_cmd_update(m, "haha", "gogo1", 5, 0);
    DINFO("memlink_cmd_xx: %d\n", ret);

    printf("=============================\n");
    ret = memlink_cmd_mask(m, "haha", "gogo2", 5, "10:3:1");
    DINFO("memlink_cmd_xx: %d\n", ret);

    printf("=============================\n");
    ret = memlink_cmd_tag(m, "haha", "gogo1", 5, 1);
    DINFO("memlink_cmd_xx: %d\n", ret);

    printf("=============================\n");
    ret = memlink_cmd_range(m, "haha", "::", 2, 10);
    */

    memlink_destroy(m);

    return 0;
}

