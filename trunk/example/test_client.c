#include <stdio.h>
#include <string.h>

#include "memlink_client.h"
#include "logfile.h"

Memlink *m;

int test_init()
{

	logfile_create("stdout", 3);
	m = memlink_create("127.0.0.1", 11001, 11002, 0);
	if (m == NULL) {
		printf("memlink_create error!\n");
	}
}

//create example
int my_test_create_key()
{
	char buf[128] = {0};
	int ret;

	snprintf(buf, 128, "%s", "test");

	ret = memlink_cmd_create(m, buf, 6, "4:3:1");
	DINFO("memlink_cmd_crate: %d\n", ret);
	
	return 1;

}

//insert example
int my_test_insert_key_value()
{
	char buf[128] = {0};
	int ret;

	snprintf(buf, 128, "%s", "myvalue");
	DINFO("INSERT test mykey %s\n");

	ret = memlink_com_insert(m, "test", buf, strlen(buf), "1:2:0", 0);

	DINFO("memlink_cmd_insert: %d\n", ret);

	return 1;
}

//stat example
int my_test_stat()
{
	MemLinkStat stat;
	
	ret = memlink_cmd_stat(m, "test", &stat);
	DINFO("memlink_cmd_stat: %d\n", ret);
	DINFO("valuesize:%d, masksize:%d, blocks:%d, data:%d, data_used:%d, mem:%d, mem_used:%d\n", stat.valuesize, stat.masksize, stat.blocks, stat.data, stat.data_used, stat.mem, stat.mem_used);

	return 1;

}

//range example
int my_test_range()
{
	MemLinkResult result;

	ret = memlink_cmd_range(m, "test", "::", 0, 


}
