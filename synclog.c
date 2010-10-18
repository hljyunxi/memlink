#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "logfile.h"
#include "myconfig.h"
#include "mem.h"
#include "dumpfile.h"
#include "synclog.h"
#include "zzmalloc.h"
#include "utils.h"

static int
truncate_file(int fd, int len)
{
    int ret;
    while (1) {
        ret = ftruncate(fd, len);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }else{
                DERROR("ftruncate %d, %d error: %s\n", fd, len, strerror(errno));
                MEMLINK_EXIT;
            }
        }
        break;
    }
    return 0;
}

SyncLog*
synclog_create()
{
    SyncLog *slog;

    slog = (SyncLog*)zz_malloc(sizeof(SyncLog));
    if (NULL == slog) {
        DERROR("malloc SyncLog error!\n");
        MEMLINK_EXIT;
        return NULL;
    }
    memset(slog, 0, sizeof(SyncLog));
    
    slog->headlen = sizeof(short) + sizeof(int) + sizeof(int);

    snprintf(slog->filename, PATH_MAX, "%s/%s", g_cf->datadir, SYNCLOG_NAME);
    DINFO("synclog filename: %s\n", slog->filename);

    //int ret;
    //struct stat stbuf;

    // check file exist
	/*
    ret = stat(slog->filename, &stbuf);
    //DINFO("stat: %d, err: %s\n", ret, strerror(errno));
    if (ret == -1 && errno == ENOENT) { // not found file, check last log id from disk filename
        DINFO("not found sync logfile, check version in disk.\n");
        unsigned int lastver = synclog_lastlog();
        DINFO("synclog_lastlog: %d\n", lastver);
        g_runtime->logver = lastver;
    }*/


    DINFO("try open sync logfile ...\n");
    //slog->fd = open(slog->filename, O_RDWR|O_CREAT|O_APPEND, 0644);
    slog->fd = open(slog->filename, O_RDWR|O_CREAT, 0644);
    if (slog->fd == -1) {
        DERROR("open synclog %s error: %s\n", slog->filename, strerror(errno));
        zz_free(slog);
        MEMLINK_EXIT;
        return NULL;
    }

    int len = slog->headlen + SYNCLOG_INDEXNUM * sizeof(int);
    slog->len = len;
    int end = lseek(slog->fd, 0, SEEK_END);
    DINFO("synclog end: %d\n", end);
    lseek(slog->fd, 0, SEEK_SET);

    if (end == 0 || end < len) { // new file
        g_runtime->logver = synclog_lastlog();

        /*if (end > 0 && end < len) {
            //truncate_file(slog->fd, 0);

            g_runtime->logver = synclog_lastlog();
            DINFO("end < len, synclog_lastlog: %d\n", g_runtime->logver);
        }*/

        unsigned short format = DUMP_FORMAT_VERSION;
        unsigned int   newver = g_runtime->logver + 1;
        unsigned int   synlen = SYNCLOG_INDEXNUM;

        //DINFO("synclog new ver: %d, %x, synlen: %d, %x\n", newver, newver, synlen, synlen);
        if (writen(slog->fd, &format, sizeof(short), 0) < 0) {
            DERROR("write synclog format error: %d\n", format);
            MEMLINK_EXIT;
        }
        if (writen(slog->fd, &newver, sizeof(int), 0) < 0) {
            DERROR("write synclog newver error: %d\n", newver);
            MEMLINK_EXIT;
        }
        if (writen(slog->fd, &synlen, sizeof(int), 0) < 0) {
            DERROR("write synclog synlen error: %d\n", synlen);
            MEMLINK_EXIT;
        }

        truncate_file(slog->fd, len);
        slog->pos = slog->len;

        g_runtime->logver = newver;
    }
       
    DINFO("mmap file ... len:%d, fd:%d\n", slog->len, slog->fd);
    slog->index = mmap(NULL, slog->len, PROT_READ|PROT_WRITE, MAP_SHARED, slog->fd, 0);
    if (slog->index == MAP_FAILED) {
        DERROR("synclog mmap error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }

    if (end >= len) {
        g_runtime->logver = *(unsigned int*)(slog->index + sizeof(short));

        DINFO("validate synclog ...\n");
        if (synclog_validate(slog) == SYNCLOG_FULL) {
            // start a new synclog
            synclog_rotate(slog);
        }
    }

    DINFO("=== runtime logver: %u file logver: %u ===\n", g_runtime->logver, (unsigned int)*(slog->index + sizeof(short)));
    DINFO("index_pos: %d, pos: %d\n", slog->index_pos, slog->pos);
    g_runtime->synclog = slog;

    return slog;
}

int
synclog_new(SyncLog *slog)
{
    slog->fd = open(slog->filename, O_RDWR|O_CREAT, 0644);
    if (slog->fd == -1) {
        DERROR("open synclog %s error: %s\n", slog->filename, strerror(errno));
        zz_free(slog);
        MEMLINK_EXIT;
        return -1;
    }

    g_runtime->logver += 1;
    unsigned short format = DUMP_FORMAT_VERSION;
    unsigned int   newver = g_runtime->logver;
    unsigned int   synlen = SYNCLOG_INDEXNUM;
    
    DINFO("synclog new ver: %d, %x, synlen: %d, %x\n", newver, newver, synlen, synlen);
    if (writen(slog->fd, &format, sizeof(short), 0) < 0) {
        DERROR("write synclog format error: %d\n", format);
        MEMLINK_EXIT;
    }
    if (writen(slog->fd, &newver, sizeof(int), 0) < 0) {
        DERROR("write synclog newver error: %d\n", newver);
        MEMLINK_EXIT;
    }
    if (writen(slog->fd, &synlen, sizeof(int), 0) < 0) {
        DERROR("write synclog synlen error: %d\n", synlen);
        MEMLINK_EXIT;
    }
    
    fsync(slog->fd);

    truncate_file(slog->fd, slog->len);

    slog->index = mmap(NULL, slog->len, PROT_READ|PROT_WRITE, MAP_SHARED, slog->fd, 0);
    if (slog->index == MAP_FAILED) {
        DERROR("synclog mmap error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }

    slog->pos       = slog->len;
    slog->index_pos = 0;

    return 0;
}

/**
 * Rotate sync log if the current sync log is not empty. The current sync log 
 * is renamed to bin.log.xxx.
 *
 * @param slog sync log
 */
int
synclog_rotate(SyncLog *slog)
{
    unsigned int    newver; 
    char            newfile[PATH_MAX];
    int             ret;
    
    if (slog->index_pos == 0) {
        DWARNING("rotate cancle, no data!\n");
        return 0;
    }

    memcpy(&newver, slog->index + sizeof(short), sizeof(int));

    ret = munmap(slog->index, slog->len);
    if (ret == -1) {
        DERROR("munmap error: %s\n", strerror(errno));
    }
    slog->index = NULL;
    close(slog->fd);
    slog->fd = -1;
    
    snprintf(newfile, PATH_MAX, "%s.%d", slog->filename, newver);
    DINFO("rotate to: %s\n", newfile);
    if (rename(slog->filename, newfile) == -1) {
        DERROR("rename error: %s\n", newfile);
    }
    //g_runtime->logver += 1;

    synclog_new(slog);
    
    return 0;
}

int 
synclog_validate(SyncLog *slog)
{
    int i;
    int looplen = (slog->len - slog->headlen) / sizeof(int); // index zone length
    char *data  = slog->index + slog->headlen; // skip to index
    unsigned int *loopdata = (unsigned int*)data;
    unsigned int lastidx   = slog->len;

    for (i = 0; i < looplen; i++) {
        if (loopdata[i] == 0) {
            //i -= 1;
            break;
        }
        lastidx = loopdata[i];
    }

    slog->index_pos = i;

    unsigned short dlen;
    int            filelen = lseek(slog->fd, 0, SEEK_END);
    
    DINFO("filelen: %d, lastidx: %d, i: %d\n", filelen, lastidx, i);

    unsigned int oldidx = lastidx; 
    while (lastidx < filelen) {
        int cur = lseek(slog->fd, lastidx, SEEK_SET);
        DINFO("check offset: %d\n", lastidx);
        if (readn(slog->fd, &dlen, sizeof(short), 0) != sizeof(short)) {
            DERROR("synclog readn error, lastidx: %u, cur: %u\n", lastidx, cur);
            MEMLINK_EXIT;
        }

        if (filelen == cur + dlen + sizeof(short)) { // size ok
            slog->pos = filelen;
            break;
        }else if (filelen < cur + dlen + sizeof(short)) { // too small
            DWARNING("synclog data too small, fixed, at:%u, index:%d\n", cur, i);
            DINFO("index: %p, loopdata: %p\n", slog->index, loopdata);
            slog->index_pos -= 1;
            loopdata[slog->index_pos] = 0;
            slog->pos = cur;

            DINFO("v: %d\n", *(unsigned int*)(slog->index + 10 + (i-1) * 4));
            DINFO("v: %d\n", *(unsigned int*)(slog->index + 10 + i * 4));
            //msync(slog->index, slog->len, MS_SYNC);
            //lseek(slog->fd, idx, SEEK_SET);
            break;
        }else{
            lastidx = lastidx + sizeof(short) + dlen; // skip to next
            if (i == 0 || lastidx > oldidx) { // add index
                loopdata[slog->index_pos] = cur;
                slog->index_pos += 1;
            }
        }
    }

    return 0;
}

int
synclog_write(SyncLog *slog, char *data, int datalen)
{
    int wlen = datalen;
    int wpos = 0;
    int ret;
    int cur;
    //int pos = lseek(slog->fd, 0, SEEK_CUR);
    //char buf[128];
    
	DINFO("datalen: %d, wlen: %d, pos:%d\n", datalen, wlen, slog->pos);
    cur = lseek(slog->fd, slog->pos, SEEK_SET);
	/*
    DINFO("write synclog, cur: %u, pos: %d, %d, wlen: %d, %s\n", cur, slog->pos, 
				(unsigned int)lseek(slog->fd, 0, SEEK_CUR),
				wlen, formath(data, wlen, buf, 128));
	ret = write(slog->fd, "zhaowei", 7);
	DINFO("write test ret: %d, cur: %d\n", ret, (unsigned int)lseek(slog->fd, 0, SEEK_CUR));
	*/	
    while (wlen > 0) {
        /*int old = lseek(slog->fd, 0, SEEK_CUR);
        char databuf[1023];
        int rsize = read(slog->fd, databuf, 23);
        DINFO("read: %s\n", formath(databuf, rsize, buf, 128)); 
        lseek(slog->fd, old, SEEK_SET);
		*/

		//DINFO("write pos: %u wpos:%d, wlen:%d, data:%s\n", (unsigned int)lseek(slog->fd, 0, SEEK_CUR), wpos, wlen, formath(data+wpos, wlen, buf, 128));
		DINFO("write pos: %u wpos:%d, wlen:%d\n", (unsigned int)lseek(slog->fd, 0, SEEK_CUR), wpos, wlen);
        ret = write(slog->fd, data + wpos, wlen);
        DINFO("write return:%d\n", ret);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }else{
                DERROR("write synclog error: %s\n", strerror(errno));
                MEMLINK_EXIT;
                return -1;
            }
        }else{
            wlen -= ret; 
            wpos += ret;
        }
    }
	
	DINFO("after write pos: %u\n", (unsigned int)lseek(slog->fd, 0, SEEK_CUR));

    //unsigned int *idxdata = (unsigned int*)(slog->index + sizeof(short) + sizeof(int) * 2);
    unsigned int *idxdata = (unsigned int*)(slog->index + slog->headlen);
    DINFO("write index: %u, %u\n", slog->index_pos, slog->pos);
    idxdata[slog->index_pos] = slog->pos;
    slog->index_pos ++;
    slog->pos += datalen;

    return 0;
}

void
synclog_destroy(SyncLog *slog)
{
    if (NULL == slog)
        return;
    
    if (munmap(slog->index, slog->len) == -1) {
        DERROR("munmap error: %s\n", strerror(errno));
    }
   
    close(slog->fd); 
    zz_free(slog);
}

/**
 * synclog_lastlog - find the log number for the latest log file.
 */
int
synclog_lastlog()
{
    DIR     *mydir; 
    struct  dirent *nodes;
    //struct  dirent *result;
    int     maxid = 0;

    mydir = opendir(g_cf->datadir);
    if (NULL == mydir) {
        DERROR("opendir %s error: %s\n", g_cf->datadir, strerror(errno));
        return 0;
    }
    //DINFO("readdir ...\n");
    //while (readdir_r(mydir, nodes, &result) == 0 && nodes) {
    while ((nodes = readdir(mydir)) != NULL) {
        DINFO("name: %s\n", nodes->d_name);
        if (strncmp(nodes->d_name, "bin.log.", 8) == 0) {
            int binid = atoi(&nodes->d_name[8]);
            if (binid > maxid) {
                maxid = binid;
            }
        }
    }
    closedir(mydir);

    return maxid;
}



