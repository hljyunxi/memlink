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

static void*
sthread_loop(void *arg) 
{
    SThread *st = (SThread*) arg;
    DINFO("sthread_loop...\n");
    event_base_loop(st->base, 0);
    return NULL;
}

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

static off_t
lseek_or_exit(int fd, off_t offset, int whence) 
{
    off_t pos;
    if ((pos = lseek(fd, offset, whence)) == -1) {
        DERROR("lseek error: {fd: %d, offset: %u, whence: %d}! %s\n", fd, 
                (unsigned int)offset, whence, strerror(errno));
        close(fd);
        MEMLINK_EXIT;    
    }
    return pos;
}

static off_t
lseek_set(int fd, off_t offset)
{
    return lseek_or_exit(fd, offset, SEEK_SET);
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
    conn->sync_fd = open(conn->log_name, O_RDONLY);
    if (conn->sync_fd == -1) {
        DERROR("open file %s error! %s\n", conn->log_name, strerror(errno));
        MEMLINK_EXIT;
    }
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
    if (read(fd, &integer, sizeof(int)) == -1) {
        DERROR("read error! %s\n", strerror(errno));
        close(fd);
        MEMLINK_EXIT;    
    }
    return integer;
}

/**
 * Return synclog record position. A 0 value of record position means that the 
 * record does not exist.
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

    conn->sync_fd = open(conn->log_name, O_RDONLY);
    if (conn->sync_fd == -1) {
        DERROR("open file %s error! %s\n", conn->log_name, strerror(errno));
        MEMLINK_EXIT;
    }

    conn->log_index_pos = get_index_pos(log_no);
    unsigned int log_pos = seek_and_read_int(conn->sync_fd, conn->log_index_pos);
    if (log_pos > 0) {
        DINFO("log record %u is at %u\n", log_no, log_pos);
    } else {
        DERROR("log record %u does not exist in log file %s\n", log_no, conn->log_name);
    }
    return log_pos;
}

static void
reset_interval(struct timeval *tv_ptr, unsigned int seconds)
{
    evutil_timerclear(tv_ptr);
    tv_ptr->tv_sec = seconds;
}

/**
 * Read n bytes. Terminate program if error.
 */
static int
try_readn(int fd, void *ptr, size_t n)
{
    int ret;
    if ((ret = readn(fd, ptr, n, 0)) < 0)
        MEMLINK_EXIT;
     else
        return ret;
}

/**
 * Write n bytes with a timeout. Terminate program if writing error or n bytes 
 * can't all be written. 
 */
static void 
writen_or_destroy(SyncConn *conn, void *ptr, size_t n)
{
    int ret;
    if ((ret = writen(conn->sock, ptr, n, g_cf->timeout)) != n) {
        DERROR("Unable to write %d bytes, only %d bytes are written.\n", (int)n, ret);
        sync_conn_destroy((Conn*)conn);
    }
}

/**
 * Read n bytes from the synclog file and write them to sync connection socket.
 */
static void 
transfer(SyncConn *conn,  void *ptr, size_t n)
{
    int ret;
    if ((ret = try_readn(conn->sync_fd, ptr, n)) != n) {
        DERROR("Unable to read %d bytes, only %d bytes are read.\n", (int)n, ret);
        MEMLINK_EXIT;
    }
    writen_or_destroy(conn, ptr, n);
}


/**
 * Send available synclog records in the current open synclog file in 
 * SyncConn.
 */
static void
send_synclog_record(SyncConn *conn)
{
    unsigned int log_pos;
    unsigned int master_log_ver;
    unsigned int master_log_pos;

    unsigned short len;
    int buf_len = 256;
    char buf[buf_len];
    char p_buf[buf_len];

    DINFO("sending sync log in %s\n", conn->log_name);
    while (conn->log_index_pos < SYNCLOG_MAX_INDEX_POS 
            && (log_pos = seek_and_read_int(conn->sync_fd, conn->log_index_pos)) > 0) {
        lseek_set(conn->sync_fd, log_pos);
        transfer(conn, &master_log_ver, sizeof(int)); // log version
        transfer(conn, &master_log_pos, sizeof(int)); // log position 
        transfer(conn, &len, sizeof(short)); // log record length

        // log record
        transfer(conn, buf, len);
        formath(buf, len, p_buf, buf_len);

        DINFO("sent {log version %u, log position: %u, log record (%u): %s}\n", 
                master_log_ver, master_log_pos, len, p_buf);

        conn->log_index_pos += sizeof(int);
    }
}

