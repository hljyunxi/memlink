#include <stdio.h>
#include <stdlib.h>
#include "logfile.h"
#include "serial.h"
#include <math.h>
#include <utils.h>
#include "hashtest.h"

int mask_string2array_test()//随机生成一个mask，0-20个项，每项的值0-256
{
	int num = 1 + my_rand(HASHTABLE_MASK_MAX_ITEM);
	char mask[512] = {0};
	char buf[10];
	int i = 1;
	
    while(i <= num) {
		int val = my_rand(512);
		sprintf(buf, "%d", val);
		do {
			if(val > 256 && num != 1)
				break;			
			strcat(mask, buf);
		}while(0);

		if(i != num)
			strcat(mask, ":");
		i++;
	}
	unsigned int result[128];
	int ret = mask_string2array(mask, result);
	if(ret != num) {		
		DERROR("mask_string2array error. mask:%s, num:%d, ret:%d\n", mask, num, ret);
		return -1;
	}

	return 0;
}

int mask_string2binary_binary2string()
{
	int i = 0;
    int j = 0;

	for (j = 0; j < 50; j++) {
		int num = 1 + my_rand(5);
		char maskformat[512] = {0};
		unsigned char maskformatnum[100] = {0};
		char maskstr[512] = {0};

		//DINFO("num:%d\n", num);		
		int len = 2;

		for(i = 1; i <= num; i++) {
			//随机生成maskformat
			char buf[10] = {0};
			char buf2[10] = {0};
			int val = 1 + my_rand(8);

			sprintf(buf, "%d", val);
			strcat(maskformat, buf);
			if(i != num) {
				strcat(maskformat, ":");
            }
			len += val;
			maskformatnum[i-1] = (char)val;
			//随机生成maskstr
			int max = pow(2, val) - 1;
			int k= 1 + my_rand(max);
			sprintf(buf2, "%d", k);
			do {
				if(k > 1024)
					break;			
				strcat(maskstr, buf2);
			}while(0);

			if(i != num)
				strcat(maskstr, ":");
		}		
		//DINFO("maskformat=%s, maskstr=%s\n", maskformat, maskstr);
		int n = len / 8;
		int masklen = ((len % 8) == 0)? n : (n+1);
		char mask[512] = {0};
		int ret;
		ret = mask_string2binary(maskformatnum, maskstr, mask);		

		char buf2[128];
        //DINFO("mask:%s\n", formath(mask, masklen, buf2, 128));
		char maskstr1[512] = {0};
		mask_binary2string(maskformatnum, num, mask, masklen, maskstr1);		
		//DINFO("maskstr1=%s\n\n", maskstr1);		
		//DINFO("maskformat=%s, maskstr=%s, maskstr1=%s\n", maskformat, maskstr, maskstr1);

		if(0 != strcmp(maskstr, maskstr1)) {	
			DERROR("================ERROR!=============== ");
			DINFO("mask:%s\n", formath(mask, masklen, buf2, 128));
			return -1;
		}
	}
	return 0;
}


int array2bin_test_one(unsigned char *format, unsigned int *array, char num, unsigned char *mask, unsigned char msize)
{
	int ret;
    char data[1024] = {0};

    ret = mask_array2binary(format, array, num, data);
	if (ret != msize) {
		DERROR("mask_array2binary error. ret:%d, mask size:%d\n", ret, msize);
		return -1;
	}else{
		ret = memcmp(data, mask, msize);
		if (0 != ret) {
            char buf[512] = {0};
			DERROR("mask memcmp error. %s\n", formath(data, msize, buf, 512));
			return -1;
		}else{
			//DINFO("test1 ok!\n");
        }
	}

    return 0;
}

int array2flag_test_one(unsigned char *format, unsigned int *array, char num, unsigned char *mask, unsigned char msize)
{
	int ret;
    char data[1024] = {0};

    ret = mask_array2flag(format, array, num, data);
	if (ret != msize) {
		DERROR("mask_array2flag error. ret:%d, mask size:%d\n", ret, msize);
		return -1;
	}else{
		ret = memcmp(data, mask, msize);
		if (0 != ret) {
            char buf[512] = {0};
			DERROR("mask memcmp error. %s\n", formath(data, msize, buf, 512));
			return -1;
		}else{
			//DINFO("test1 ok!\n");
        }
	}

    return 0;
}

typedef struct testitem
{
    unsigned char format[16];
    unsigned int  array[16];
    int           num;
    unsigned char mask[512];
    int           msize;
}TestItem;

