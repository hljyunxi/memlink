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

ssize_t 
readn(int fd, void *vptr, size_t n)
{
    size_t  nleft;
    ssize_t nread;
    char    *ptr;

    ptr = vptr;
    nleft = n;

    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR) {
                nread = 0;
            }else {
                DERROR("readn error: %s\n", strerror(errno));
                MEMLINK_EXIT;
                return -1;
            }
        }else if (nread == 0)
            break;
        nleft -= nread;
        ptr += nread;
    }

    return (n - nleft);
}

size_t
writen(int fd, const void *vptr, size_t n)
{
    size_t  nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;

    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR){
                nwritten = 0;
            }else{
                DERROR("writen error: %s\n", strerror(errno));
                MEMLINK_EXIT;
                return -1;
            }
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
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
    snprintf(slog->filename, PATH_MAX, "%s/%s", g_cf->datadir, SYNCLOG_NAME);

    int ret;
    struct stat stbuf;

    ret = stat(slog->filename, &stbuf);
    if (ret == -1 && errno == ENOENT) { // not found file, check last log id from disk filename
        unsigned int lastver = synclog_lastlog();
        DINFO("synclog_lastlog: %u\n", lastver);
        g_runtime->logver = lastver;
    }

    slog->fd = open(slog->filename, O_RDWR|O_CREAT|O_APPEND, 0644);
    if (slog->fd == -1) {
        DERROR("open synclog %s error: %s\n", slog->filename, strerror(errno));
        zz_free(slog);
        MEMLINK_EXIT;
        return NULL;
    }

    int len = sizeof(short) + sizeof(int) + sizeof(int) + SYNCLOG_INDEXNUM * sizeof(int);
    slog->len = len;
    int cur = lseek(slog->fd, 0, SEEK_SET);
    DINFO("synclog cur: %d\n", cur);

    if (cur == 0) {
        truncate_file(slog->fd, len);

        unsigned short format = DUMP_FORMAT_VERSION;
        unsigned int newver = g_runtime->logver + 1;
        writen(slog->fd, &format, sizeof(short));
        writen(slog->fd, &newver, sizeof(int));

        g_runtime->logver = newver;
    }else if (cur < len) { // file error, clear
        truncate_file(slog->fd, 0);
        truncate_file(slog->fd, len);
        unsigned short format = 0;
        readn(slog->fd, &format, sizeof(short));

        unsigned int nowver = 0;
        readn(slog->fd, &nowver, sizeof(int));

        format = DUMP_FORMAT_VERSION;
        unsigned int newver = g_runtime->logver + 1;
        write(slog->fd, &format, sizeof(short));
        write(slog->fd, &newver, sizeof(int));
        g_runtime->logver = newver;

    }else{ // validate index and data
        if (synclog_validate(slog) == SYNCLOG_FULL) {
            // start a new synclog
            synclog_rotate(slog);
        }
    } 
        
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
    
    truncate_file(slog->fd, slog->len);

    unsigned short format = DUMP_FORMAT_VERSION;
    unsigned int newver = g_runtime->logver + 1;
    writen(slog->fd, &format, sizeof(short));
    writen(slog->fd, &newver, sizeof(int));

    slog->index = mmap(NULL, slog->len, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED, slog->fd, 0);
    if (slog->index == MAP_FAILED) {
        DERROR("synclog mmap error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }

    return 0;
}

int
synclog_rotate(SyncLog *slog)
{
    //int     ret;
    int     newver = g_runtime->logver + 1; 
    char    newfile[PATH_MAX];

    munmap(slog->index, slog->len);
    slog->index = NULL;
    close(slog->fd);
    slog->fd = -1;

    snprintf(newfile, PATH_MAX, "%s.%d", newfile, newver);
    if (rename(slog->filename, newfile) == -1) {
        DERROR("rename error: %s\n", newfile);
    }
    g_runtime->logver += 1;

    synclog_new(slog);
    
    return 0;
}

int 
synclog_validate(SyncLog *slog)
{
    int i;
    int looplen = slog->len - sizeof(short) - sizeof(int) * 2;
    char *data  = slog->index + sizeof(short) + sizeof(int) * 2;
    unsigned int *loopdata = (unsigned int*)data;
    unsigned int lastidx = 0;

    for (i = 0; i < looplen; i++) {
        if (loopdata[i] == 0) {
            //off_t nowpos = lseek(slog->fd, 0, SEEK_SET);
            lseek(slog->fd, lastidx, SEEK_SET);
        }
        lastidx = loopdata[i];
    }

    return 0;
}

int
synclog_write(SyncLog *slog, char *data, int datalen)
{
    int wlen = datalen;
    int ret;
    int pos = lseek(slog->fd, 0, SEEK_SET);

    while (wlen > 0) {
        ret = write(slog->fd, data, wlen);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }else{
                DERROR("write synclog error: %s\n", strerror(errno));
                if (lseek(slog->fd, pos, SEEK_SET) == -1) {
                    DERROR("lseek synclog error: %s\n", strerror(errno));
                    MEMLINK_EXIT;
                }
                return -1;
            }
        }else{
            wlen -= ret; 
        }
    }
    unsigned int *idxdata = (unsigned int*)(slog->index + sizeof(short) + sizeof(int));
    idxdata[slog->idxpos] = pos;
    slog->idxpos += 1;
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

int
synclog_lastlog()
{
    DIR     *mydir; 
    struct  dirent *nodes;
    struct  dirent *result;
    int     maxid = 0;

    mydir = opendir(g_cf->datadir);
    if (NULL == mydir) {
        DERROR("opendir %s error: %s\n", g_cf->datadir, strerror(errno));
        return 0;
    }
    while (readdir_r(mydir, nodes, &result) == 0) {
        if (nodes == NULL)
            break;
        if (strncmp(nodes->d_name, "bin.log.", 8) == 0) {
            int binid = atoi(&nodes->d_name[8]);
            if (binid > maxid) {
                maxid = binid;
            }
        }
    }
    return maxid;
}



