#include <stdio.h>
#include <stdlib.h>
#include <memlink_client.h>
#include "logfile.h"
#include "test.h"

// 异常参数测试
int check_exception_parameter(MemLink *m)
{
    int  ret;
    char key[512] = {0};
    char val[512] = {0};

    sprintf(key, "haha1");
	ret = memlink_cmd_create_list(m, key, 256, "4:3:1");
	if (ret == MEMLINK_OK) {
		DERROR("memlink_cmd_create error, ret:%d key:%s\n", ret, key);
		return -2;
	}
    ret = memlink_cmd_create_list(m, key, -1, "4:3:1");
	if (ret == MEMLINK_OK) {
		DERROR("memlink_cmd_create error, ret:%d key:%s\n", ret, key);
		return -2;
	}
    ret = memlink_cmd_create_list(m, key, 0, "4:3:1");
	if (ret == MEMLINK_OK) {
		DERROR("memlink_cmd_create error, ret:%d key:%s\n", ret, key);
		return -2;
	}
    ret = memlink_cmd_create_list(m, key, 4, "");
	if (ret != MEMLINK_OK) {
		DERROR("memlink_cmd_create error, ret:%d key:%s\n", ret, key);
		return -2;
	}
    ret = memlink_cmd_rmkey(m, key);
    if (ret != MEMLINK_OK) {
        DERROR("rmkey error, ret:%d, key:%s\n", ret, key);
        return -1;
    }
    ret = memlink_cmd_create_list(m, key, 6, "4:3:1");
	if (ret != MEMLINK_OK) {
		DERROR("memlink_cmd_create error, ret:%d key:%s\n", ret, key);
		return -2;
	}


    sprintf(val, "%06d", 2);
    ret = memlink_cmd_insert(m, key, val, strlen(val), "14294967295:3:1", 0); 
	if (ret == MEMLINK_OK) {
		DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", key, val, "14294967295:3:1", 0);
		return -3;
	}

    char *maskstr = "7:2:1";
	ret = memlink_cmd_insert(m, key, val, -1, maskstr, -10); 
	if (ret == MEMLINK_OK) {
		DERROR("insert error, key:%s, val:%s, mask:%s, pos:%d\n", key, val, maskstr, -10);
		return -3;
	}
	
	strcpy(val, "0610");
	ret = memlink_cmd_insert(m, key, val, strlen(val), "4:3:2:1", 0); 
	if (ret == MEMLINK_OK) {
		DERROR("insert error, key:%s, val:%s, mask=4:3:2:1, ret:%d\n", key, val, ret);
		return -3;
	}

	strcpy(val, "0610");
	ret = memlink_cmd_insert(m, key, val, strlen(val), "2:1", 0); 
	if (ret == MEMLINK_OK) {
		DERROR("insert error, key:%s, val:%s, mask=2:1, ret:%d\n", key, val, ret);
		return -3;
	}

    // mask截断
    ret = memlink_cmd_insert(m, key, val, strlen(val), "20:9:4", 0); 
	if (ret != MEMLINK_OK) {
		DERROR("insert error, key:%s, val:%s, mask=20:9:4, ret:%d\n", key, val, ret);
		return -3;
	}
    
    MemLinkResult result;
    ret = memlink_cmd_range(m, key, MEMLINK_VALUE_ALL, "", 0, 1, &result);
    if (ret != MEMLINK_OK) {
        DERROR("range error, ret:%d\n", ret);
        return -1;
    }
    if (1 != result.count) {
        DERROR("result count error: %d\n", result.count);
        return -1;
    }
    MemLinkItem *item = result.root; 
    if (strcmp(item->mask, "4:1:0") != 0) {
        DERROR("mask error: %s\n", item->mask);
        return -1;
    }

    return 0;
}

#define check_result(m,k,mask,f,l,c,v)  check_result_real(m,k,mask,f,l,c,v,__FILE__,__LINE__)