int mask_array2binary_test()
{
    TestItem    testitems[100];
    
    TestItem    *item = &testitems[0];
    item->num       = 3;
    item->format[0] = 4;
    item->format[1] = 3;
    item->format[2] = 1;
    item->array[0]  = UINT_MAX;
    item->array[1]  = 2;
    item->array[2]  = 0;
    item->mask[0]   = 0x81;
    item->mask[1]   = 0;
    item->msize     = 2;

    item = &testitems[1];
    item->num       = 3;
    item->format[0] = 4;
    item->format[1] = 3;
    item->format[2] = 1;
    item->array[0]  = 5;
    item->array[1]  = 2;
    item->array[2]  = 2;
    item->mask[0]   = 0x95;
    item->mask[1]   = 0;
    item->msize     = 2;

    item = &testitems[2];
    item->num       = 3;
    item->format[0] = 4;
    item->format[1] = 3;
    item->format[2] = 8;
    item->array[0]  = 5;
    item->array[1]  = 2;
    item->array[2]  = 128;
    item->mask[0]   = 0x95;
    item->mask[1]   = 0;
    item->mask[2]   = 0x01;
    item->msize     = 3;

    item = &testitems[3];
    item->num       = 2;
    item->format[0] = 32;
    item->format[1] = 1;
    item->array[0]  = 0x23;
    item->array[1]  = 0x01;
    item->mask[0]   = 0x8d;
    item->mask[1]   = 0;
    item->mask[2]   = 0;
    item->mask[3]   = 0;
    item->mask[4]   = 0x04;
    item->msize     = 5;

    item = &testitems[4];
    item->num       = 2;
    item->format[0] = 2;
    item->format[1] = 16;
    item->array[0]  = 1;
    item->array[1]  = 0x1111;
    item->mask[0]   = 0x15;
    item->mask[1]   = 0x11;
    item->mask[2]   = 0x01;
    item->msize     = 3;

    item = &testitems[5];
    item->num       = 2;
    item->format[0] = 32;
    item->format[1] = 32;
    item->array[0]  = 0x0fffffff;
    item->array[1]  = 0x10101010;
    item->mask[0]   = 0xfd;
    item->mask[1]   = 0xff;
    item->mask[2]   = 0xff;
    item->mask[3]   = 0x3f;
    item->mask[4]   = 0x40;
    item->mask[5]   = 0x40;
    item->mask[6]   = 0x40;
    item->mask[7]   = 0x40;
    item->mask[8]   = 0x0;
    item->msize     = 9;

    item = &testitems[6];
    item->num       = 2;
    item->format[0] = 32;
    item->format[1] = 32;
    item->array[0]  = 0xffffffff;
    item->array[1]  = 0x10101010;
    item->mask[0]   = 0x01;
    item->mask[1]   = 0x0;
    item->mask[2]   = 0x0;
    item->mask[3]   = 0x0;
    item->mask[4]   = 0x40;
    item->mask[5]   = 0x40;
    item->mask[6]   = 0x40;
    item->mask[7]   = 0x40;
    item->mask[8]   = 0x0;
    item->msize     = 9;

    item = &testitems[7];
    item->num       = 1;
    item->format[0] = 32;
    item->array[0]  = 0xffffffff;
    item->mask[0]   = 0x01;
    item->mask[1]   = 0x0;
    item->mask[2]   = 0x0;
    item->mask[3]   = 0x0;
    item->mask[4]   = 0x0;
    item->msize     = 5;


    item = &testitems[8];
    item->num       = 3;
    item->format[0] = 2;
    item->format[1] = 32;
    item->format[2] = 32;
    item->array[0]  = 1;
    item->array[1]  = 0x0fffffff;
    item->array[2]  = 0x10101010;
    item->mask[0]   = 0xf5;
    item->mask[1]   = 0xff;
    item->mask[2]   = 0xff;
    item->mask[3]   = 0xff;
    item->mask[4]   = 0x0;
    item->mask[5]   = 0x01;
    item->mask[6]   = 0x01;
    item->mask[7]   = 0x01;
    item->mask[8]   = 0x01;
    item->msize     = 9;

    int i;
    for (i = 0; i < 9; i++) {
        item = &testitems[i];
        int ret = array2bin_test_one(item->format, item->array, item->num, 
                item->mask, item->msize);
        if (ret != 0) {
            DERROR("test error: %d, %d\n", i, ret);
        }
    }

	return 0;
}

