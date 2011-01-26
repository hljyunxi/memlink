#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
	ret = memlink_cmd_create_list(m, key, 6, "4:3:1");
	if (ret != MEMLINK_OK) {
		DERROR("create %s error: %d\n", key, ret);
		return -2;
	}

    ret = memlink_cmd_rmkey(m, key);
    if (ret != MEMLINK_OK) {
        DERROR("rmkey error, key:%s, ret:%d\n", key, ret);
        return -3;
    }

	ret = memlink_cmd_create_list(m, key, 6, "4:3:1");
	if (ret != MEMLINK_OK) {
		DERROR("create %s error: %d\n", key, ret);
		return -4;
	}

	memlink_destroy(m);

	return 0;
}

