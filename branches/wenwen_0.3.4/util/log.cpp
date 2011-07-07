#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <limits.h>
#include <time.h>
#include "log.h"


#define COMMON_LOG  0
#define ERROR_LOG   1

#define END_LINE "\r\n"

#define ALLOC_SIZE 1024

#define NO_DIR  -1
#define NOT_DIR -2

#define NOT_INIT    0
#define INIT_FAILED 1
#define INITED      2

namespace _log_global {
    const char * level_str[] = {
        "NOLOG", "ERROR", "WARNING", "NOTICE", "DEBUG"
    };

    FILE *g_fp = NULL;
    FILE *g_err_fp = NULL;
    char g_path_prefix[PATH_MAX] = {0};
    int g_inited = NOT_INIT;

    //initialized as L_NOTICE, for the use of printing to stderr
    //when print before log_init
    int g_log_level = L_NOTICE;
    int g_cut_type = CUT_BY_DAY;
    uint32_t g_max_file_size = (1024U * 1024 * 1024 * 2) - 1;
    int g_hour = 0;
    int g_day = 0;
    pthread_mutex_t g_mlock;

    __thread char *t_put_buf = NULL;
};

using namespace _log_global;

static int verify_log_dir(const char* path)
{
    struct stat st;
    if (stat(path, &st) == -1)
    {
        return NO_DIR;
    }

    if (!S_ISDIR(st.st_mode))
    {
        return NOT_DIR;
    }

    return 0;
}

static int log_makedir_recursive(const char *dir)
{
    int ret = 0;
    char tmp[PATH_MAX] = {0};
    char dir_creating[PATH_MAX] = {0};
    char *dir_layer = NULL;
    char *cur_layer = NULL;

    if ((dir == NULL) || (strlen(dir) == 0)) {
        return 0;
    }

    strncpy(tmp, dir, (PATH_MAX - 1));
    cur_layer = tmp;
    if (cur_layer[0] == '/') {
        strcpy(dir_creating, "/");
        cur_layer += 1;
    }

    do {
        //get the next layer
        if (cur_layer[0] == '/')
            dir_layer = strchr(cur_layer + 1, '/');
        else
            dir_layer = strchr(cur_layer, '/');

        if (dir_layer) {
            *dir_layer = '\0';
            strncat(dir_creating, cur_layer, (PATH_MAX - strlen(dir_creating) - 1));
            ret = verify_log_dir(dir_creating);
            if (ret == NO_DIR) {
                if (mkdir(dir_creating, 0755) != 0) {  //create the new gotten layer
                    return -1;
                }
            } else if (ret == NOT_DIR){
                return -1;
            }
            strncat(dir_creating, "/", (PATH_MAX - strlen(dir_creating) - 1));
            cur_layer = dir_layer + 1;  //set the pointer to next layer
        } else {
            strncat(dir_creating, cur_layer, (PATH_MAX - strlen(dir_creating) - 1));
            ret = verify_log_dir(dir_creating);
            if (ret == NO_DIR) {
                if (mkdir(dir_creating, 0755) != 0) {
                    return -1;
                }
            } else if (ret == NOT_DIR) {
                return -1;
            }
        }
    } while (dir_layer);

    return 0;
}

static void log_make_name(int sequence, int type, char *result)
{
    char   data_str[32] = {0};
    time_t m_time = time(NULL);
    struct tm tm;
    localtime_r(&m_time, &tm);

    g_hour = tm.tm_hour;
    g_day = tm.tm_mday;

    snprintf(data_str, sizeof(data_str), 
            "%04d%02d%02d_%02d:%02d",
            tm.tm_year + 1900, tm.tm_mon + 1, 
            tm.tm_mday, tm.tm_hour,
            tm.tm_min);
    strncpy(result, data_str, sizeof(data_str));

    //deal with same file name
    if (sequence > 0) {
        memset(data_str, 0, sizeof(data_str));
        snprintf(data_str, sizeof(data_str), "_%d", sequence);
        strncat(result, data_str, sizeof(data_str));
    }
}


static void log_init_lock()
{
    pthread_mutex_init(&g_mlock, NULL);
}

static void log_lock()
{
    pthread_mutex_lock(&g_mlock);
}

