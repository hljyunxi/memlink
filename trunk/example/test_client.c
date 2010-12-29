#include <stdio.h>
#include <string.h>

#include "memlink_client.h"
#include "logfile.h"

MemLink *m;

int test_init()
{

	logfile_create("stdout", 3);
	m = memlink_create("127.0.0.1", 11001, 11002, 0);
	if (m == NULL) {
		printf("memlink_create error!\n");
	}

	return 1;
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
	DINFO("INSERT test mykey %s\n", buf);

	ret = memlink_cmd_insert(m, "test", buf, strlen(buf), "1:2:0", 0);

	DINFO("memlink_cmd_insert: %d\n", ret);

	return 1;
}

//stat example
int my_test_stat()
{
	MemLinkStat stat;
	int ret;
	
	ret = memlink_cmd_stat(m, "test", &stat);
	DINFO("memlink_cmd_stat: %d\n", ret);
	DINFO("valuesize:%d, masksize:%d, blocks:%d, data:%d, data_used:%d, mem:%d\n", stat.valuesize, stat.masksize, stat.blocks, stat.data, stat.data_used, stat.mem);

	return 1;

}

//range example
int my_test_range()
{
	int ret;
	MemLinkResult result;
	MemLinkItem *p ;

	ret = memlink_cmd_range(m, "test", MEMLINK_VALUE_VISIBLE, "::", 0, 2, &result);
	DINFO("valuesize:%d, masksize:%d, count:%d\n", result.valuesize, result.masksize,
		result.count);

	p = result.root;
	while (p != NULL) {
		DINFO("value: %s, mask: %s\n", p->value, p->mask);
		p = p->next;
	}
	return 1;
}

//rmkey example
int my_test_rm_key()
{
	int ret;

	ret = memlink_cmd_rmkey(m, "test");
	DINFO("memlink_cmd_rmkey:%d\n", ret);

	return 1;
}

//count example
int my_test_count()
{
	int ret;
	MemLinkCount count;

	ret = memlink_cmd_count(m, "test", "::", &count);

	DINFO("memlink_cmd_count: ret: %d, visible_count: %d, tagdel_count: %d\n", ret, count.visible_count, count.tagdel_count);
	return 1;
}

//del example
int my_test_del_value()
{
	int ret;
	char key[10];
    char value[10];

	sprintf(key, "%s", "test");
	sprintf(value, "%s", "myvalue");

	ret = memlink_cmd_del(m, key, value, strlen(value));
	DINFO("memlink_cmd_del: %d", ret);
	return 1;
}

//mask example
int my_test_mask()
{
	int ret;
	char key[10];
	char value[10];

	sprintf(key, "%s", "test");
	sprintf(value, "%s", "myvalue");

	ret = memlink_cmd_mask(m, key, value, strlen(value), "1:1:1");
	DINFO("memlink_cmd_mask: %d\n", ret);
	return 1;
}

//dump example
int my_test_dump()
{
	int ret;
	
	ret = memlink_cmd_dump(m);
	if (ret != MEMLINK_OK) {
		DERROR("dump error: %d\n", ret);
	}
	return 1;
}

int test_destroy()
{
	memlink_destroy(m);
	return 1;
}


int main(int argc, char **argv)
{
	test_init();

	test_destroy();

	return 1;
}

