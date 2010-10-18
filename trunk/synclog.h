#ifndef MEMLINK_SYNCLOG_H
#define MEMLINK_SYNCLOG_H

#include <stdio.h>
#include <limits.h>

#define SYNCLOG_INDEXNUM 1000000

#define SYNCLOG_OK   0
#define SYNCLOG_FULL 1

#define SYNCLOG_NAME "bin.log"

#define SYNCLOG_HEADER_LEN  (sizeof(short)+sizeof(int)+sizeof(int))

/**
 * header and index area are mapped in memory address space.
 */
typedef struct _synclog
{
    char    filename[PATH_MAX]; // file path
    int     fd;                 // open file descriptor
    char    *index;             // mmap addr
    int     len;                // mmap len
    int     headlen;            // header length
    unsigned int    index_pos;  // last index pos
    unsigned int    pos;        // last write data pos
}SyncLog;

//SyncLog*    synclog_create(char *filename);
SyncLog*    synclog_create();
int         synclog_new(SyncLog *slog);
int         synclog_validate(SyncLog *slog);
int         synclog_write(SyncLog *slog, char *data, int datalen);
void        synclog_destroy(SyncLog *slog);
int         synclog_rotate(SyncLog *slog);
int         synclog_lastlog();

#endif
