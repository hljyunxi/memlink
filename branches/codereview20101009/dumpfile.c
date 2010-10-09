#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "myconfig.h"
#include "logfile.h"
#include "dumpfile.h"
#include "common.h"

int 
dumpfile(HashTable *ht)
{
    HashNode    *node;
    HashNode    **bks = ht->bunks;

    char        tmpfile[PATH_MAX];
    char        dumpfile[PATH_MAX];
    int         i;
    
    DINFO("dumpfile start ...\n");

    snprintf(dumpfile, PATH_MAX, "%s/%s", g_cf->datadir, DUMP_FILE_NAME);
    snprintf(tmpfile, PATH_MAX, "%s/%s.tmp", g_cf->datadir, DUMP_FILE_NAME);

    DINFO("dumpfile to tmp: %s\n", tmpfile);
    
    FILE    *fp = fopen(tmpfile, "wb");

    unsigned short formatver = DUMP_FORMAT_VERSION;
    fwrite(&formatver, sizeof(short), 1, fp);
    DINFO("write format version ok!\n");

    unsigned int dumpver = g_runtime->dumpver + 1;
    fwrite(&dumpver, sizeof(int), 1, fp);
    DINFO("write dumpfile version ok!\n");

    unsigned int logver = 0;
    fwrite(&logver, sizeof(int), 1, fp);
    DINFO("write logfile version ok!\n");

    unsigned char keylen;
    int datalen;
    int n;

    for (i = 0; i < HASHTABLE_BUNKNUM; i++) {
        node = bks[i];
        while (NULL != node) {
            DINFO("dump key: %s\n", node->key);
            keylen = strlen(node->key);
            fwrite(&keylen, sizeof(char), 1, fp);
            fwrite(&node->key, keylen, 1, fp);
            fwrite(&node->valuesize, sizeof(char), 1, fp);
            //fwrite(&node->valuetype, sizeof(char), 1, fp);
            fwrite(&node->masksize, sizeof(char), 1, fp);
            fwrite(&node->masknum, sizeof(char), 1, fp);
            fwrite(&node->maskformat, sizeof(char) * node->masknum, 1, fp);
            fwrite(&node->used, sizeof(int), 1, fp);
            datalen = node->valuesize + node->masksize;

            DataBlock *dbk = node->data;

            while (dbk) {
                char *itemdata = dbk->data;

                for (n = 0; n < g_cf->block_data_count; n++) {
                    fwrite(itemdata, node->valuesize, 1, fp);
                    fwrite(itemdata + node->valuesize, node->masksize, 1, fp);

                    itemdata += datalen;
                }
                dbk = dbk->next;
            }
            node = node->next;
        }
    }

    fclose(fp);

    int ret = rename(tmpfile, dumpfile);
    DINFO("rename %s to %s return: %d\n", tmpfile, dumpfile, ret);
    if (ret == -1) {
        DERROR("dumpfile rename error: %s\n", strerror(errno));
    }

    return ret;
}

int
loaddump(HashTable *ht)
{
    FILE    *fp;
    char    filename[PATH_MAX];
    
    snprintf(filename, PATH_MAX, "%s/%s", g_cf->datadir, DUMP_FILE_NAME);
    fp = fopen(filename, "r");
    if (NULL == fp) {
        DERROR("open dumpfile %s error: %s\n", filename, strerror(errno));
        return -1;
    }
    
    unsigned short dumpfver;
    fread(&dumpfver, sizeof(short), 1, fp);

    if (dumpfver != DUMP_FORMAT_VERSION) {
        DERROR("dumpfile format version error: %d, %d\n", dumpfver, DUMP_FORMAT_VERSION);
        fclose(fp);
        return -2;
    }

    fread(&g_runtime->dumpver, sizeof(int), 1, fp);
    DINFO("load dumpfile ver: %u\n", g_runtime->dumpver);

    fread(&g_runtime->dumplogver, sizeof(int), 1, fp);
    DINFO("load dumpfile log ver: %u\n", g_runtime->dumplogver);

    /*unsigned long long datale?n;
    fread(&datalen, sizeof(long long), 1, fp);
    DINFO("dumpfile datalen: %u\n", datalen);*/

    unsigned char keylen;
    unsigned char masklen;
    unsigned char masknum;
    unsigned char maskformat[HASHTABLE_MASK_MAX_LEN];
    unsigned char valuelen;
    //unsigned char valuetype;
    unsigned int  itemnum;
    char          key[256];
    int           i;
    int           datalen;

    while (!feof(fp)) {
        fread(&keylen, sizeof(unsigned char), 1, fp);
        DINFO("keylen: %d\n", keylen);
        fread(&key, keylen, 1, fp);
        key[keylen] = 0;
        DINFO("key: %s\n", key);
        
        fread(&valuelen, sizeof(unsigned char), 1, fp);
        //fread(&valuetype, sizeof(unsigned char), 1, fp);
        fread(&masklen, sizeof(unsigned char), 1, fp);
        fread(&masknum, sizeof(char), 1, fp);
        fread(&maskformat, sizeof(char) * masknum, 1, fp);
        fread(&itemnum, sizeof(unsigned int), 1, fp);

        datalen = valuelen + masklen;
        
        unsigned int maskarray[HASHTABLE_MASK_MAX_LEN];
        for (i = 0; i < masknum; i++) {
            maskarray[i] = maskformat[i];
        }
        hashtable_add_info_mask(ht, key, valuelen, maskarray, masknum);
        HashNode    *node = hashtable_find(ht, key);
        DataBlock   *dbk = NULL;
        char        *itemdata = NULL;

        for (i = 0; i < itemnum; i++) {
            if (i % g_cf->block_data_count == 0) {
                DataBlock *newdbk = mempool_get(g_runtime->mpool, datalen); 
                if (NULL == newdbk) {
                    DERROR("mempool_get NULL!\n");
                    MEMLINK_EXIT;
                }

                if (dbk == NULL) {
                    node->data = newdbk;
                }else{
                    dbk->next = newdbk;
                }
                dbk = newdbk;

                itemdata = dbk->data;
            }

            fread(&itemdata, datalen, 1, fp);
            itemdata += datalen;
        }
    }
    
    fclose(fp);

    return 0;
}

void
dumpfile_call_loop(int fd, short event, void *arg)
{
    dumpfile_call();
}

int
dumpfile_call()
{
    int ret;

    pthread_mutex_lock(&g_runtime->mutex);
    g_runtime->indump = 1;
    ret = dumpfile(g_runtime->ht);
    g_runtime->indump = 0;
    pthread_mutex_unlock(&g_runtime->mutex);
    
    return ret;
}


