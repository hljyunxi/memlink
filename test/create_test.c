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
	
	int nodenum = 100;
	int i;
///////////创建100个不同key的hashnode
	for( i = 0 ; i < nodenum; i++) 
	{
		sprintf(key, "haha%d", i);
		ret = memlink_cmd_create(m, key, 6, "4:3:1");
		
		if (ret != MEMLINK_OK) {
			DERROR("1 memlink_cmd_create %s error: %d\n", key, ret);
			return -2;
		}
	}
	
	ret = memlink_cmd_create(m, key, 6, "4:3:1");
	if (ret == MEMLINK_OK) {
		DERROR("2 memlink_cmd_create %s error: %d\n", key, ret);
		return -3;
	}
	
	memlink_destroy(m);

	return 0;
}