static void log_unlock()
{
    pthread_mutex_unlock(&g_mlock);
}

static void log_destroy_lock()
{
    pthread_mutex_destroy(&g_mlock);
}

static int log_open(int type)
{
    char full_path[PATH_MAX] = {0};

    strncpy(full_path, g_path_prefix, (PATH_MAX - 1));

    if (type == COMMON_LOG) {
        strncat(full_path, ".log", (PATH_MAX - strlen(full_path) - 1));
        g_fp = fopen(full_path, "a");
        if (!g_fp) {
            return -1;
        }
    } else if (type == ERROR_LOG) {
        strncat(full_path, ".err.log", (PATH_MAX - strlen(full_path) - 1));
        g_err_fp = fopen(full_path, "a");
        if (!g_err_fp) {
            return -1;
        }
    }

    return 0;
}

static int log_cut(int type)
{
    int i = 0;
    char log_name[NAME_MAX] = {0};
    char full_path[PATH_MAX] = {0};
    char old_name[PATH_MAX] = {0};

    if (type == COMMON_LOG) {
        fclose(g_fp);
        g_fp = NULL;
    } else {
        fclose(g_err_fp);
        g_err_fp = NULL;
    }

    //generate name for the log to be cut
    strncpy(full_path, g_path_prefix, (PATH_MAX - 1));
    if (type == COMMON_LOG) {
        strncat(full_path, ".log.", (PATH_MAX - strlen(old_name) - 1));
    } else {
        strncat(full_path, ".err.log.", (PATH_MAX - strlen(old_name) - 1));
    }
    log_make_name(0, type, log_name);
    strncat(full_path, log_name, (PATH_MAX - strlen(full_path) - 1));

    //check whether the file exists or not
    while (access(full_path, F_OK) == 0) {
        i++;
        memset(full_path, 0, sizeof(full_path));
        memset(log_name, 0, sizeof(log_name));

        strncpy(full_path, g_path_prefix, (PATH_MAX - 1));
        if (type == COMMON_LOG) {
            strncat(full_path, ".log.", (PATH_MAX - strlen(old_name) - 1));
        } else {
            strncat(full_path, ".err.log.", (PATH_MAX - strlen(old_name) - 1));
        }
        log_make_name(i, type, log_name);
        strncat(full_path, log_name, (PATH_MAX - strlen(full_path) - 1));
    }

    strncpy(old_name, g_path_prefix, (PATH_MAX - 1));
    if (type == COMMON_LOG) {
        strncat(old_name, ".log", (PATH_MAX - strlen(old_name) - 1));
    } else {
        strncat(old_name, ".err.log", (PATH_MAX - strlen(old_name) - 1));
    }

    rename(old_name, full_path);

    return log_open(type);
}

static void log_init_time_attr()
{
    struct tm tm;
    time_t m_time = time(NULL);
    localtime_r(&m_time, &tm);
    g_hour = tm.tm_hour;
    g_day = tm.tm_mday;
}

static int log_check_time_cut()
{
    int ret = 0;
    struct tm tm;
    time_t m_time = time(NULL);
    localtime_r(&m_time, &tm);
    int m_hour = tm.tm_hour;
    int m_day = tm.tm_mday;

    if (((g_cut_type == CUT_BY_DAY) && (m_day != g_day)) ||
            ((g_cut_type == CUT_BY_HOUR) && (m_hour != g_hour))) {

        log_lock();

        //get the time again to avoid cutting by multiple threads
        m_time = time(NULL);
        localtime_r(&m_time, &tm);
        m_hour = tm.tm_hour;
        m_day = tm.tm_mday;

        if (((g_cut_type == CUT_BY_DAY) && (m_day != g_day)) ||
                ((g_cut_type == CUT_BY_HOUR) && (m_hour != g_hour))) {
            ret = log_cut(COMMON_LOG);
            if (ret != 0) {
                log_unlock();
                return ret;
            }
            ret = log_cut(ERROR_LOG);
            if (ret != 0) {
                log_unlock();
                return ret;
            }

            g_hour = m_hour;
            g_day = m_day;
        }

        log_unlock();
    }

    return 0;
}

