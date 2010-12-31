#include <stdio.h>
#include <stdlib.h>
//#include <memlink_client.h>
#include "logfile.h"
#include "serial.h"
#include <math.h>
#include <utils.h>
int my_rand(int base)
{
	int i = -1;
	usleep(10);
	srand((unsigned)time(NULL)+rand());//在种子里再加一个随机数
	
	while (i < 0 )
	{
		i = rand()%base;
	}
	return i;
} 

int mask_string2array_test()//随机生成一个mask，0-20个项，每项的值0-256
{
	int num = 1 + my_rand(20);
	//num = 1;
	char mask[512] = {0};
	char buf[10];
	int i = 1;
	//for(i = 0; i < num; i++)
	//printf("num:%d\n", num);
	while(i <= num)
	{
		int val = my_rand(512);
		sprintf(buf, "%d", val);
		do
		{
			if(val > 256)
				break;			
			strcat(mask, buf);
		}while(0);
		if(i != num)
			strcat(mask, ":");
		i++;
	}
	//printf("mask=%s\n", mask);
	int result[128];
	int ret = mask_string2array(mask, result);
	if(ret != num)
	{		
		DERROR("mask_string2array error. num:%d, ret:%d\n", num, ret);
		return -1;
	}

	//DERROR("mask_string2array error. num:%d, ret:%d\n", num, ret);
	return 0;
}

int mask_string2binary_binary2string()
{
	int i = 0;int j = 0;
	for (j = 0; j < 50; j++)
	{
		int num = 1 + my_rand(3);
		num = 3;
		char maskformat[512] = {0};
		char maskformatnum[100] = {0};
		char maskstr[512] = {0};
		DINFO("num:%d\n", num);		
		int len = 2;
		for(i = 1; i <= num; i++)
		{
			//随机生成maskformat
			char buf[10] = {0};
			char buf2[10] = {0};
			int val = 1 + my_rand(4);
			sprintf(buf, "%d", val);
			strcat(maskformat, buf);
			if(i != num)
				strcat(maskformat, ":");
			len += val;
			maskformatnum[i-1] = (char)val;
			//随机生成maskstr
			int max = pow(2, val) - 1;
			int k= 1 + my_rand(max);
			sprintf(buf2, "%d", k);
			do
			{
				if(k > 32)
					break;			
				strcat(maskstr, buf2);
			}while(0);
			if(i != num)
				strcat(maskstr, ":");
		}		
		DINFO("maskformat=%s, maskstr=%s\n", maskformat, maskstr);
		int n = len / 8;
		int masklen = ((len % 8) == 0)? n : (n+1);
		char mask[512];
		int ret = mask_string2binary(maskformatnum, maskstr, mask);		
		char buf1[128], buf2[128];
        DINFO("mask:%s\n", formath(mask, masklen, buf2, 128));
		char maskstr1[512] = {0};
		mask_binary2string(maskformatnum, num, mask, masklen, maskstr1);		
		DINFO("maskstr1=%s\n\n", maskstr1);
	}
}

int mask_array2binary_test()
{
	int ret;
    unsigned char format1[3] = {4, 3, 1};
    unsigned int  maskarray1[3] = {UINT_MAX, 2, 0};
	//00 10000001
	char mask1[2] = {0x81, 0};
    unsigned int  masknum1 = 3;
    char data[1024] = {0};
    ret = mask_array2binary(format1, maskarray1, masknum1, data);
	if(ret != 2)
	{
		DERROR("mask_array2binary error. ret:%d\n", ret);
		return -1;
	}
	else
	{
		ret = memcmp(data, mask1, 2);
		if(0 != ret)
		{
			DERROR("mask1 memcmp error. \n");
			return -1;
		}
		else
			printf("test1 ok!\n");
	}
    unsigned char format2[3] = {4, 3, 1};
    unsigned int  maskarray2[3] = {5, 2, 2};
	//00 10010101
	char mask2[2] = {0x95, 0}; 
    unsigned int  masknum2 = 3;
	memset(data, 0, 1024);	
    ret = mask_array2binary(format2, maskarray2, masknum2, data);
	if(ret != 2)
	{
		DERROR("mask_array2binary error. ret:%d\n", ret);
		return -1;
	}
	else
	{
		ret = memcmp(data, mask2, 2);
		if(0 != ret)
		{
			DERROR("mask2 memcmp error. \n");
			return -1;
		}
		else
			printf("test2 ok!\n");
	}

    unsigned char format3[3] = {4, 3, 8};
    unsigned int  maskarray3[3] = {5, 2, 128};
	// 1 00000000 10010101
	char mask3[3] = {0x95, 0, 0x01}; 
    unsigned int  masknum3 = 3;
	memset(data, 0, 1024);	
    ret = mask_array2binary(format3, maskarray3, masknum3, data);
	if(ret != 3)
	{
		DERROR("mask_array2binary error. ret:%d\n", ret);
		return -1;
	}
	else
	{
		ret = memcmp(data, mask3, 3);
		if(0 != ret)
		{
			DERROR("mask3 memcmp error. \n");
			return -1;
		}
		else
			printf("test3 ok!\n");
	}

    unsigned char format4[2] = {127, 1};
    unsigned int  maskarray4[2] = {0x7f, 0x01};
	// 0 00000001 1111 1101
	char mask4[17] = {0xfd, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x02}; 
    unsigned int  masknum4 = 2;
	/*memset(data, 0, 1024);	
	int val = 0x03fd;
	memcpy(data, &val, 4);
	printb(data, 17);
	printf("==========================\n");*/
	memset(data, 0, 1024);	
    ret = mask_array2binary(format4, maskarray4, masknum4, data);
	if(ret != 17)
	{
		DERROR("mask_array2binary error. ret:%d\n", ret);
		return -1;
	}
	else
	{
    	printb(data, ret);
		ret = memcmp(data, mask4, 17);
		if(0 != ret)
		{
			DERROR("mask4 memcmp error. \n");
			return -1;
		}
		else
			printf("test3 ok!\n");
	}

	return 0;
	//printf("%x\n", UINT_MAX);
    //printb(data, ret);
}

int main()
{
	//#define DEBUG

	logfile_create("stdout", 4);
	int n = 0x03 & 0x02;
	printf("-----------> %d\n", n);
	int ret;
	int i = 0;
	for(i = 0; i < 0; i++)
		mask_string2array_test();

	//mask_array2binary_test();
	mask_string2binary_binary2string();
}
