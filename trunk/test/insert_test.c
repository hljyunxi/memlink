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

	int  i;
	char val[64];
	char* maskstr1[] = {"8::1", "7:2:1", "6:2:1"};
	int	 insertnum = 200;
	for (i = 0; i < insertnum; i++) {
		sprintf(val, "%06d", i);
		int k = i%3;
		ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr1[k], 0);	
		//ret = memlink_cmd_insert(m, buf, val, strlen(val), "14294967295:3:1", 0);	
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", buf, val, maskstr1[k], i);
			return -3;
		}
	}

	ret = memlink_cmd_insert(m, buf, val, strlen(val), "14294967295:3:1", 0); 
	if (ret == MEMLINK_OK) {
		DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", buf, val, "14294967295:3:1", 0);
		return -3;
	}

	ret = memlink_cmd_insert(m, buf, val, -1, maskstr1[1], -10); 
	if (ret == MEMLINK_OK) {
		DERROR("insert error, key:%s, val:%s, mask:%s, pos:%d\n", buf, val, maskstr1[1], -10);
		return -3;
	}
	//ret = memlink_cmd_insert(m, buf, val, strlen(val), "8:3:1", 201); 
	//DINFO("ret:%d val: %s\n", ret, val);
	/*for(i = 98; i <= 99; i++)
	{
		sprintf(val, "%06d", i);
		ret = memlink_cmd_del(m,buf,val,strlen(val));
		if(ret != MEMLINK_OK)
		{
			DERROR("stat error, key:%s, ret:%d\n", buf, ret);
			return -4;
		}
	}

	i = 98;
	sprintf(val, "%06d", i);
	ret = memlink_cmd_insert(m, buf, val, strlen(val), "7:2:1", i); 
	if (ret != MEMLINK_OK) {
		DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", buf, val, maskstr1[1], i);
		return -3;
	}*/

	///*****************************************///
	MemLinkStat	stat;
	ret = memlink_cmd_stat(m, buf, &stat);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s, ret:%d\n", buf, ret);
		return -4;
	}
	//DINFO("stat blocks:%d data:%d, data_used:%d\n", stat.blocks, stat.data, stat.data_used);

	if (stat.data_used != insertnum) {
		DERROR("insert num error, data_used:%d\n", stat.data_used);
		return -5;
	}	
	if (stat.data != insertnum) { 
		DERROR("insert num error, data:%d\n", stat.data);
		return -5;
	}

	MemLinkResult   result;
	ret = memlink_cmd_range(m, buf, 4,  "::", 0, insertnum, &result);
	if (ret != MEMLINK_OK) {
		DERROR("range error, ret:%d\n", ret);
		return -6;
	}
	MemLinkItem	*item = result.root;
	if (NULL == item) {
		DERROR("range must not null\n");
		return -7;
	}
	i = insertnum;
	while (item) {
		i--;
		sprintf(val, "%06d", i);
		//DERROR("mask=%s\n", item->mask);
		if (strcmp(item->value, val) != 0) {
			DERROR("range value error, value:%s\n", val);
			return -8;
		}
		item = item->next;
	}
	memlink_result_free(&result);

	/*
//É¾³ıµÚÒ»¸öÌõÄ¿£¬È»ºóÔÚpos=1´¦²åÈë1¸ö,¼ì²éÊÇ·ñ²åÈëÕıÈ·
	i = 199;
	sprintf(val, "%06d", i);
	ret = memlink_cmd_del(m, buf, val, strlen(val));
	
	ret = memlink_cmd_insert(m, buf, val, strlen(val), "7:2:1", 1); 
	if (ret != MEMLINK_OK) {
		DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", buf, val, "7:2:1", i);
		return -3;
	}

	ret = memlink_cmd_range(m, buf, 4,  "", 0, 2, &result);
	if (ret != MEMLINK_OK) {
		DERROR("range error, ret:%d\n", ret);
		return -6;
	}
	item = result.root;
	if (NULL == item) {
		DERROR("range must not null\n");
		return -7;
	}

	i = 198;
	sprintf(val, "%06d", i);
	//DERROR("mask=%s\n", item->mask);
	if (strcmp(item->value, val) != 0) {
		DERROR("range value error, value:%s, item->value:%s\n", val, item->value);
		//return -8;
	}
	item = item->next;

	i = 199;
	sprintf(val, "%06d", i);
	//DERROR("mask=%s\n", item->mask);
	if (strcmp(item->value, val) != 0) {
		DERROR("range value error, value:%s, item->value:%s\n", val, item->value);
		return -8;
	}
	item = item->next;
	
	memlink_result_free(&result);

	DERROR("11111111111111111111\n");
	*/
