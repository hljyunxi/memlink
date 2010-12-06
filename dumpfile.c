#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <event.h>
#include <sys/time.h>
#include "myconfig.h"
#include "logfile.h"
#include "dumpfile.h"
#include "common.h"

/**
 * Creates a dump file from the hash table. The old dump file is replaced by 
 * the new dump file. Sync log is also rotated.
 * format:
 * ---------------------------------------------------------------------
 * | dumpfile format(2B) | dumpfile version (4B) | sync log version(4B)|
 * ---------------------------------------------------------------------
 * | dumpfile is master (1B) | dumpfile size (8B)| data |
 * ------------------------------------------------------
 *
 * @param ht hash table
 */
int 
dumpfile(HashTable *ht)
{
    HashNode    *node;
    HashNode    **bks = ht->bunks;

    char        tmpfile[PATH_MAX];
    char        dumpfile[PATH_MAX];
    int         i;
	long long   size = 0;
    
    DINFO("dumpfile start ...\n");

    snprintf(dumpfile, PATH_MAX, "%s/%s", g_cf->datadir, DUMP_FILE_NAME);
    snprintf(tmpfile, PATH_MAX, "%s/%s.tmp", g_cf->datadir, DUMP_FILE_NAME);

    DINFO("dumpfile to tmp: %s\n", tmpfile);
    
    //FILE    *fp = fopen(tmpfile, "wb");
    FILE    *fp = fopen64(tmpfile, "wb");

    unsigned short formatver = DUMP_FORMAT_VERSION;
    fwrite(&formatver, sizeof(short), 1, fp);
    DINFO("write format version %d\n", formatver);

    g_runtime->dumpver += 1;
    unsigned int dumpver = g_runtime->dumpver;
    fwrite(&dumpver, sizeof(int), 1, fp);
    DINFO("write dumpfile version %d\n", dumpver);

    unsigned int logver;
    if (g_runtime->synclog->index_pos > 0) {
        logver = g_runtime->logver + 1;
    }else{
        logver = g_runtime->logver;
    }
    fwrite(&logver, sizeof(int), 1, fp);
    DINFO("write logfile version %d\n", logver);

	fwrite(&g_cf->role, sizeof(char), 1, fp);
	DINFO("write role: %d\n", g_cf->role);

	fwrite(&size, sizeof(long long), 1, fp);

    unsigned char keylen;
    int datalen;
    int n;
    int dump_count = 0;

    for (i = 0; i < HASHTABLE_BUNKNUM; i++) {
        node = bks[i];
        while (NULL != node) {
            DINFO("start dump key: %s\n", node->key);
            keylen = strlen(node->key);
            fwrite(&keylen, sizeof(char), 1, fp);
			DINFO("dump keylen: %d\n", keylen);
            fwrite(node->key, keylen, 1, fp);
            fwrite(&node->valuesize, sizeof(char), 1, fp);
            fwrite(&node->masksize, sizeof(char), 1, fp);
            fwrite(&node->masknum, sizeof(char), 1, fp);
            fwrite(node->maskformat, sizeof(char) * node->masknum, 1, fp);
            /*for (k = 0; k < node->masknum; k++) {
                DINFO("dump mask, k:%d, mask:%d\n", k, node->maskformat[k]);
            }*/
            fwrite(&node->used, sizeof(int), 1, fp);
            datalen = node->valuesize + node->masksize;

            DataBlock *dbk = node->data;

            while (dbk) {
                char *itemdata = dbk->data;

                for (n = 0; n < g_cf->block_data_count; n++) {
					if (dataitem_have_data(node, itemdata, 0)) {	
						//fwrite(itemdata, node->valuesize, 1, fp);
						//fwrite(itemdata + node->valuesize, node->masksize, 1, fp);
                        fwrite(itemdata, datalen, 1, fp);
                        dump_count += 1;
					}
                    itemdata += datalen;
                }
                dbk = dbk->next;
            }
            node = node->next;
        }
    }
    
    DINFO("dump count: %d\n", dump_count);
	
	size = ftell(fp);

	fseek(fp,  DUMP_HEAD_LEN - sizeof(long long), SEEK_SET);
	fwrite(&size, sizeof(long long), 1, fp);

    fclose(fp);

    int ret = rename(tmpfile, dumpfile);
    DINFO("rename %s to %s return: %d\n", tmpfile, dumpfile, ret);
    if (ret == -1) {
        DERROR("dumpfile rename error: %s\n", strerror(errno));
    }
    
    // start a new sync log
	DINFO("start a new synclog.\n");
    synclog_rotate(g_runtime->synclog);

    return ret;
}

/**
 * @param ht
 * @param filename  dumpfile name
 * @param localdump  is local dump file. 
 */
