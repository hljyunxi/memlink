#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include "serial.h"
#include "logfile.h"
#include "utils.h"

/*
void printb(char *data, int datalen)
{
    int i, j;
    unsigned char c;

    for (i = 0; i < datalen; i++) {
        c = 0x80;
        for (j = 0; j < 8; j++) {
            if (c & data[datalen - i - 1]) {
                printf("1");
            }else{
                printf("0");
            }
            c = c >> 1;
        }
        printf(" ");
    }
    printf("\n");
}*/


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

}
END_TEST
