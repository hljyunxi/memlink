#include <stdlib.h>
#include <string.h>
#include "logfile.h"
#include "utils.h"
#include "zzmalloc.h"

void*
zz_malloc(size_t size)
{
#ifdef DEBUGMEM
	void *ptr = malloc(size + 12);
	char  *b = (char*)ptr;

	*((int*)b) = size;
	*((int*)(b + 4)) = 0x55555555;
	*((int*)(b + 8 + size)) = 0x55555555;
		
	return b + 8;
#else

#ifdef USETCMALLOC
    return tc_malloc(size);
#else
    return malloc(size);
#endif

#endif
}

#ifndef DEBUGMEM
void
zz_free(void *ptr)
{
#ifdef USETCMALLOC
    tc_free(ptr);
#else
    free(ptr);
#endif
}
#endif


void
zz_check_dbg(void *ptr, char *file, int line)
{
    if (NULL == ptr) {
        DERROR("check NULL, file:%s, line:%d\n", file, line);
        MEMLINK_EXIT;
    }

	char *b = ptr - 8;
	int  size = *((int*)b);

	if (*((int*)(b + 4)) != 0x55555555 || *((int*)(b + 8 + size)) != 0x55555555) {
		char buf1[128] = {0};
		char buf2[128] = {0};

		DERROR("check error! %p, size:%d, file:%s, line:%d, %s, %s\n", ptr, size, file, line, 
					formatb(ptr-4, 4, buf1, 128), formatb(ptr+size+8, 4, buf2, 128));
		MEMLINK_EXIT;
	}
}

void
zz_free_dbg(void *ptr, char *file, int line)
{
    if (NULL == ptr) {
        DERROR("free NULL, file:%s, line:%d\n", file, line);
        MEMLINK_EXIT;
    }
	char *b = ptr - 8;
	zz_check_dbg(ptr, file, line);
	
    free(b);
}

char*
zz_strdup(char *s)
{
    int len = strlen(s);

    char *ss = (char*)zz_malloc(len + 1);
    if (NULL == ss) {
        DERROR("zz_strdump malloc error!\n");
        MEMLINK_EXIT;
        return NULL;
    }

    strcpy(ss, s);

    return ss;
}
