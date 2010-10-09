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

    int ret, i;
    //ret = memlink_cmd_dump(m);
    //DINFO("memlink_dump: %d\n", ret);

    //ret = memlink_cmd_clean(m, "haha");

    //ret = memlink_cmd_stat(m, "haha");
    //DINFO("memlink_cmd_xx: %d\n", ret);

   
    
	printf("============== create ===============\n");
    for (i = 0; i < 2; i++) {
        sprintf(buf, "haha%d", i);
        ret = memlink_cmd_create(m, buf, 6, "4:3:1");
        DINFO("memlink_cmd_xx: %d\n", ret);
    }


    //ret = memlink_cmd_del(m, "haha", "gogo", 4);
   
    
    for (i = 0; i < 30; i++) {
        printf("============== insert ===============\n");
        sprintf(buf, "gogo%d", i);
        DINFO("INSERT haha1 %s\n", buf);
        ret = memlink_cmd_insert(m, "haha1", buf, strlen(buf), "1:2:0", 0);
        DINFO("memlink_cmd_insert: %d\n", ret);
    }
	

	
    printf("============= stat ================\n");
    MemLinkStat stat;
    ret = memlink_cmd_stat(m, "haha1", &stat);
    DINFO("memlink_cmd_stat: %d\n", ret);
    DINFO("valuesize:%d, masksize:%d, blocks:%d, data:%d, data_used:%d, mem:%d, mem_used:%d\n", stat.valuesize, stat.masksize, stat.blocks, stat.data, stat.data_used, stat.mem, stat.mem_used);


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
	*/

    for (i = 0; i < 2; i++) {
        printf("============== range %d ===============\n", i);
        MemLinkResult result;
        ret = memlink_cmd_range(m, "haha1", "::", 2, 10, &result);
        DINFO("valuesize:%d, masksize:%d, count:%d\n", result.valuesize, result.masksize,
                result.count);
    }
    

over:
    memlink_destroy(m);

    return 0;
}

