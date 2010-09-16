#ifndef MEMLINK_ZZ_MALLOC_H
#define MEMLINK_ZZ_MALLOC_H

#include <stdio.h>

void*   zz_malloc(size_t size);
void    zz_free(void *ptr);
char*   zz_strdup(char *s);

#endif
