#include <stdlib.h>
#include <network.h>
#include <string.h>
#include <errno.h>
#include <dirent.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "sthread.h"
#include "wthread.h"
#include "logfile.h"
#include "myconfig.h"
#include "zzmalloc.h"
#include "utils.h"
#include "common.h"

#define BUF_SIZE 4096
#define SYNCLOG_MAX_INDEX_POS (get_index_pos(SYNCLOG_INDEXNUM))
#define RECORD_HEAD_LEN (sizeof(int) * 2 + sizeof(short))
#define RECORD_LEN_POS  (sizeof(int) * 2)

/** read n bytes, exit if error */
static int
try_read(int fd, void *ptr, size_t n)
{
    int ret;
    if ((ret = read(fd, ptr, n)) == -1) {
        DERROR("file %d read error! %s\n", fd, strerror(errno));
        MEMLINK_EXIT;
    } else
        return ret;
}

/** read, exit if error or n bytes can't be read */
static void
read_or_exit(int fd, void *ptr, size_t n)
{
    int ret;
    if ((ret = read(fd, ptr, n)) != n) {
        DERROR("Unable to read %d bytes, only %d bytes are read from file %d.\n", (int)n, ret, fd);
        MEMLINK_EXIT;
    }
}

/** open readonly, exit if error */
static int 
open_or_exit(char *path)
{
    int fd;
    if ((fd = open(path, O_RDONLY)) == -1) {
        DERROR("open %s error! %s\n", path, strerror(errno));
        MEMLINK_EXIT;
    }
    return fd;
}

/** seek, exit if error */
static off_t
lseek_or_exit(int fd, off_t offset, int whence) 
{
    off_t pos;
    if ((pos = lseek(fd, offset, whence)) == -1) {
        DERROR("lseek error: {fd: %d, offset: %u, whence: %d}! %s\n", fd, 
                (unsigned int)offset, whence, strerror(errno));
        MEMLINK_EXIT;    
    }
    return pos;
}

/** seek with SEEK_SET, exit if error */
static off_t
lseek_set(int fd, off_t offset)
{
    return lseek_or_exit(fd, offset, SEEK_SET);
}

/** thread event loop */
static void*
sthread_loop(void *arg) 
{
    SThread *st = (SThread*) arg;
    DINFO("sthread_loop...\n");
    event_base_loop(st->base, 0);
    return NULL;
}

