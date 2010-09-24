#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include "serial.h"
#include "logfile.h"
#include "utils.h"


START_TEST(test_mask)
{
    unsigned char format[3] = {4, 3, 1};
    unsigned int  maskarray[3] = {7, 2, 1};
    unsigned int  masknum = 3;
    char          data[1024] = {0}; 
    int           ret;
    int           v = 8;

    printb((char*)&v, 4);

    DINFO("format 4:3:1, array: 7:2:0 \n");
    ret = mask_array2binary(format, maskarray, masknum, data);
    DINFO("mask_array2binary ret: %d\n", ret);
    printb(data, ret);

    DINFO("==================================================\n");
    DINFO("flag 4:3:1, array: 10:3:1\n");

    maskarray[0] = 10;
    maskarray[1] = 3;
    maskarray[2] = 1;
    
    ret = mask_array2flag(format, maskarray, masknum, data);
    DINFO("mask_array2flag ret: %d\n", ret);
    printb(data, ret);

}
END_TEST