static int log_check_size_cut(int type, int in_size)
{
    int ret = 0;
    uint32_t m_size = 0;
    FILE *fp = NULL;

    if (type == COMMON_LOG) {
        fp = g_fp;
    } else {
        fp = g_err_fp;
    }

    m_size = ftello(fp);

    if ((m_size + in_size) >= g_max_file_size) {
        ret = log_cut(COMMON_LOG);
        if (ret != 0) {
            return ret;
        }
        ret = log_cut(ERROR_LOG);
        if (ret != 0) {
            return ret;
        }
    }

    return 0;
}

int log_init(const char *path, const char* base_name, const int level,
        const int cut_type, const uint32_t max_file_size)
{
    int ret = 0;

    log_set_level(level); //check error

    if (path != NULL) {
        ret = log_makedir_recursive(path);
        if (ret != 0) {
            g_inited = INIT_FAILED;
            return -1;
        }
        strncpy(g_path_prefix, path, (PATH_MAX - 1));
        strncat(g_path_prefix, "/", (PATH_MAX - strlen(g_path_prefix) -1));
    }

    if (base_name != NULL) {
        strncat(g_path_prefix, base_name, (PATH_MAX - strlen(g_path_prefix) - 1));
    } else {
        strncat(g_path_prefix, level_str[g_log_level], (PATH_MAX - strlen(g_path_prefix) - 1));
    }

    if ((cut_type != CUT_BY_HOUR) && (cut_type != CUT_BY_DAY)) {
        g_cut_type = CUT_BY_DAY;
    } else {
        g_cut_type = cut_type;
    }

    log_init_time_attr();

    if ((max_file_size > 0) && (max_file_size < g_max_file_size))
        g_max_file_size = max_file_size;

    ret = log_open(COMMON_LOG);
    if (ret != 0) {
        g_inited = INIT_FAILED;
        return ret;
    }

    ret = log_open(ERROR_LOG);
    if (ret != 0) {
        g_inited = INIT_FAILED;
        return ret;
    }

    log_init_lock();

    g_inited = INITED;

    return 0;
}

void log_set_level(int level)
{
    if (level < L_NOLOG)
        level = L_NOLOG;
    if (level > L_DEBUG)
        level = L_DEBUG;

    g_log_level = level;
}

int log_print(const int level, const char* file_name, 
        const int line, const char* format, ...)
{
    int ret = 0;
    struct timeval tm_v;
    struct tm tm;
    FILE *t_fp = NULL;
    va_list va;
    char t_str[128] = {0};
    int str_size = 0;

    if ((format == NULL) || (level < L_NOLOG))
        return -1;

    if (g_inited == INIT_FAILED)
        return -1;

    if (level > g_log_level)
        return 0;


    if (g_inited == NOT_INIT) {
        gettimeofday(&tm_v, NULL);
        localtime_r(&tm_v.tv_sec, &tm);

        fprintf(stderr, "%04d-%02d-%02d %02d:%02d:%02d.%06d %s %ld %s:%d ",
                tm.tm_year + 1900, tm.tm_mon + 1,
                tm.tm_mday, tm.tm_hour,
                tm.tm_min, tm.tm_sec, (int)tm_v.tv_usec,
                level_str[g_log_level], syscall(SYS_gettid), file_name, line);
        va_start(va, format);
        vfprintf(stderr, format, va);
        va_end(va);
        fprintf(stderr, "%s", END_LINE);
        return 0;
    }

    ret = log_check_time_cut();
    if (ret != 0) {
        return ret;
    }

    va_start(va, format);
    str_size = vsnprintf(t_str, 128, format, va);
    va_end(va);

    log_lock();

    if ((level == L_ERROR) || (level == L_WARNING)) {
        ret = log_check_size_cut(ERROR_LOG, str_size);
        if (ret != 0) {
            log_unlock();
            return ret;
        }
    } else {
        ret = log_check_size_cut(COMMON_LOG, str_size);
        if (ret != 0) {
            log_unlock();
            return ret;
        }
    }

    if ((level == L_ERROR) || (level == L_WARNING)) {
        t_fp = g_err_fp;
    } else {
        t_fp = g_fp;
    }

    gettimeofday(&tm_v, NULL);
    localtime_r(&tm_v.tv_sec, &tm);

    fprintf(t_fp, "%04d-%02d-%02d %02d:%02d:%02d.%06d %s %ld %s:%d ",
            tm.tm_year + 1900, tm.tm_mon + 1,
            tm.tm_mday, tm.tm_hour,
            tm.tm_min, tm.tm_sec, (int)tm_v.tv_usec,
            level_str[level], syscall(SYS_gettid), file_name, line);
    va_start(va, format);
    vfprintf(t_fp, format, va);
    va_end(va);
    fprintf(t_fp, "%s", END_LINE);

    log_unlock();

    return 0;
}