/** create sthread's thread */
static SThread* 
create_thread(SThread* st) 
{
    pthread_t threadid;
    pthread_attr_t attr;
    int ret;

    ret = pthread_attr_init(&attr);
    if (ret != 0) {
        DERROR("pthread_attr_init error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }
    ret = pthread_create(&threadid, &attr, sthread_loop, st);
    if (ret != 0) {
        DERROR("pthread_create error: %s\n", strerror(errno));
        MEMLINK_EXIT;
    }
    DINFO("create sync thread ok\n");
    return st;
}

static void 
close_if_open(SyncConn *conn)
{
    if (conn->sync_fd >= 0)
        close(conn->sync_fd);
    conn->sync_fd = -1;
}

/**
 * Return the synclog file pathname for the given synclog version.
 * 
 * @param conn->log_ver synclog version
 * @param conn->log_name return the synclog file pathname
 * @return 1 if the synclog file exists, 0 otherwise.
 */
static int 
get_synclog_filename(SyncConn *conn)
{
    if (conn->log_ver <= 0) {
        DINFO("Invalid log version: %u. Log version must be a positive number.\n", conn->log_ver);
        return 0;
    }

    /*
     * The following steps when sync log is rotated:
     *
     * 1. Rename bin.log(xxx) to xxx.log
     * 2. Create a new bin.log(xxx+1)
     * 3. Update g_runtime->logver to xxx+1
     *
     * The following list shows a example of sync log rotation. State 1 show 
     * the initial state. State 4 is the final state after the rotation. State 
     * format is g_runtime->logver followed by sync log file names.
     *
     * 1. 003: 001.log 002.log bin.log
     * 2. 003: 001.log 002.log 003.log
     * 3. 003: 001.log 002.log 003.log bin.log
     * 4. 004: 001.log 002.log 003.log bin.log
     */
    if (conn->log_ver < g_runtime->logver) {
        /*
         * If conn->log_ver < g_runtime->log_ver, <conn->log_ver>.log must 
         * exist if not deleted by users.
         */
        snprintf(conn->log_name, PATH_MAX, "%s/data/bin.log.%u", g_runtime->home, conn->log_ver);
        return isfile(conn->log_name);
    } else if (conn->log_ver == g_runtime->logver) {
        /*
         * If conn->log_ver == g_runtime->log_ver, there are 2 siutions. Return 
         * <conn->log_ver>.log if it exists. This happens for state 2 and state 
         * 3.  Otherwise, return bin.log. This happends for state 1.
         */
        snprintf(conn->log_name, PATH_MAX, "%s/data/bin.log.%u", g_runtime->home, conn->log_ver);
        if (isfile(conn->log_name) != 1) {
            snprintf(conn->log_name, PATH_MAX, "%s/data/bin.log", g_runtime->home);
            return isfile(conn->log_name);
        } else {
            return 1;
        }
    } else {
        return 0;
    }
}

/**
 * Open the synclog for the given synclog version.
 *
 * @param conn->log_ver synclog version
 * @param conn->log_name return the sync log file pathname
 * @param conn->sync_fd return the opened file descriptor
 */
static void 
open_synclog(SyncConn *conn) 
{
    if (get_synclog_filename(conn) != 1) {
        DERROR("sync log with version %u does not exist\n", conn->log_ver);
        MEMLINK_EXIT;
    }
    close_if_open(conn);
    conn->sync_fd = open_or_exit(conn->log_name);
}

/**
 * Return the log index position for the given log record number.
 * 
 * @param log_no log record number
 * @return log record index position
 */
inline static unsigned int
get_index_pos(unsigned int log_no)
{
    return SYNCLOG_HEAD_LEN + log_no * sizeof(int);
}

/**
 * Seek to the given offset and read an int.
 */
static unsigned int
seek_and_read_int(int fd, unsigned int offset)
{
    unsigned int integer;
    lseek_set(fd, offset);
    read_or_exit(fd, &integer, sizeof(int));
    return integer;
}

/**
 * Return synclog record position. 0 means that the record does not exist.
 */
static int
get_synclog_record_pos(SyncConn *conn, unsigned int log_no)
{
    if (log_no >= SYNCLOG_INDEXNUM) {
        DERROR("log no %u is not less than SYNCLOG_INDEXNUM %u\n", log_no, SYNCLOG_INDEXNUM);
        return 0;
    }
    if (get_synclog_filename(conn) != 1)
        return 0;

    conn->sync_fd = open_or_exit(conn->log_name);
    conn->log_index_pos = get_index_pos(log_no);
    unsigned int log_pos = seek_and_read_int(conn->sync_fd, conn->log_index_pos);
    if (log_pos > 0) {
        DINFO("log record %u is at %u\n", log_no, log_pos);
    } else {
        DERROR("log record %u does not exist in log file %s\n", log_no, conn->log_name);
    }
    return log_pos;
}

static int
cmd_sync(SyncConn *conn, char *data, int datalen) 
{
    int ret;
    unsigned int log_no;

    cmd_sync_unpack(data, &conn->log_ver, &log_no);
    DINFO("log version: %u, log record number: %u\n", conn->log_ver, log_no);
    if (get_synclog_record_pos(conn, log_no) > 0
            || (conn->log_ver == g_runtime->logver && log_no == 0)) {
        conn->status = SEND_LOG;
        ret = data_reply((Conn*)conn, 0, NULL, 0);
        DINFO("Found sync log file (version = %u)\n", conn->log_ver);
    } else { 
        conn->status = NOT_SEND;
        ret = data_reply((Conn*)conn, 1, NULL, 0);
        DINFO("Not found syn log file (version %u) having log record %d\n", conn->log_ver, log_no);
    }
    return ret;
}

static int 
open_dump()
{
    char dump_filename[PATH_MAX];
    snprintf(dump_filename, PATH_MAX, "%s/dump.dat", g_cf->datadir);
    return open_or_exit(dump_filename);
}

static off_t
get_file_size(int fd)
{
    return lseek_or_exit(fd, 0, SEEK_END);
}

static void 
reset_conn_wbuf(SyncConn *conn, char *wbuf, int wlen) 
{
    conn->wbuf = wbuf;
    conn->wlen = wlen;
    conn->wpos = 0;
}

static void
clear_conn_wbuf(SyncConn *conn)
{
    if (conn->wbuf) {
        zz_free(conn->wbuf);
    }
    conn->wlen = conn->wpos = 0;
    conn->wbuf = NULL;
}

static void 
sync_write(int fd, short event, void *arg)
{
    SyncConn *conn = (SyncConn*) arg;
    if (event & EV_TIMEOUT) {
        DWARNING("write timeout:%d, close\n", fd);
        conn->destroy((Conn*)conn);
        return;
    }
    if (conn->wpos == conn->wlen) {
        clear_conn_wbuf(conn);
        DINFO("finished sending sync log in buffer\n");
        conn->fill_wbuf(0, 0, conn);
        return;
    }
    conn_write((Conn*)conn);
}

static void 
sync_read(int fd, short event, void *arg)
{
    SyncConn *conn = (SyncConn*) arg;
    int ret = read(conn->sync_fd, 0, 0);
    if (ret == 0) {
        DINFO("read 0, close conn %d.\n", fd);
        conn->destroy((Conn*)conn);
    }
}

static void
set_timeval(struct timeval *tm_ptr, unsigned int seconds) 
{
    evutil_timerclear(tm_ptr);
    tm_ptr->tv_sec = seconds;
}

static void
interval_event_init(SyncConn *conn)
{
    evtimer_set(&conn->sync_interval_evt, sync_write, conn);
    event_base_set(conn->base, &conn->sync_interval_evt);
    set_timeval(&conn->interval, g_cf->sync_interval);
}

static void
write_event_init(SyncConn *conn)
{
    event_set(&conn->sync_write_evt, conn->sock, EV_WRITE | EV_PERSIST, sync_write, conn);
    event_base_set(conn->base, &conn->sync_write_evt);
    set_timeval(&conn->timeout, g_cf->timeout);
}

static void
read_event_init(SyncConn *conn) 
{
    event_set(&conn->sync_read_evt, conn->sock, EV_READ | EV_PERSIST, sync_read, conn);
    event_base_set(conn->base, &conn->sync_read_evt);
}

static void
read_record(SyncConn *conn, 
            unsigned int log_pos, 
            char **wbuf_ptr, 
            unsigned short *wlen_ptr)
{
    unsigned short len;
    char head_buf[RECORD_HEAD_LEN] = {0};

    lseek_set(conn->sync_fd, log_pos);
    read_or_exit(conn->sync_fd, head_buf, RECORD_HEAD_LEN);

    len = *((char *)(head_buf + RECORD_LEN_POS));
    *wlen_ptr = RECORD_HEAD_LEN + len;
    *wbuf_ptr = zz_malloc(*wlen_ptr);

    memcpy(*wbuf_ptr, head_buf, RECORD_HEAD_LEN);
    read_or_exit(conn->sync_fd, (void *)(*wlen_ptr + RECORD_HEAD_LEN), len);

    DINFO("read synclog record at %u\n",  log_pos);
}


/**
 * Return a log record from the open sync log file.
 * @return 1 if such a log record can be read. Otherwise 0.
 */
static int
get_synclog_record(SyncConn *conn)
{
    unsigned int log_pos;
    unsigned short wlen;
    char *wbuf;
    DINFO("trying to read a sync log record in %s...\n", conn->log_name);
    if (conn->log_index_pos < SYNCLOG_MAX_INDEX_POS 
            && (log_pos = seek_and_read_int(conn->sync_fd, conn->log_index_pos)) > 0) {
        read_record(conn, log_pos, &wbuf, &wlen);
        conn->log_index_pos += sizeof(int);
        reset_conn_wbuf(conn, wbuf, wlen);
        return 1;
    } else {
        return 0;
    }
}

static void
read_synclog(int fd, short event, void *arg) 
{
    SyncConn *conn = arg;
    DINFO("reading sync log...\n");
    while (!get_synclog_record(conn)) {
        if (conn->log_ver < g_runtime->logver) {
            (conn->log_ver)++;
            conn->log_index_pos = get_index_pos(0);
            open_synclog(conn);
        } else {
            if (event == EV_WRITE) 
                event_del(&conn->sync_write_evt);
            evtimer_add(&conn->sync_interval_evt, &conn->interval);
            return;
        }
    } 
    if (event == EV_TIMEOUT) {
        event_add(&conn->sync_write_evt, &conn->timeout);
    }
}

static void
read_synclog_init(SyncConn *conn)
{
    conn->fill_wbuf = read_synclog;
    interval_event_init(conn);
    read_synclog(0, 0, conn);
}

static void
read_dump(int fd, short event, void *arg)
{
    SyncConn *conn = (SyncConn*) arg;
    char buf[BUF_SIZE];
    int ret;

    DINFO("reading dump...");
    ret = try_read(conn->sync_fd, buf, BUF_SIZE);
    if (ret > 0) {
        reset_conn_wbuf(conn, buf, ret);
    } else if (ret == 0) {
        conn->log_index_pos = get_index_pos(0);
        open_synclog(conn);
        read_synclog_init(conn);
    } else {
        DERROR("read dump error! %s", strerror(errno));
        MEMLINK_EXIT;
    }
}

static void
read_dump_init(SyncConn *conn) 
{
    read_event_init(conn);
    event_add(&conn->sync_read_evt, 0); 
    conn->fill_wbuf = read_dump;
    read_dump(0, 0, conn);
}

static void 
common_event_init(SyncConn *conn) 
{
    event_del(&conn->evt);
    write_event_init(conn);
    read_event_init(conn);
}

static void
check_dump_ver_and_size(unsigned int dumpver, 
        unsigned long long transferred_size, 
        unsigned long long file_size,
        int *retcode, 
        unsigned long long *offset)
{
    if (g_runtime->dumpver == dumpver) {
        if (transferred_size <= file_size) {
            *retcode = CMD_GETDUMP_OK;
        } else {
            *retcode = CMD_GETDUMP_SIZE_ERR;
        }
    } else {
        *retcode = CMD_GETDUMP_CHANGE;
    }

    *offset = *retcode == CMD_GETDUMP_OK ? transferred_size : 0;
}

static int
cmd_get_dump(SyncConn* conn, char *data, int datalen)
{
    int ret;
    unsigned int dumpver;
    unsigned long long transferred_size;
    int retcode;
    unsigned long long offset;
    unsigned long long file_size;
    unsigned long long remaining_size;

    cmd_getdump_unpack(data, &dumpver, &transferred_size);
    DINFO("dump version: %u, synchronized data transferred size: %llu\n", dumpver, transferred_size);

    /*
     * The current log version may be changed if dump event happends. So it is 
     * saved here.
     */
    conn->log_ver = g_runtime->logver;
    conn->sync_fd = open_dump(); 
    file_size = get_file_size(conn->sync_fd);
    check_dump_ver_and_size(dumpver, transferred_size, file_size, &retcode, &offset);
    remaining_size = file_size - offset; 

    int retlen = sizeof(int) + sizeof(long long);
    char retrc[retlen];
    memcpy(retrc, &g_runtime->dumpver, sizeof(int));
    memcpy(retrc + sizeof(int), &remaining_size, sizeof(long long));
    if (offset < file_size) {
        conn->status = SEND_DUMP;
        lseek_set(conn->sync_fd, offset);
    } else {
        conn->status = NOT_SEND;
    }
    ret = data_reply((Conn*)conn, retcode, retrc, retlen);
    return ret;
}

static int 
sync_conn_wrote(Conn *c)
{
    SyncConn *conn = (SyncConn*) c;
    switch (conn->status) {
        case NOT_SEND:
            conn_wrote(c);
            break;
        case SEND_LOG:
            common_event_init(conn);
            read_synclog_init(conn);
            break;
        case SEND_DUMP:
            common_event_init(conn);
            read_dump_init(conn);
            break;
        default:
            DERROR("illegal sync connectoin status %d\n", conn->status);
            MEMLINK_EXIT;
    }
    return 0;
}

static void 
sthread_read(int fd, short event, void *arg) 
{
    SThread *st = (SThread*) arg;
    SyncConn *conn;
    int ret;

    DINFO("sthread_read...\n");
    conn = (SyncConn *)conn_create(fd, sizeof(SyncConn));

    if (conn) {
        conn->port = g_cf->sync_port;
        conn->base = st->base;
        conn->ready = sdata_ready;
        conn->destroy = sync_conn_destroy;
        conn->wrote = sync_conn_wrote;

        DINFO("new conn: %d\n", conn->sock);
        DINFO("change event to read.\n");
        ret = change_event((Conn*)conn, EV_READ | EV_PERSIST, 0, 1);
        if (ret < 0) {
            DERROR("change_event error: %d, close conn.\n", ret);
            sync_conn_destroy((Conn*)conn);
        }
    }
}

SThread*
sthread_create() 
{
    SThread *st = (SThread*)zz_malloc(sizeof(SThread));
    if (st == NULL) {
        DERROR("sthread malloc error.\n");
        MEMLINK_EXIT;
    }
    memset(st, 0, sizeof(SThread));

    st->sock = tcp_socket_server(g_cf->sync_port);
    if (st->sock == -1) 
        MEMLINK_EXIT;
    DINFO("sync thread socket creation ok!\n");

    st->base = event_base_new();
    event_set(&st->event, st->sock, EV_READ | EV_PERSIST, sthread_read, st);
    event_base_set(st->base, &st->event);
    struct timeval tm;
    set_timeval(&tm, g_cf->timeout);
    event_add(&st->event, &tm);

    g_runtime->sthread = st;

    return create_thread(st);
}

int
sdata_ready(Conn *c, char *data, int datalen) 
{
    int ret;
    char cmd;
    SyncConn *conn = (SyncConn*) c;
    conn->sync_fd = -1;

    memcpy(&cmd, data + sizeof(short), sizeof(char));
    char buf[256] = {0};
    DINFO("data ready cmd: %d, data: %s\n", cmd, formath(data, datalen, buf, 256));
    switch (cmd) {
        case CMD_SYNC:
            ret = cmd_sync(conn, data, datalen);
            break;
        case CMD_GETDUMP:
            ret = cmd_get_dump(conn, data, datalen);
            break;
        default:
            ret = MEMLINK_ERR_CLIENT_CMD;
            break;
    } 
    DINFO("data_reply return: %d\n", ret);
    return 0;
}

void sthread_destroy(SThread *st) 
{
    zz_free(st);
}

void 
sync_conn_destroy(Conn *c) 
{
    DINFO("destroy sync connection\n");
    SyncConn *conn = (SyncConn*) c;
    event_del(&conn->sync_interval_evt);
    event_del(&conn->sync_write_evt);
    event_del(&conn->evt);
    close(conn->sock);
    close_if_open(conn);
    zz_free(conn);
}
