#ifndef BASE_LOGFILE_H
#define BASE_LOGFILE_H

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <pthread.h>

#define LOG_ROTATE_NO		0
#define LOG_ROTATE_TIME		1
#define LOG_ROTATE_SIZE		2

#define LOG_INFO	4
#define LOG_NOTE	3
#define LOG_NOTICE	3
#define LOG_WARN	2
#define LOG_ERROR	1
#define LOG_FATAL	0

typedef struct _logfile
{
    volatile int logfd;
    volatile int errfd; // for error
    char     filepath[PATH_MAX];
    char     errpath[PATH_MAX];
    int      loglevel;
	int		 rotype; // rotate type: LOG_ROTATE_TIME(by minute), LOG_ROTATE_SIZE
	int		 maxsize; // logfile max size
	int		 maxtime; // logfile max time
	int		 count; // logfile count
	int		 check_interval;
	uint32_t last_rotate_time;
	uint32_t last_check_time; // last check time
	pthread_mutex_t lock;
}LogFile;

LogFile*    logfile_create(char *filepath, int loglevel);
void        logfile_destroy(LogFile *log);
int			logfile_error_separate(LogFile *log, char *errfile);
void		logfile_rotate_size(LogFile *log, int logsize, int logcount);
void		logfile_rotate_time(LogFile *log, int logtime, int logcount);
void		logfile_rotate_no(LogFile *log);
void		logfile_flush(LogFile *log);
void        logfile_write(LogFile *log, char *level, char *file, int line, char *format, ...);

extern LogFile *g_log;

#ifdef NOLOG

#define DFATALERR(format,args...) 
#define DERROR(format,args...) 
#define DWARNING(format,args...) 
#define DNOTE(format,args...) 
#define DINFO(format,args...) 
#define DASSERT(e) ((void)0)
#else

#define DFATALERR(format,args...) do{\
        if (!g_log) {\
            fprintf(stderr, format, ##args);\
        }else if(g_log->loglevel >= 0) {\
            logfile_write(g_log, "FATAL", __FILE__, __LINE__, format, ##args);\
			exit(EXIT_FAILURE);\
        }\
    }while(0)

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

#define DNOTE(format,args...) do{\
        if (!g_log) {\
            fprintf(stderr, format, ##args);\
        }else if (g_log->loglevel >= 3) {\
            logfile_write(g_log, "NOTE", __FILE__, __LINE__, format, ##args);\
        }\
    }while(0)

#define DNOTICE(format,args...) do{\
        if (!g_log) {\
            fprintf(stderr, format, ##args);\
        }else if (g_log->loglevel >= 3) {\
            logfile_write(g_log, "NOTICE", __FILE__, __LINE__, format, ##args);\
        }\
    }while(0)



#define DINFO(format,args...) do{\
        if (!g_log) {\
            fprintf(stderr, format, ##args);\
        }else if (g_log->loglevel >= 4) {\
            logfile_write(g_log, "INFO", __FILE__, __LINE__, format, ##args);\
        }\
    }while(0)

#define DASSERT(e) \
	((void) ((!g_log) ? ((void) ((e) ? 0 : (fprintf(stderr,"assert failed: (%s) %s %s:%d\n",#e,__FUNCTION__,__FILE__,__LINE__),abort()))) : ((void) ((e) ? 0 : (logfile_write(g_log,"ERR",__FILE__,__LINE__,"assert failed: (%s) %s %s:%d\n",#e,__FUNCTION__,__FILE__,__LINE__),abort())))))
	

#endif



#endif
