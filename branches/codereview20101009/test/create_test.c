#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
	char buf[32];

	sprintf(buf, "haha");
	ret = memlink_cmd_create(m, buf, 6, "4:3:1");
	
	if (ret != MEMLINK_OK) {
		DERROR("1 memlink_cmd_create %s error: %d\n", buf, ret);
		return -2;
	}

	ret = memlink_cmd_create(m, buf, 6, "4:3:1");
	if (ret == MEMLINK_OK) {
		DERROR("2 memlink_cmd_create %s error: %d\n", buf, ret);
		return -3;
	}
	
	memlink_destroy(m);

	return 0;
}