////////////////////////////
	//å†ä¸­é—´æ’å…¥ä¸€ä¸ª
	char* maskstr = "8:3:1";
	sprintf(val, "%06d", 300);

	ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, 100);	
	if (ret != MEMLINK_OK) {
		DERROR("insert error, ret:%d\n", ret);
		return -9;
	}

	MemLinkStat	stat1;
	ret = memlink_cmd_stat(m, buf, &stat1);
	if (ret != MEMLINK_OK) {
		DERROR("stat1 error, key:%s\n", buf);
		return -4;
	}

	//DINFO("stat.data:%d, dataused:%d, block:%d\n", stat1.data, stat1.data_used, stat1.blocks);
	if (stat1.data != insertnum + 1) {
		DERROR("insert num error, data:%d\n", stat1.data);
		return -5;
	}

	MemLinkResult   result2;
	//range 201 ä¸ª
	ret = memlink_cmd_range(m, buf, 4,  "::", 0, insertnum + 1, &result2);
	if (ret != MEMLINK_OK) {
		DERROR("range error, ret:%d\n", ret);
		return -6;
	}

	item = result2.root;

	if (NULL == item) {
		DERROR("range must not null\n");
		return -7;
	}

	i = 200;
	while (item) {
		i--;
		if (i == 99) {
			sprintf(val, "%06d", 300);
			if (strcmp(item->value, val) != 0) {
				DERROR("range value error, item->value:%s, value:%s\n", item->value, val);
				return -8;
			}
			item = item->next;
		}

		sprintf(val, "%06d", i);
		if (strcmp(item->value, val) != 0) {
			DERROR("range value error, value:%s---------\n", val);
			return -8;
		}
		item = item->next;
	}
	
	memlink_result_free(&result2);
	//åˆ é™¤200ä¸ª
	for (i = 0; i < insertnum; i++) {
		sprintf(val, "%06d", i);
		ret = memlink_cmd_del(m, buf, val, strlen(val));
		if (ret != MEMLINK_OK) {
			DERROR("del error, val:%s, ret:%d\n", val, ret);
			return -9;
		}
	}

	// =======================================================
	//æ’å…¥200ä¸ª æ’å…¥æ—¶çš„posåªæ˜¯ç›¸å¯¹äºéæ ‡è®°åˆ é™¤çš„æ¡ç›®
	for (i = 0; i < insertnum; i++) {
		sprintf(val, "%06d", i);
		ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, i*2+1);	
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", buf, val, maskstr, i);
			return -3;
		}
	}

	ret = memlink_cmd_stat(m, buf, &stat);
	if (ret != MEMLINK_OK) {
		DERROR("stat error, key:%s\n", buf);
		return -4;
	}
	
	if (stat.data_used != insertnum + 1) {
		DERROR("insert num error, data_used:%d\n", stat.data_used);
		return -5;
	}


	MemLinkResult   result3;

	ret = memlink_cmd_range(m, buf, MEMLINK_VALUE_VISIBLE,  "::", 0, insertnum + 1, &result3);
	if (ret != MEMLINK_OK) {
		DERROR("range error, ret:%d\n", ret);
		return -6;
	}
	//DINFO("result3 count: %d\n", result3.count);
	item = result3.root;

	if (NULL == item) {
		DERROR("range must not null\n");
		return -7;
	}

	sprintf(val, "%06d", 300);
	if (strcmp(item->value, val) != 0) {
		DERROR("first value error, item->value:%s, val:%s\n", item->value, val);
		//return -8;
	}
	item = item->next;

	i = 0;
	while (item) {
		//if(i == 0)
			//sprintf(val, "%06d", 300);
		//else
			sprintf(val, "%06d", i);
		if (strcmp(item->value, val) != 0) {
			DERROR("range value error, item->value:%s, value:%s\n", item->value, val);
			//return -8;
		}
		i++;
		item = item->next;
	}

	//maskå€¼è·Ÿformatä¸ä¸€è‡´çš„æƒ…å†µ
	strcpy(val, "0610");
	//DERROR("val:%s, mask = 4:3:2:1\n", val);
	ret = memlink_cmd_insert(m, buf, val, strlen(val), "4:3:2:1", 0); 
	if (ret == MEMLINK_OK) {
		DERROR("insert error, key:%s, val:%s, mask=4:3:2:1, ret:%d\n", buf, val, ret);
		return -3;
	}
	
	memlink_result_free(&result3);

	/* test: value is not a string, ex: int/struct */
	sprintf(buf, "hehe");
	ret = memlink_cmd_create(m, buf, 4, "4:3:1");
	if (ret != MEMLINK_OK) {
		DERROR("1 memlink_cmd_create %s error: %d\n", buf, ret);
		return -2;
	}
	
	insertnum = 200;
	for (i = 0; i < insertnum; i++) {
		int k = i%3;
		ret = memlink_cmd_insert(m, buf, (char*)&i, sizeof(int), maskstr1[k], 0);	
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s, val:%d, mask:%s\n", buf, i, maskstr1[k]);
			return -3;
		}
	}
	
	MemLinkResult rs;
	ret = memlink_cmd_range(m, buf, MEMLINK_VALUE_VISIBLE,  "::", 0, insertnum, &rs);
	if (ret != MEMLINK_OK) {
		DERROR("range error, ret:%d\n", ret);
		return -6;
	}
	item = rs.root;
	if (NULL == item) {
		DERROR("range must not null\n");
		return -7;
	}
	i = 200;
	while (item) {
		i--;
		if (memcmp(item->value, &i, 4) != 0) {
			DERROR( "range value error, value:%d\n", *((int*)item->value) );
			return -8;
		}
		//DERROR( "item->value:%d, %d\n", *((int*)item->value), i);
		item = item->next;
	}
	
	memlink_result_free(&rs);

	memlink_destroy(m);

	return 0;
}
