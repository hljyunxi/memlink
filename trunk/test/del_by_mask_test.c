#include <stdio.h>
#include <stdlib.h>
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
		printf("memlink_create error!\n");
		return -1;
	}
	
	int  ret;
	char buf[64];
	
	sprintf(buf, "haha");
	ret = memlink_cmd_create_list(m, buf, 6, "4:3:2");
	if (ret != MEMLINK_OK) {
		printf("memlink_cmd_create %s error: %d\n", buf, ret);
		return -2;
	}

	int i;
	char *maskstr = "8:3:1";
	char val[64];

	for (i = 0; i < 100; i++) {
		sprintf(val, "%06d", i);

		ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, i);
		if (ret != MEMLINK_OK) {
			printf("insert error, key:%s, value:%s, mask:%s, i:%d\n", buf, val, maskstr, i);
			return -3;
		}
	}
	printf("insert 100!\n");

	ret = memlink_cmd_del_by_mask(m, buf, ":3:1");	
	if (ret != MEMLINK_OK) {
		printf("del_by_mask key:%s, ret:%d\n", buf, ret);
		return -4;
	}
	printf("del_by_mask %s!\n", ":3:1");

	MemLinkResult rs;
	int frompos = 0;
	int len = 100;
	
	ret = memlink_cmd_range(m, buf, MEMLINK_VALUE_VISIBLE,  "", frompos, len, &rs);
	if (ret != MEMLINK_OK)
	{
		printf("range error, key:%s, mask:%s\n", buf, "");
		return -3;
	}
	if(rs.count != 0)
	{
		printf("error! rs.count must be 0! rs.count:%d\n", rs.count);
		return -1;
	}
	
	MemLinkStat st;
	ret = memlink_cmd_stat(m, buf, &st);
	if(ret != MEMLINK_OK)
	{
		printf("stat err key:%s, ret:%d\n", buf, ret);
		return -1;
	}

	if(st.data_used != 0)
	{
		printf("error data_used must be 0, st.data_used:%d\n", st.data_used);
		//return -1;
	}

	sprintf(buf, "hehe");
	ret = memlink_cmd_create_list(m, buf, 6, "4:3:2");
	if (ret != MEMLINK_OK) {
		printf("memlink_cmd_create %s error: %d\n", buf, ret);
		return -2;
	}
	
	char* maskstr1[] = {"8::1", "7:2:1", "6:2:1", "3:3:3"};
	char* valarray[] = {"111111", "222222", "333333", "444444"};
	int num = 100;
	for (i = 0; i < num; i++) {
		int k = i%4;
		ret = memlink_cmd_insert(m, buf, valarray[k], 6, maskstr1[k], i);
		if (ret != MEMLINK_OK) {
			printf("insert error, key:%s, value:%s, mask:%s, i:%d\n", buf, valarray[k], maskstr, i);
			return -3;
		}
	}
	printf("insert %d!\n", num);

	int spared = num;	
	for(i = 0; i < 4; i++)
	{
		int k = i % 4;
		int frompos = 0;
		int len = 100;

		ret = memlink_cmd_del_by_mask(m, buf, maskstr1[k]);	
		if (ret != MEMLINK_OK) {
			printf("del_by_mask key:%s, ret:%d\n", buf, ret);
			return -4;
		}
		
		ret = memlink_cmd_range(m, buf, MEMLINK_VALUE_VISIBLE,  maskstr1[k], frompos, len, &rs);
		if (ret != MEMLINK_OK)
		{
			printf("range error, key:%s, mask:%s\n", buf, maskstr1[k]);
			return -3;
		}
		MemLinkItem* item = rs.root;
		if(rs.count != 0)
		{
			printf("err! rs.count must be 0! rs.count:%d\n", rs.count);
			return -1;
		}

		MemLinkStat st;
		ret = memlink_cmd_stat(m, buf, &st);
		if(ret != MEMLINK_OK)
		{
			printf("stat err key:%s, ret:%d\n", buf, ret);
			return -1;
		}

		spared -= num/4;
		if(st.data_used != spared)
		{
			printf("st.data_used:%d\n", st.data_used);
			//return -1;
		}
		/*
		while(item != NULL)
		{
			if(strcmp(item->value, valarray[k]) == 0)
			{
				printf("no item->value:%s\n", item->value);
				return -1;
			}
		}*/
	}
    //goto memlink_over;
	
memlink_over:
	memlink_destroy(m);

	return 0;
}
