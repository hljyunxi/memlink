/**
 * 调试信息
 * @file logfile.c
 * @author zhaowei
 * @ingroup memlink
 * @{
 */
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "zzmalloc.h"
#include "logfile.h"

LogFile *g_log;

LogFile*
logfile_create(char *filename, int loglevel)
{
    LogFile *logger;
    
    logger = (LogFile*)zz_malloc(sizeof(LogFile));
    if (NULL == logger) {
        DERROR("malloc LogFile error!\n");
        return NULL;
    }
	memset(logger, 0, sizeof(LogFile));
    logger->loglevel = loglevel;

    if (strcmp(filename, "stdout") == 0) {
        logger->filename[0] = 0;
        logger->logfd = STDOUT_FILENO;
    }else{
        strncpy(logger->filename, filename, 1023);
        logger->logfd = open(filename, O_CREAT|O_WRONLY|O_APPEND, 0644);
        if (-1 == logger->logfd) {
            fprintf(stderr, "open log file %s error: %s\n", filename, strerror(errno));
            zz_free(logger);
			MEMLINK_EXIT;
            return NULL;
        }
    }

    g_log = logger;

    return logger;
}

void
logfile_destroy(LogFile *log)
{
    if (NULL == log) {
        return;
    }
    //fsync(log->fd);
    close(log->logfd); 
    zz_free(log);

    g_log = NULL;
}


void
logfile_write(LogFile *log, char *level, char *file, int line, char *format, ...)
{
    char    buffer[8192];
    char    *writepos;
    va_list arg;
    int     maxlen = 8192;
    int     ret, wlen = 0;
    time_t  timenow;
    char    *shortname;
    struct tm   timestru;
   
    time(&timenow);
    localtime_r(&timenow, &timestru);

    shortname = strrchr(file, '/');
    if (shortname) {
        file = shortname + 1;
    }

    ret = snprintf(buffer, 8192, "%d%02d%02d %02d:%02d:%02d %lu %s:%d [%s] ", 
                    timestru.tm_year+1900, timestru.tm_mon+1, timestru.tm_mday, 
                    timestru.tm_hour, timestru.tm_min, timestru.tm_sec, 
                    (unsigned long)pthread_self(), file, line, level);

    maxlen -= ret;
    writepos = buffer + ret;
    wlen = ret;
    
    //printf("maxlen: %d\n", maxlen);
    va_start (arg, format);
    ret = vsnprintf (writepos, maxlen, format, arg); 
    wlen += ret;
    va_end (arg);

    int rwlen = wlen;
    int wrn   = 0;

    while (rwlen > 0) {
        ret = write(log->logfd, buffer + wrn, rwlen);   
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }else{
                break;
            }
        }
        rwlen -= ret;
        wrn   += ret;
    }

	if (log->maxsize > 0) {
		struct stat fs;
		ret = fstat(log->logfd, &fs);
		if (ret == -1) {
			DERROR("log fstat error: %s\n", strerror(errno));
		}else{
			if (fs.st_size >= log->maxsize) {
				close(log->logfd);
				char newpath[PATH_MAX];
				snprintf(newpath, PATH_MAX, "%s.%d%02d%02d-%02d%02d%02d", log->filename, 
					timestru.tm_year+1900, timestru.tm_mon+1, timestru.tm_mday, 
                    timestru.tm_hour, timestru.tm_min, timestru.tm_sec);
				ret = rename(log->filename, newpath);
				if (ret == -1) {
					DERROR("rename log to %s error! %s\n", newpath, strerror(errno));
					MEMLINK_EXIT;
				}
				log->logfd = open(log->filename, O_CREAT|O_WRONLY|O_APPEND, 0644);
				if (-1 == log->logfd) {
					fprintf(stderr, "open log file %s error: %s\n", log->filename, strerror(errno));
					zz_free(log);
					MEMLINK_EXIT;
				}

			}
		}

	}
}

/**
 * @}
 */

