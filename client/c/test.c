#include <stdio.h>
#include "memlink_client.h"
#include "logfile.h"

int main(int argc, char *argv[])
{
    MemLink *m;
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

    ret = memlink_cmd_create(m, "haha", 6, "4:3:1");
    DINFO("memlink_cmd_xx: %d\n", ret);

    //ret = memlink_cmd_del(m, "haha", "gogo", 4);
    ret = memlink_cmd_insert(m, "haha", "gogo", 4, "1:2:0", 0);
    DINFO("memlink_cmd_xx: %d\n", ret);


    memlink_destroy(m);

    return 0;
}

