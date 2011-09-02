#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
    short       formatver;
    uint32_t    dumpver; 
    uint32_t    logver;
    uint32_t    logpos;
    uint64_t    size;
}DumpHead;

typedef struct
{
    uint8_t keylen; 
    char    key[256];
    char    listtype;
    char    valuetype;
    char    sortfield;
    char    valuesize;
    char    attrsize;
    char    attrnum;
    char    attrformat[16];
    uint32_t count;
    char     *data;
}OldDumpList;

typedef struct
{
    uint8_t namelen; 
    char    name[256];
    char    listtype;
    char    valuetype;
    char    sortfield;
    char    valuesize;
    char    attrsize;
    char    attrnum;
    char    attrformat[16];
}NewDumpTable;

typedef struct
{
    uint8_t keylen;
    char    key[256];
    uint32_t count;
    char     *data;
}NewDumpList;

int copy_head(FILE *oldf, FILE *newf)
{
    char buf[1024] = {0}; 
    int  size = 2+4+4+4+8; 
    int  ret; 

    ret = fread(buf, 1, size, oldf);
    if (ret != size) {
        printf("fread head from oldump error! %d\n", ret);
        return -1;
    }
    
    ret = fwrite(buf, 1, size, newf);
    if (ret != size) {
        printf("fwrite head to newump error! %d\n", ret);
        return -1;
    }
    // note: size must be rewrited after convert all  
    return 0;
}

int olddump_read_list(FILE *fp, OldDumpList *odump)
{
    int ret;
    int datalen;
    
    ret = fread(&odump->listtype, sizeof(char), 1, fp);
    if (ret != 1) {
        printf("end of file. %d\n", ret);
        return -1;
    }
    ret = fread(&odump->sortfield, sizeof(char), 1, fp);
    ret = fread(&odump->valuetype, sizeof(char), 1, fp);
    ret = fread(&odump->keylen, sizeof(unsigned char), 1, fp);
    ret = fread(odump->key, odump->keylen, 1, fp);
    odump->key[odump->keylen] = 0;
    
    ret = fread(&odump->valuesize, sizeof(unsigned char), 1, fp);
    ret = fread(&odump->attrsize, sizeof(unsigned char), 1, fp);
    ret = fread(&odump->attrnum, sizeof(char), 1, fp);
    if (odump->attrnum > 0) {
        ret = fread(odump->attrformat, sizeof(char) * odump->attrnum, 1, fp);
    }   

    /*for (i = 0; i < attrnum; i++) {
      DINFO("attrformat, i:%d, mask:%d\n", i, attrformat[i]);
      }*/
    ret = fread(&odump->count, sizeof(unsigned int), 1, fp);
    datalen = odump->valuesize + odump->attrsize;
   
    int datasize = datalen * odump->count; 
    
    if (odump->data != NULL) {
        free(odump->data);
    }
    odump->data = (char*)malloc(datasize);
    if (NULL == odump->data) {
        printf("malloc error!\n");
        return -1;
    }
    
    ret = fread(odump->data, 1, datasize, fp);
    if (ret != datasize) {
        printf("read data error! %d, %d\n", ret, datasize);
        return -1;
    }

    return 0;
}

int olddump_print(OldDumpList *odump)
{
    printf("======================\n");
    printf("keylen: %d\n", odump->keylen);
    printf("key: %s\n", odump->key);
    printf("listtype: %d\n", odump->listtype);
    printf("valuetype: %d\n", odump->valuetype);
    printf("sortfield: %d\n", odump->sortfield);
    printf("valuesize: %d\n", odump->valuesize);
    printf("attrsize:  %d\n", odump->attrsize);
    printf("attrnum:   %d\n", odump->attrnum);
    printf("attrformat: %s\n", odump->attrformat);
    printf("count:     %d\n", odump->count);
    //data;

    return 0;
}

int newdump_write_table(FILE *fp, char *table_name, OldDumpList *odump)
{
    unsigned char tlen = strlen(table_name);

    fwrite(&tlen, sizeof(char), 1, fp);
    fwrite(table_name, tlen, 1, fp);
    fwrite(&odump->listtype, sizeof(char), 1, fp);
    fwrite(&odump->valuetype, sizeof(char), 1, fp);
    fwrite(&odump->valuesize, sizeof(char), 1, fp);
    fwrite(&odump->sortfield, sizeof(char), 1, fp);
    fwrite(&odump->attrsize, sizeof(char), 1, fp);
    fwrite(&odump->attrnum, sizeof(char), 1, fp);

    if (odump->attrnum > 0) {
        fwrite(odump->attrformat, sizeof(char) * odump->attrnum, 1, fp);
    }   
    //int datalen = odump->valuesize + odump->attrsize;
    return 0;
}

int newdump_write_table_list(FILE *fp, char *list_key, OldDumpList *odump)
{
    unsigned char keylen = strlen(list_key);
    char flag = 1;    
    int ret;
    
    fwrite(&flag, sizeof(char), 1, fp);
    fwrite(&keylen, sizeof(char), 1, fp);
    fwrite(list_key, keylen, 1, fp);
    fwrite(&odump->count, sizeof(int), 1, fp);
    
    int datalen = odump->valuesize + odump->attrsize;
    // fixme: not bigger than 4G
    unsigned int datasize = datalen * odump->count;

    ret = fwrite(odump->data, datasize, 1, fp);
    if (ret != 1) {
        printf("write list error! %d, %d\n", ret, datasize);
    }

    return 0;
}

int newdump_write_table_end(FILE *fp)
{
    char flag = 0;    
    fwrite(&flag, sizeof(char), 1, fp);
    return 0;
}

int main(int argc, char *argv[])
{   
    if (argc != 3) {
        printf("usage: dumpconv olddump newdump\n");
        return 0;
    }
    FILE *oldf, *newf;
    
    oldf = fopen(argv[1], "rb");
    if (NULL == oldf) {
        printf("open old dump error!\n");
        return -1;
    }    

    newf = fopen(argv[2], "wb");
    if (NULL == oldf) {
        printf("open new dump error!\n");
        return -1;
    }    

    copy_head(oldf, newf);
    OldDumpList odump;
    memset(&odump, 0, sizeof(OldDumpList));
    
    char tbname[256] = {0};
    char key[256] = {0};
    char *sp = NULL;
    while (olddump_read_list(oldf, &odump) == 0) {
        //printf("load list: %s\n", odump.key);
        olddump_print(&odump);
        memset(tbname, 0, sizeof(tbname));
        memset(key, 0, sizeof(key));
        if ((sp = strchr(odump.key, '.')) != NULL) {
            *sp = 0;     
            strcpy(tbname, odump.key);
            strcpy(key, (sp+1));
        }else{
            strcpy(tbname, "test");
            strcpy(key, odump.key);
        }
        newdump_write_table(newf, tbname, &odump);
        newdump_write_table_list(newf, key, &odump);
        newdump_write_table_end(newf);
    }

    uint64_t fsize = ftell(newf);
        
    fseek(newf, 2+4+4+4, SEEK_SET);
    fwrite(&fsize, 1, sizeof(uint64_t), newf);

    fclose(oldf);
    fclose(newf);

    return 0;
}