int check_result_real(MemLink *m, char *key, char **maskstrs, 
                int from, int len, int retcount, int *retval, char *file, int line)
{
    MemLinkResult result;
    int ret = memlink_cmd_range(m, key, MEMLINK_VALUE_ALL, "", from, len, &result);
    if (ret != MEMLINK_OK) {
        DERROR("range error, ret:%d, key:%s, from:%d, len:%d, file:%s:%d\n", 
                    ret, key, from, len, file, line);
        return -1;
    }
    if (result.count != retcount) {
        DERROR("result count error:%d, retcount:%d, file:%s:%d\n", result.count, retcount, file, line);
        return -1;
    }
    MemLinkItem *item = result.root;
    int  i = 0;
    char val[512] = {0};

    while (item) {
        sprintf(val, "%06d", retval[i]);
        if (strcmp(val, item->value) != 0) {
            DERROR("value error, item->value:%s, check value:%s, i:%d, file:%s:%d\n", 
                        item->value, val, i, file, line);
            return -1;
        }
        if (strcmp(maskstrs[retval[i]%3], item->mask) != 0) {
            DERROR("mask error, item->mask:%s, mask:%s, i:%d, file:%s:%d\n", 
                        item->mask, maskstrs[i%3], i, file, line);
            return -1;
        }
        i++;
        item = item->next;
    }
    memlink_result_free(&result);

    return 0;
}


