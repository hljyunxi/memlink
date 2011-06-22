#ifndef MEMLINK_ZZ_MALLOC_H
#define MEMLINK_ZZ_MALLOC_H

#include <stdio.h>

void*   zz_malloc(size_t size);
void    zz_free_dbg(void *ptr, char *file, int line);
void    zz_check_dbg(void *ptr, char *file, int line);
char*   zz_strdup(char *s);

#ifdef DEBUGMEM
#define zz_free(ptr) zz_free_dbg(ptr,__FILE__,__LINE__)
#define zz_check(ptr) zz_check_dbg(ptr,__FILE__,__LINE__)
#else
void    zz_free(void *ptr);
#define zz_check(ptr)
#endif

#endif
