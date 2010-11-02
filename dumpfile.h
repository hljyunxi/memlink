#ifndef MEMLINK_DUMPFILE_H
#define MEMLINK_DUMPFILE_H

#include <stdio.h>
#include <limits.h>
#include "hashtable.h"

#define DUMP_FILE_NAME "dump.dat"
#define DUMP_FORMAT_VERSION 1

// format + version + log version + ismaster + size
#define DUMP_HEAD_LEN	(sizeof(short)+sizeof(int)+sizeof(int)+sizeof(char)+sizeof(long long))

int  dumpfile(HashTable *ht);
int  loaddump(HashTable *ht, char *filename);
void dumpfile_call_loop(int fd, short event, void *arg);
int  dumpfile_call();

#endif