int
dumpfile_load(HashTable *ht, char *filename, int localdump)
{
    FILE    *fp;
    //char    filename[PATH_MAX];
    int     filelen;
    
    //snprintf(filename, PATH_MAX, "%s/%s", g_cf->datadir, DUMP_FILE_NAME);
    //snprintf(filename, PATH_MAX, "%s/%s", g_cf->datadir, dumpfile_name);
    //fp = fopen(filename, "rb");
    fp = fopen64(filename, "rb");
    if (NULL == fp) {
        DERROR("open dumpfile %s error: %s\n", filename, strerror(errno));
        return -1;
    }
   
    fseek(fp, 0, SEEK_END);
    filelen = ftell(fp);
    DINFO("filelen: %d\n", filelen);
    fseek(fp, 0, SEEK_SET);
    unsigned short dumpfver;
    fread(&dumpfver, sizeof(short), 1, fp);
	DINFO("load format ver: %d\n", dumpfver);

    if (dumpfver != DUMP_FORMAT_VERSION) {
        DERROR("dumpfile format version error: %d, %d\n", dumpfver, DUMP_FORMAT_VERSION);
        fclose(fp);
        return -2;
    }
    
    unsigned int dumpver;
    fread(&dumpver, sizeof(int), 1, fp);
    DINFO("load dumpfile ver: %u\n", dumpver);
    if (localdump) {
        g_runtime->dumpver = dumpver;
    }

    unsigned int dumplogver;
    fread(&dumplogver, sizeof(int), 1, fp);
    DINFO("load dumpfile log ver: %u\n", dumplogver);
    if (localdump) {
        g_runtime->dumplogver = dumplogver;
    }

	char role;
	fread(&role, sizeof(char), 1, fp);

	long long size;
	fread(&size, sizeof(long long), 1, fp);

    unsigned char keylen;
    unsigned char masklen;
    unsigned char masknum;
    unsigned char maskformat[HASHTABLE_MASK_MAX_ITEM];
    unsigned char valuelen;
    unsigned int  itemnum;
    char          key[256];
    int           i;
    int           datalen;
    int           ret;
    int           load_count = 0;

    while (ftell(fp) < filelen) {
        DINFO("cur: %d, filelen: %d, %d\n", (int)ftell(fp), filelen, feof(fp));
        ret = fread(&keylen, sizeof(unsigned char), 1, fp);
        DINFO("keylen: %d\n", keylen);
        fread(key, keylen, 1, fp);
        key[keylen] = 0;
        DINFO("key: %s\n", key);
        
        fread(&valuelen, sizeof(unsigned char), 1, fp);
		DINFO("valuelen: %d\n", valuelen);
        fread(&masklen, sizeof(unsigned char), 1, fp);
		DINFO("masklen: %d\n", masklen);
        fread(&masknum, sizeof(char), 1, fp);
		DINFO("masknum: %d\n", masknum);
        fread(maskformat, sizeof(char) * masknum, 1, fp);
        /*for (i = 0; i < masknum; i++) {
            DINFO("maskformat, i:%d, mask:%d\n", i, maskformat[i]);
        }*/
        fread(&itemnum, sizeof(unsigned int), 1, fp);
        datalen = valuelen + masklen;
		DINFO("itemnum: %d, datalen: %d\n", itemnum, datalen);

        unsigned int maskarray[HASHTABLE_MASK_MAX_ITEM];
        for (i = 0; i < masknum; i++) {
            maskarray[i] = maskformat[i];
        }
        DINFO("create info, key:%s, valuelen:%d, masknum:%d\n", key, valuelen, masknum);
        ret = hashtable_add_info_mask(ht, key, valuelen, maskarray, masknum);
        if (ret != MEMLINK_OK) {
            DERROR("hashtable_add_info_mask error, ret:%d\n", ret);
            return -2;
        }
        HashNode    *node = hashtable_find(ht, key);
        DataBlock   *dbk = NULL;
        char        *itemdata = NULL;

        for (i = 0; i < itemnum; i++) {
            //DINFO("i: %d\n", i);
            if (i % g_cf->block_data_count == 0) {
                DINFO("create new datablock ...\n");
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
				node->all += g_cf->block_data_count;

                itemdata = dbk->data;
            }

            fread(itemdata, datalen, 1, fp);
           
			ret = dataitem_check_data(node, itemdata);
			if (ret == MEMLINK_ITEM_VISIBLE) {
				dbk->visible_count++;
			}else{
				dbk->mask_count++;
			}
			node->used++;

            char buf[256] = {0};
            memcpy(buf, itemdata, node->valuesize);
            DINFO("load value: %s\n", buf);

            itemdata += datalen;
            load_count += 1;
        }
    }
    
    fclose(fp);
    DINFO("load count: %d\n", load_count);

    return 0;
}

int
dumpfile_logver(char *filename)
{
    int  ret;
    FILE    *dumpf;

    //dumpf = fopen(filename, "r");
    dumpf = fopen64(filename, "r");
    if (dumpf == NULL) {
        DERROR("open file %s error! %s\n", filename, strerror(errno));
        return -1;
    }

    int pos = sizeof(short) + sizeof(int);
    fseek(dumpf, pos, SEEK_SET);

    int dump_logver = 0;
    ret = fread(&dump_logver, sizeof(int), 1, dumpf);
    if (ret != sizeof(int)) {
        DERROR("fread error: %s\n", strerror(errno));
        fclose(dumpf);
        return -1;
    }
    fclose(dumpf);

    return dump_logver;
}

void
dumpfile_call_loop(int fd, short event, void *arg)
{
    dumpfile_call();
	
	struct timeval	tv;
	struct event *timeout = arg;

	evutil_timerclear(&tv);
    tv.tv_sec = g_cf->dump_interval * 60; 
	event_add(timeout, &tv);
}

int
dumpfile_call()
{
    int ret;

    pthread_mutex_lock(&g_runtime->mutex);
    //g_runtime->indump = 1;
    ret = dumpfile(g_runtime->ht);
    //g_runtime->indump = 0;
    pthread_mutex_unlock(&g_runtime->mutex);
    
    return ret;
}


