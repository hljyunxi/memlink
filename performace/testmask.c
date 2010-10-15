#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serial.h"
#include "utils.h"
#include "logfile.h"

int main()
{
    unsigned char format[3] = {4, 3, 1};
    unsigned int  maskarray[3] = {7, 2, 1};
    unsigned int  masknum = 3;
    char          data[1024] = {0}; 
    int           ret;
    int           v = 8;

    DINFO("format 4:3:1, array: 7:2:1 \n");
    ret = mask_array2binary(format, maskarray, masknum, data);
    DINFO("mask_array2binary ret: %d\n", ret);

    printb(data, ret);
	
	int i;
	char result[256] = {0};
	for (i = 0; i < 1000000; i++) {
		mask_binary2string(format, 3, data, ret, result);
	}
	printf("to string: %s\n", result);


	return 0;
}


