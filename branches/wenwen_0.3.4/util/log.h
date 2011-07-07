#ifndef MEMLINK_UTIL_LOG
#define MEMLINK_UTIL_LOG

#include <stdarg.h>
#include <stdint.h>


#define CUT_BY_HOUR     0
#define CUT_BY_DAY      1

#define L_NOLOG     0
#define L_ERROR     1
#define L_WARNING   2
#define L_NOTICE    3
#define L_DEBUG     4


#define ERROR(format, ...)      log_print(L_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define WARNING(format, ...)    log_print(L_WARNING, __FILE__, __LINE__,  format, ##__VA_ARGS__)
#define NOTICE(format, ...)     log_print(L_NOTICE, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define DEBUG(format, ...)      log_print(L_DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define PUT_ERROR(format, ...)      log_put(L_ERROR, format, ##__VA_ARGS__)
#define PUT_WARNING(format, ...)    log_put(L_WARNING, format, ##__VA_ARGS__)
#define PUT_NOTICE(format, ...)     log_put(L_NOTICE, format, ##__VA_ARGS__)
#define PUT_DEBUG(format, ...)      log_put(L_DEBUG, format, ##__VA_ARGS__)
#define FLUSH_PUT() log_flush_put(__FILE__, __LINE__);

/*
 * Function: Initialization for log, each process can only call this
 *          function once.
 * Arguments:
 *          char *path:             Path of the log file, for example: "/tmp"
 *          char *base_name         Name of the log, if NULL, name of log level
 *                                  will be used.
 *          int  level              Global level of this initialization
 *          int  cut_type:          Define cut type, by hour or by day
 *                                  (CUT_BY_HOUR or CUT_BY_DAY)
 *          uint32_t max_file_size  The max size of the log file,
 *                                  if over this size, the file will be cut.
 *                                  this parameter should be smaller than 2G,
 *                                  if bigger than 2G, 2G will be used.
 * Return Value:
 *          0 for success
 *          others for failure
 */
int log_init(const char *path, const char *base_name, const int level,
         const int cut_type = CUT_BY_DAY, 
         const uint32_t max_file_size = ((2 * 1024 * 1024 * 1024U) - 1));

/*
 * Function: Set global level of message printing. user can use this
 *           to configure the print level.
 * Arguments:
 *          int level: (L_DEBUG, L_NOTICE, L_WARNING or L_ERROR)
 * Return Value:
 *          N/A
 */
void log_set_level(int level);

/*
 * Function: Write a log to the log file, if log_init is not called yet,
 *           the log would be printed to stderr.
 * Arguments:
 *          int level:          The level of this log
 *                              (L_DEBUG, L_NOTICE, L_WARNING or L_ERROR)
 *          char *file_name     Used for __FILE__
 *          int  line           Used for __LINE__
 *          const char *format: Format of the log
 * Return Value:
 *          0 for success
 *          others for failure
 */
int log_print(const int level, const char* file_name, const int line, const char* format, ...);

/*
 * Function: Just store a log, without printing.
 *          the stored logs will be printed by log_flush_put
 * Arguments:
 *          int level:          The level of this log, as that in log_print
 *          const char *format: Same with format in log_print
 * Return Value:
 *          0 for success
 *          others for failure
 */
int log_put(const int level, const char *format, ...);

/*
 * Function: Write the stored logs into logfile
 * Arguments:
 *          char *file_name     Used for __FILE__
 *          int  line           Used for __LINE__
 * Return Value:
 *          0 for success
 *          others for failure
 */
int log_flush_put(const char *file_name, const int line);

/*
 * Function: Release related sources for log file
 * Arguments:
 *          N/A
 * Return Value:
 *          0 for success
 *          others for failure
 */
int log_destroy();


#endif
