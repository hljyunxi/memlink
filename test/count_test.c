#include <stdio.h>
#include <stdlib.h>
#include <memlink_client.h>
#include "logfile.h"

int main()
{
	MemLink	*m;
#ifdef DEBUG
	logfile_create("stdout", 3);
#endif
	
	m = memlink_create("127.0.0.1", 11001, 11002, 30);
	if (NULL == m) {
		DERROR("memlink_create error!\n");
		return -1;
	}
	
	int  ret;
	char key[64];
	
	sprintf(key, "haha");
	ret = memlink_cmd_create(m, key, 6, "4:3:2");
	if (ret != MEMLINK_OK) {
		DERROR("memlink_cmd_create %s error: %d\n", key, ret);
		return -2;
	}

	int i;
	char *maskstr = "8:3:1";
	char val[64];
    int  insertnum = 100;

	for (i = 0; i < insertnum; i++) {
		sprintf(val, "%06d", i);

		ret = memlink_cmd_insert(m, key, val, strlen(val), maskstr, i * 3);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s, value:%s, mask:%s, i:%d\n", key, val, maskstr, i);
			return -3;
		}
	}
    
    MemLinkCount    count;
    ret = memlink_cmd_count(m, key, "", &count);
    if (ret != MEMLINK_OK) {
        DERROR("count error, ret:%d\n", ret);
        return -4;
    }
    if (count.visible_count != insertnum) {
        DERROR("count visible_count error: %d\n", count.visible_count);
        return -5;
    }
    if (count.tagdel_count != 0) {
        DERROR("count tagdel_count error: %d\n", count.tagdel_count);
        return -5;
    }

    int tagcount = 10;
	for (i = 0; i < tagcount; i++) {
		sprintf(val, "%06d", i*2);
		
		ret = memlink_cmd_tag(m, key, val, strlen(val), MEMLINK_TAG_DEL);
		if (ret != MEMLINK_OK) {
			DERROR("del error, key:%s, val:%s\n", key, val);
			return -5;
		}

        MemLinkCount    count2;
        ret = memlink_cmd_count(m, key, "", &count2);
        if (ret != MEMLINK_OK) {
            DERROR("count error, ret:%d\n", ret);
            return -4;
        }
        if (count2.visible_count != insertnum - i - 1) {
            DERROR("count visible_count error: %d\n", count2.visible_count);
            return -5;
        }
        if (count2.tagdel_count != i + 1) {
            DERROR("count tagdel_count error: %d\n", count2.tagdel_count);
            return -5;
        }
	}

	int restore_count = 5;
	for (i = 0; i < restore_count; i++) {
		sprintf(val, "%06d", i*2);
		
		ret = memlink_cmd_tag(m, key, val, strlen(val), MEMLINK_TAG_RESTORE);
		if (ret != MEMLINK_OK) {
			DERROR("tag restroe error, ret:%d, key:%s, val:%s\n", ret, key, val);
			return -5;
		}

        MemLinkCount    count2;
        ret = memlink_cmd_count(m, key, "", &count2);
        if (ret != MEMLINK_OK) {
            DERROR("count error, ret:%d\n", ret);
            return -4;
        }
        if (count2.visible_count != insertnum - tagcount + i + 1) {
            DERROR("count visible_count error: %d, must:%d\n", count2.visible_count, insertnum - tagcount + i + 1);
            return -5;
        }
        if (count2.tagdel_count != tagcount - i - 1) {
            DERROR("count tagdel_count error: %d, must:%d\n", count2.tagdel_count, tagcount - i - 1);
            return -5;
        }
	}


	for (i = 50; i < 60; i++) {
		sprintf(val, "%06d", i);
		
		ret = memlink_cmd_del(m, key, val, strlen(val));
		if (ret != MEMLINK_OK) {
			DERROR("del error, key:%s, val:%s\n", key, val);
			return -5;
		}

        MemLinkCount    count2;
        ret = memlink_cmd_count(m, key, "", &count2);
        if (ret != MEMLINK_OK) {
            DERROR("count error, ret:%d\n", ret);
            return -4;
        }
        if (count2.visible_count != insertnum - tagcount - i - 1 + 50 + restore_count) {
            DERROR("count visible_count error, i:%d, count2.visible_count:%d, v:%d\n", i, count2.visible_count, insertnum - tagcount - i -1);
            return -5;
        }
        if (count2.tagdel_count != tagcount - restore_count) {
            DERROR("count tagdel_count error: %d\n", count2.tagdel_count);
            return -5;
        }
	}

	memlink_destroy(m);

	return 0;
}
