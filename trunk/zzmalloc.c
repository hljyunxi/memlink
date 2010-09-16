#include <stdlib.h>
#include <string.h>
#include "logfile.h"
#include "zzmalloc.h"

void*
zz_malloc(size_t size)
{
    return malloc(size);
}

void
zz_free(void *ptr)
{
    free(ptr);
}

char*
zz_strdup(char *s)
{
    int len = strlen(s);

    char *ss = (char*)zz_malloc(len + 1);
    if (NULL == ss) {
        DERROR("malloc char* error!\n");
        return NULL;
    }

    strcpy(ss, s);

    return ss;
}