int mask_array2flag_test()
{
    TestItem    testitems[100];
    
    TestItem    *item = &testitems[0];
    item->num       = 3;
    item->format[0] = 4;
    item->format[1] = 3;
    item->format[2] = 1;
    item->array[0]  = UINT_MAX;
    item->array[1]  = 2;
    item->array[2]  = 0;
    item->mask[0]   = 0x3f;
    item->mask[1]   = 0;
    item->msize     = 2;

    item = &testitems[1];
    item->num       = 3;
    item->format[0] = 4;
    item->format[1] = 3;
    item->format[2] = 1;
    item->array[0]  = 5;
    item->array[1]  = 2;
    item->array[2]  = 2;
    item->mask[0]   = 0x03;
    item->mask[1]   = 0;
    item->msize     = 2;

    item = &testitems[2];
    item->num       = 3;
    item->format[0] = 4;
    item->format[1] = 3;
    item->format[2] = 8;
    item->array[0]  = 5;
    item->array[1]  = 2;
    item->array[2]  = 128;
    item->mask[0]   = 0x03;
    item->mask[1]   = 0;
    item->mask[2]   = 0;
    item->msize     = 3;

    item = &testitems[3];
    item->num       = 2;
    item->format[0] = 32;
    item->format[1] = 1;
    item->array[0]  = 0x23;
    item->array[1]  = 0x01;
    item->mask[0]   = 0x03;
    item->mask[1]   = 0;
    item->mask[2]   = 0;
    item->mask[3]   = 0;
    item->mask[4]   = 0;
    item->msize     = 5;

    item = &testitems[4];
    item->num       = 2;
    item->format[0] = 2;
    item->format[1] = 16;
    item->array[0]  = 1;
    item->array[1]  = 0x1111;
    item->mask[0]   = 0x03;
    item->mask[1]   = 0;
    item->mask[2]   = 0;
    item->msize     = 3;

    item = &testitems[5];
    item->num       = 2;
    item->format[0] = 32;
    item->format[1] = 32;
    item->array[0]  = 0x0fffffff;
    item->array[1]  = 0x10101010;
    item->mask[0]   = 0x03;
    item->mask[1]   = 0;
    item->mask[2]   = 0;
    item->mask[3]   = 0;
    item->mask[4]   = 0;
    item->mask[5]   = 0;
    item->mask[6]   = 0;
    item->mask[7]   = 0;
    item->mask[8]   = 0;
    item->msize     = 9;

    item = &testitems[6];
    item->num       = 2;
    item->format[0] = 32;
    item->format[1] = 32;
    item->array[0]  = 0xffffffff;
    item->array[1]  = 0x10101010;
    item->mask[0]   = 0xff;
    item->mask[1]   = 0xff;
    item->mask[2]   = 0xff;
    item->mask[3]   = 0xff;
    item->mask[4]   = 0x03;
    item->mask[5]   = 0;
    item->mask[6]   = 0;
    item->mask[7]   = 0;
    item->mask[8]   = 0;
    item->msize     = 9;

    item = &testitems[7];
    item->num       = 1;
    item->format[0] = 32;
    item->array[0]  = 0xffffffff;
    item->mask[0]   = 0xff;
    item->mask[1]   = 0xff;
    item->mask[2]   = 0xff;
    item->mask[3]   = 0xff;
    item->mask[4]   = 0x03;
    item->msize     = 5;


    item = &testitems[8];
    item->num       = 3;
    item->format[0] = 2;
    item->format[1] = 32;
    item->format[2] = 32;
    item->array[0]  = 1;
    item->array[1]  = 0x0fffffff;
    item->array[2]  = 0x10101010;
    item->mask[0]   = 0x03;
    item->mask[1]   = 0;
    item->mask[2]   = 0;
    item->mask[3]   = 0;
    item->mask[4]   = 0;
    item->mask[5]   = 0;
    item->mask[6]   = 0;
    item->mask[7]   = 0;
    item->mask[8]   = 0;
    item->msize     = 9;

    int i;
    for (i = 0; i < 9; i++) {
        item = &testitems[i];
        int ret = array2flag_test_one(item->format, item->array, item->num, 
                item->mask, item->msize);
        if (ret != 0) {
            DERROR("test error: %d, %d\n", i, ret);
        }
    }

	return 0;
}

int main()
{
	logfile_create("test.log", 3);
	//logfile_create("stdout", 4);

	int ret;
	int i = 0;

	for (i = 0; i < 10; i++) {
		ret = mask_string2array_test();
		if (0 != ret)
			return -1;
	}
	
	ret = mask_array2binary_test();
	if (0 != ret) {
		return -1;
    }

	ret = mask_array2flag_test();
	if (0 != ret) {
		return -1;
    }
	mask_string2binary_binary2string();
	if (0 != ret) {
		return -1;
    }

	return 0;
}