static void
send_synclog(int fd, short event, void *arg) 
{
    DINFO("sending sync log...\n");
    SyncConn *conn = arg;
    /*
     * Send history synclog files and open the current synclog file.
     */
    while (conn->log_ver < g_runtime->logver) {
        send_synclog_record(conn);
        (conn->log_ver)++;
        conn->log_index_pos = get_index_pos(0);
        open_synclog(conn);
    }

    /*
     * Send available records in the current synclog file
     */
    send_synclog_record(conn);

    reset_interval(&(conn->interval), g_cf->timeout);
    event_add(&conn->sync_evt, &(conn->interval));
}

static void 
send_synclog_init(SyncConn* conn) 
{
    evtimer_set(&conn->sync_evt, send_synclog, conn);
    event_base_set(conn->base, &conn->sync_evt);
    reset_interval(&(conn->interval), 0);
    event_add(&conn->sync_evt, &(conn->interval));
}

static int
cmd_sync(SyncConn *conn, char *data, int datalen) 
{
    int ret;
    unsigned int log_no;
    cmd_sync_unpack(data, &conn->log_ver, &log_no);
    DINFO("log version: %u, log record number: %u\n", conn->log_ver, log_no);
    if ((get_synclog_record_pos(conn, log_no)) > 0
            || (conn->log_ver == g_runtime->logver && log_no == 0)) {
        ret = data_reply((Conn*)conn, 0, NULL, 0);
        DINFO("Found sync log file (version = %u)\n", conn->log_ver);
        send_synclog_init(conn);
    } else { 
        ret = data_reply((Conn*)conn, 1, NULL, 0);
        close(conn->sync_fd);
        DINFO("Not found syn log file (version %u) having log record %d\n", conn->log_ver, log_no);
    }
    return ret;
}

static int 
open_dump()
{
    char dump_filename[PATH_MAX];
    snprintf(dump_filename, PATH_MAX, "%s/dump.dat", g_cf->datadir);
    int fd;
    if ((fd = open(dump_filename, O_RDONLY)) == -1) {
        DERROR("open %s error! %s\n", dump_filename, strerror(errno));
        MEMLINK_EXIT;
    }
    return fd;
}

static off_t
get_file_size(int fd)
{
    return lseek_or_exit(fd, 0, SEEK_END);
}

static void 
send_dump(int fd, short event, void *arg)
{
    SyncConn* conn = arg;
    char buf[BUF_SIZE];
    int p_buf_size = BUF_SIZE * 3;
    char p_buf[p_buf_size];
    int ret;
    /*
     * The current log version may be changed if dump event happends. So it is 
     * saved here.
     */
    conn->log_ver = g_runtime->logver;

    DINFO("sending dump...\n");
    while ((ret = try_readn(conn->sync_fd, buf, BUF_SIZE)) > 0) {
        DINFO("dump data: %s\n", formath(buf, ret, p_buf, p_buf_size));
        writen_or_destroy(conn, buf, ret);
    }
    // close dump file
    close(conn->sync_fd);

    conn->log_index_pos = get_index_pos(0);

    open_synclog(conn);
    send_synclog_init(conn); 
}

static void 
send_dump_init(SyncConn *conn)
{
    evtimer_set(&conn->sync_evt, send_dump, conn);
    event_base_set(conn->base, &conn->sync_evt);
    reset_interval(&(conn->interval), 0);
    event_add(&conn->sync_evt, &(conn->interval));
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

    conn->sync_fd = open_dump(); 
    file_size = get_file_size(conn->sync_fd);
    check_dump_ver_and_size(dumpver, transferred_size, file_size, &retcode, &offset);
    remaining_size = file_size - offset; 

    int retlen = sizeof(int) + sizeof(long long);
    char retrc[retlen];
    memcpy(retrc, &g_runtime->dumpver, sizeof(int));
    memcpy(retrc + sizeof(int), &remaining_size, sizeof(long long));
    ret = data_reply((Conn*)conn, retcode, retrc, retlen);

    if (offset < file_size) {
        lseek_set(conn->sync_fd, offset);
        send_dump_init(conn);
    }
    return ret;
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
    event_add(&st->event, 0);

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
    event_del(&conn->sync_evt);
    event_del(&conn->evt);
    close(conn->sock);
    if (conn->sync_fd >= 0)
        close(conn->sync_fd);
    zz_free(conn);
}
