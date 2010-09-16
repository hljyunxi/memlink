#ifndef MEMLINK_LOGFILE_H
#define MEMLINK_LOGFILE_H

#include <stdio.h>
#include <limits.h>
#include "myconfig.h"

typedef struct _logfile
{
    char    filename[PATH_MAX];
    int     loglevel;
    int     logfd;
}LogFile;

LogFile*    logfile_create(char *filename, int loglevel);
void        logfile_destroy(LogFile *log);
void        logfile_write(LogFile *log, char *level, char *file, int line, char *format, ...);

extern LogFile *g_log;

#define MEMLINK_EXIT \
    abort()

#define DERROR(format,args...) do{\
        if (!g_log) {\
            fprintf(stderr, format, ##args);\
        }else if(g_log->loglevel >= 1) {\
            logfile_write(g_log, "ERR", __FILE__, __LINE__, format, ##args);\
        }\
    }while(0)

#define DWARNING(format,args...) do{\
        if (!g_log) {\
            fprintf(stderr, format, ##args);\
        }else if(g_log->loglevel >= 2) {\
            logfile_write(g_log, "WARN", __FILE__, __LINE__, format, ##args);\
        }\
    }while(0)

#define DINFO(format,args...) do{\
        if (!g_log) {\
            fprintf(stderr, format, ##args);\
        }else if (g_log->loglevel >= 3) {\
            logfile_write(g_log, "INFO", __FILE__, __LINE__, format, ##args);\
        }\
    }while(0)


#endif
