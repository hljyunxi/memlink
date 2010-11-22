#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include "serial.h"
#include "logfile.h"
#include "utils.h"


START_TEST(test_mask)
{
    unsigned char format[3] = {4, 3, 1};
    unsigned int  maskarray[3] = {7, 0, 0};
    unsigned int  masknum = 3;
    char          data[128] = {0}; 
    int           ret;
    int           v = 8;

    printf("=================== start masktest ===========================\n");
    printb((char*)&v, 4);

    DINFO("format %d:%d:%d, array: %d:%d:%d\n", format[0], format[1], format[2], maskarray[0], maskarray[1], maskarray[2]);
    ret = mask_array2binary(format, maskarray, masknum, data);
    DINFO("mask_array2binary ret: %d\n", ret);

    printb(data, ret);
    fail_if(ret != 2, "mask length error: %d\n", ret);

	char result[256] = {0};

	mask_binary2string(format, 3, data, ret, result);
	printf("binary2string: %s\n", result);

    DINFO("==================== array2flag ==============================\n");
    //DINFO("flag 4:3:1, array: 10:3:1\n");
    char buf[64];

    maskarray[0] = 10;
    maskarray[1] = 3;
    maskarray[2] = 1;
   
    memset(data, 0, 128);
    ret = mask_array2flag(format, maskarray, masknum, data);
    DINFO("10:3:1 mask_array2flag ret: %d\n", ret);
    DINFO("flag: %s", formatb(data, ret, buf, 64));
    //fail_if(ret != 2, "flag length error: %d\n", ret);

	maskarray[0] = 10;
    maskarray[1] = UINT_MAX;
    maskarray[2] = 0;
    
    memset(data, 0, 128);
    ret = mask_array2flag(format, maskarray, masknum, data);
    DINFO("10:-1:0 mask_array2flag ret: %d\n", ret);
    DINFO("flag: %s", formatb(data, ret, buf, 64));
   
    maskarray[0] = 10;
    maskarray[1] = UINT_MAX;
    maskarray[2] = 1;
    
    memset(data, 0, 128);
    ret = mask_array2flag(format, maskarray, masknum, data);
    DINFO("10:-1:1 mask_array2flag ret: %d\n", ret);
    DINFO("flag: %s", formatb(data, ret, buf, 64));
   

    maskarray[0] = 10;
    maskarray[1] = UINT_MAX;
    maskarray[2] = UINT_MAX;
    
    memset(data, 0, 128);
    ret = mask_array2flag(format, maskarray, masknum, data);
    DINFO("10:-1:-1 mask_array2flag ret: %d\n", ret);
    DINFO("flag: %s", formatb(data, ret, buf, 64));
   
    maskarray[0] = 10;
    maskarray[1] = 1;
    maskarray[2] = UINT_MAX;
    
    memset(data, 0, 128);
    ret = mask_array2flag(format, maskarray, masknum, data);
    DINFO("10:1:-1 mask_array2flag ret: %d\n", ret);
    DINFO("flag: %s", formatb(data, ret, buf, 64));
   
    maskarray[0] = UINT_MAX;
    maskarray[1] = UINT_MAX;
    maskarray[2] = 0;
    
    memset(data, 0, 128);
    ret = mask_array2flag(format, maskarray, masknum, data);
    DINFO("-1:-1:0 mask_array2flag ret: %d\n", ret);
    DINFO("flag: %s", formatb(data, ret, buf, 64));
 
    maskarray[0] = UINT_MAX;
    maskarray[1] = UINT_MAX;
    maskarray[2] = UINT_MAX;
    
    memset(data, 0, 128);
    ret = mask_array2flag(format, maskarray, masknum, data);
    DINFO("-1:-1:-1 mask_array2flag ret: %d\n", ret);
    DINFO("flag: %s", formatb(data, ret, buf, 64));
 
    maskarray[0] = 0;
    maskarray[1] = 0;
    maskarray[2] = 0;
    
    memset(data, 0, 128);
    ret = mask_array2flag(format, maskarray, masknum, data);
    DINFO("0:0:0 mask_array2flag ret: %d\n", ret);
    DINFO("flag: %s", formatb(data, ret, buf, 64));
 
    
    //fail_if(ret != 2, "flag length error: %d\n", ret);
}
END_TEST