int check_insert(MemLink *m)
{
    int  ret;
    char key[512] = {0};
    char val[512] = {0};
    char *maskstrs[]    = {"8::1", "7:2:1", "6:2:1"}; // parameter
    char *maskstrs_in[] = {"8:0:1", "7:2:1", "6:2:1"};  // in memlink list mask
    
    sprintf(key, "haha2");
    ret = memlink_cmd_create(m, key, 6, "4:3:1", MEMLINK_LIST, MEMLINK_VALUE_STRING);
	if (ret != MEMLINK_OK) {
		DERROR("memlink_cmd_create error, ret:%d key:%s\n", ret, key);
		return -1;
	}

    int valnum = 0;
    int i;

    for (i = 0; i < 10; i++) {
        sprintf(val, "%06d", i);
        ret = memlink_cmd_insert(m, key, val, strlen(val), maskstrs[i%3], 0);	
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", key, val, maskstrs[i%3], i);
			return -1;
		}
        
        MemLinkStat stat; 
        ret = memlink_cmd_stat(m, key, &stat);
        if (ret != MEMLINK_OK) {
            DERROR("stat error, ret:%d key:%s\n", ret, key);
            return -1;
        }
        if (stat.blocks != 1) {
            DERROR("stat blocks error:%d\n", stat.blocks);
            return -1;
        }

        if (stat.data_used != i + 1) {
            DERROR("stat data_used error:%d\n", stat.data_used);
            return -1;
        }
        
        if (i == 0) {
            if (stat.data != 1) {
                DERROR("stat data error:%d i:%d\n", stat.data, i);
                return -1;
            }
        }else if (i == 1) {
            if (stat.data != 2) {
                DERROR("stat data error:%d i:%d\n", stat.data, i);
                return -1;
            }
        }else if (i > 1 && i < 5) {
            if (stat.data != 5) {
                DERROR("stat data error:%d i:%d\n", stat.data, i);
                return -1;
            }
        }else{
            if (stat.data != 10) {
                DERROR("stat data error:%d i:%d\n", stat.data, i);
                return -1;
            }
        }
    }
    valnum += 10; 

    int retval[1000] = {0};
    int k = 0;
    for (i = valnum - 1; i >= 0; i--) {
        retval[k] = i;
        k++;
    }
    ret = check_result(m, key, maskstrs_in, 0, 20, valnum, retval);
    if (ret < 0)
        return ret;

    for (i = 10; i < 50; i++) {
        sprintf(val, "%06d", i);
        ret = memlink_cmd_insert(m, key, val, strlen(val), maskstrs[i % 3], 0);	
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", key, val, maskstrs[i % 3], i);
			return -1;
		}
    } 
    valnum += 40;

    k = 0;
    for (i = valnum - 1; i >= 0; i--) {
        retval[k] = i;
        k++;
    }
    ret = check_result(m, key, maskstrs_in, 0, 100, valnum, retval);
    if (ret < 0)
        return ret;
    
    i = 45;
    sprintf(val, "%06d", i);
    ret = memlink_cmd_del(m, key, val, strlen(val));
    if (ret != MEMLINK_OK) {
        DERROR("del error, key:%s, val:%s\n", key, val);
        return -1;
    }
    valnum--;
    
    int pos = 4;
    retval[0] = i - 1;
    ret = check_result(m, key, maskstrs_in, pos, 1, 1, retval);
    if (ret < 0)
        return ret;

    // 在空位置插入
    i = 100;
    sprintf(val, "%06d", i);
    ret = memlink_cmd_insert(m, key, val, strlen(val), maskstrs[i%3], pos);
    if (ret != MEMLINK_OK) {
        DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", key, val, maskstrs[i%3], i);
        return -1;
    }
    valnum++;

    retval[0] = i;
    ret = check_result(m, key, maskstrs_in, pos, 1, 1, retval);
    if (ret < 0)
        return ret;
   
    // block有空，但当前位置不为空
    sprintf(val, "%06d", 100);
    ret = memlink_cmd_del(m, key, val, strlen(val));  
    if (ret != MEMLINK_OK) {
        DERROR("del error, ret:%d, val:%s\n", ret, val);
        return -1;
    }
    valnum--;
    sprintf(val, "%06d", 49);
    ret = memlink_cmd_del(m, key, val, strlen(val));  
    if (ret != MEMLINK_OK) {
        DERROR("del error, ret:%d, val:%s\n", ret, val);
        return -1;
    }
    valnum--;
    sprintf(val, "%06d", 47);
    ret = memlink_cmd_del(m, key, val, strlen(val));  
    if (ret != MEMLINK_OK) {
        DERROR("del error, ret:%d, val:%s\n", ret, val);
        return -1;
    }
    valnum--;

    pos = 3;
    i = 200;
    sprintf(val, "%06d", i);
    ret = memlink_cmd_insert(m, key, val, strlen(val), maskstrs[i%3], pos);
    if (ret != MEMLINK_OK) {
        DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", key, val, maskstrs[i%3], i);
        return -1;
    }
    valnum++;
    
    retval[0] = 48;
    retval[1] = 46;
    retval[2] = 44;
    retval[3] = 200;
    k = 4; 
    for (i = 43; i >= 0; i--) {
        retval[k] = i;
        k++;
    }
    ret = check_result(m, key, maskstrs_in, 0, 100, valnum, retval);
    if (ret < 0)
        return ret;
   
    // 在满的block中插入，此block的下一个block的第一个位置是空
    sprintf(val, "%06d", 29);
    ret = memlink_cmd_del(m, key, val, strlen(val));  
    if (ret != MEMLINK_OK) {
        DERROR("del error, ret:%d, val:%s\n", ret, val);
        return -1;
    }
    valnum--;

    pos = 10;
    i = 300;
    sprintf(val, "%06d", i);
    ret = memlink_cmd_insert(m, key, val, strlen(val), maskstrs[i%3], pos);
    if (ret != MEMLINK_OK) {
        DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", key, val, maskstrs[i%3], i);
        return -1;
    }
    valnum++;
    
    retval[0] = 39;
    retval[1] = 38;
    retval[2] = 300;
    k = 3; 
    for (i = 37; i >= 30; i--) {
        retval[k] = i;
        k++;
    }
    for (i = 28; i >= 0; i--) {
        retval[k] = i;
        k++;
    }
    ret = check_result(m, key, maskstrs_in, 8, 100, valnum - 8, retval);
    if (ret < 0)
        return ret;
 
    // 在满的block中插入，此block的下一个block有空，但不是第一个位置
    sprintf(val, "%06d", 28);
    ret = memlink_cmd_del(m, key, val, strlen(val));  
    if (ret != MEMLINK_OK) {
        DERROR("del error, ret:%d, val:%s\n", ret, val);
        return -1;
    }
    valnum--;

    pos = 10;
    i = 400;
    sprintf(val, "%06d", i);
    ret = memlink_cmd_insert(m, key, val, strlen(val), maskstrs[i%3], pos);
    if (ret != MEMLINK_OK) {
        DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", key, val, maskstrs[i%3], i);
        return -1;
    }
    valnum++;
    
    retval[0] = 39;
    retval[1] = 38;
    retval[2] = 400;
    retval[3] = 300;
    k = 4; 
    for (i = 37; i >= 30; i--) {
        retval[k] = i;
        k++;
    }
    for (i = 27; i >= 0; i--) {
        retval[k] = i;
        k++;
    }
    ret = check_result(m, key, maskstrs_in, 8, 100, valnum - 8, retval);
    if (ret < 0)
        return ret;
 
    // 在满的block中插入，此block的下一个block也是满的
    pos = 10;
    i = 500;
    sprintf(val, "%06d", i);
    ret = memlink_cmd_insert(m, key, val, strlen(val), maskstrs[i%3], pos);
    if (ret != MEMLINK_OK) {
        DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", key, val, maskstrs[i%3], i);
        return -1;
    }
    valnum++;
 
    retval[0] = 39;
    retval[1] = 38;
    retval[2] = 500;
    retval[3] = 400;
    retval[4] = 300;
    k = 5; 
    for (i = 37; i >= 30; i--) {
        retval[k] = i;
        k++;
    }
    for (i = 27; i >= 0; i--) {
        retval[k] = i;
        k++;
    }
    ret = check_result(m, key, maskstrs_in, 8, 100, valnum - 8, retval);
    if (ret < 0)
        return ret;

    // 在满的block中插入。此满的block可以增大 
    pos = 18;
    i = 100;
    sprintf(val, "%06d", i);
    ret = memlink_cmd_insert(m, key, val, strlen(val), maskstrs[i%3], pos);
    if (ret != MEMLINK_OK) {
        DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", key, val, maskstrs[i%3], i);
        return -1;
    }
    valnum++;
 
    retval[0] = 100;
    retval[1] = 32;
    retval[2] = 31;
    retval[3] = 30;
    k = 4; 
    for (i = 27; i >= 0; i--) {
        retval[k] = i;
        k++;
    }
    ret = check_result(m, key, maskstrs_in, pos, 100, valnum - pos, retval);
    if (ret < 0)
        return ret;

    // 在满的block中插入。下个block也是满的，但下个block可以增大 
    pos = 16;
    i = 600;
    sprintf(val, "%06d", i);
    ret = memlink_cmd_insert(m, key, val, strlen(val), maskstrs[i%3], pos);
    if (ret != MEMLINK_OK) {
        DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", key, val, maskstrs[i%3], i);
        return -1;
    }
    valnum++;
 
    retval[0] = 600;
    retval[1] = 34;
    retval[2] = 33;
    retval[3] = 100;
    retval[4] = 32;
    retval[5] = 31;
    retval[6] = 30;
    k = 7; 
    for (i = 27; i >= 0; i--) {
        retval[k] = i;
        k++;
    }
    ret = check_result(m, key, maskstrs_in, pos, 100, valnum - pos, retval);
    if (ret < 0)
        return ret;

    // 尾部插入
    pos = valnum;
    k = 0;
    for (i = 700; i < 720; i++) {
        sprintf(val, "%06d", i);
        ret = memlink_cmd_insert(m, key, val, strlen(val), maskstrs[i%3], valnum);
        if (ret != MEMLINK_OK) {
            DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", key, val, maskstrs[i%3], i);
            return -1;
        }
        retval[k] = i;
        k++;
        valnum++;
    }
    ret = check_result(m, key, maskstrs_in, pos, 100, valnum - pos, retval);
    if (ret < 0)
        return ret;

    // 尾部插入, 位置为负数
    pos = valnum;
    k = 0;
    for (i = 800; i < 820; i++) {
        sprintf(val, "%06d", i);
        ret = memlink_cmd_insert(m, key, val, strlen(val), maskstrs[i%3], -1);
        if (ret != MEMLINK_OK) {
            DERROR("insert error, key:%s, val:%s, mask:%s, i:%d\n", key, val, maskstrs[i%3], i);
            return -1;
        }
        retval[k] = i;
        k++;
        valnum++;
    }
    ret = check_result(m, key, maskstrs_in, pos, 100, valnum - pos, retval);
    if (ret < 0)
        return ret;




    return 0;
}


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
    int ret;
    
    ret = check_exception_parameter(m);
    if (ret < 0) {
        return ret;
    }

    ret = check_insert(m);
    if (ret < 0) {
        return ret;
    }

	memlink_destroy(m);

	return 0;
}