int log_put(const int level, const char *format, ...)
{
    int str_sz = 0;
    int total_size = 0;
    int total_allc_cnt = 0;
    char fmt_str[ALLOC_SIZE] = {0};
    va_list va;

    if ((format == NULL) || (level < L_NOLOG))
        return -1;

    if (g_inited == INIT_FAILED)
        return -1;

    if (level > g_log_level)
        return 0;


    //allocat the buffer at the first time
    if (t_put_buf == NULL) {
        t_put_buf = (char *)malloc(ALLOC_SIZE);
        memset(t_put_buf, 0 , ALLOC_SIZE);
    }

    //get the length of the message
    va_start(va, format);
    str_sz = vsnprintf(fmt_str, ALLOC_SIZE, format, va);
    va_end(va);

    total_size = strlen(t_put_buf);
    total_allc_cnt = (total_size/ALLOC_SIZE) + 1;
    if((total_size + str_sz) >= (total_allc_cnt * ALLOC_SIZE)) {
        //if buffer is not big enough, reallocate it
        total_allc_cnt += (str_sz/ALLOC_SIZE) + 1;
        t_put_buf = (char *)realloc(t_put_buf, (ALLOC_SIZE * total_allc_cnt));
    }

    if (str_sz < ALLOC_SIZE) {
        //fmt_str is enough for the message
        strncat(t_put_buf, fmt_str, str_sz);
    } else {
        //allocate a tmp buffer to store the message
        va_start(va, format);
        vsnprintf(t_put_buf + total_size, str_sz, format, va);
        va_end(va);
    }

    return 0;
}

//NOTE: log_flush_put is a must after log_put
//or memory leak will happen
int log_flush_put(const char *file_name, const int line)
{
    if (t_put_buf == NULL)
        return -1;

    if (g_inited == INIT_FAILED)
        return -1;

    //the level of put is controled in log_put
    //so here we set the level to global level
    log_print(g_log_level, file_name, line, "%s", t_put_buf);

    if (t_put_buf) {
        free(t_put_buf);
        t_put_buf = NULL;
    }

    return 0;
}

int log_destroy()
{
    if (g_fp) {
        fclose(g_fp);
        g_fp = NULL;
    }
    if (g_err_fp) {
        fclose(g_err_fp);
        g_err_fp = NULL;
    }

    log_destroy_lock();

    return 0;
}


#ifdef _TEST_
static void *thread_fn(void *arg)
{
    //test log put
    int i = 0;
    while (i < 100) {
        PUT_ERROR("%lu: i is %d\n", pthread_self(), i);
        if (i % 10 == 0) {
            FLUSH_PUT();
            if ((t_head == NULL) && (t_tail == NULL)) {
                NOTICE("---------------%lu-----------------", pthread_self());
            }
        }
        i++;
    }
}

int main()
{
    int i = 0;
    int n = 10;
    pthread_t tids[10];

    log_init(NULL, NULL, 4, 1);

#if 0
    //cut by time
    while(i < 500) {
        ERROR("testing the log input %s, %d\n", "haha", i);
        i++;
        sleep(1);
    }
#endif

#if 0
    //cut by file size
    //need to modify the definition of MAX_FILE_SIZE and recompile
    i = 0;
    while(i < 10) {
        ERROR("testing the log cutting by file size, %s, %d\n", "haha", i);
        i++;
    }
#endif


    //test for multithread
    for (i = 0; i < n; i++) {
        pthread_create(&tids[i], NULL, thread_fn, NULL);
    }

    for (i = 0; i < n; i++) {
        pthread_join(tids[i], NULL);
    }

    log_destroy();
    return 0;
}
#endif
