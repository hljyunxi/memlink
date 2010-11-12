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

#define BUF_SIZE 1024
#define SYNCLOG_MAX_INDEX_POS (get_index_pos(SYNCLOG_INDEXNUM))

/**
 * Malloc. Terminate program if fails.
 */
static void* 
sthread_malloc(size_t size)
{
    void *ptr = zz_malloc(size);
    if (ptr == NULL) {
        DERROR("malloc error\n");
        MEMLINK_EXIT;
    }
    return ptr;
}

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

/**
 * Return the synclog file pathname for the given synclog version.
 * 
 * @param log_ver synclog version
 * @param log_name return the synclog file pathname
 */
static int 
get_synclog_filename(unsigned int log_ver, char *log_name)
{
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
	if (log_ver < g_runtime->logver) {
		/*
		 * If log_ver < g_runtime->log_ver, <log_ver>.log must exist.
		 */
		snprintf(log_name, PATH_MAX, "%s/data/bin.log.%u", g_runtime->home, log_ver);
        return 0;
	} else if (log_ver == g_runtime->logver) {
		/*
		 * If log_ver == g_runtime->log_ver, there are 2 siutions. Return 
		 * <log_ver>.log if it exists. This happens for state 2 and state 3.  
		 * Otherwise, return bin.log. This happends for state 1.
		 */
		snprintf(log_name, PATH_MAX, "%s/data/bin.log.%u", g_runtime->home, log_ver);
		struct stat stbuf;
		int ret = stat(log_name, &stbuf);
		if (ret == -1 && errno == ENOENT) {
			snprintf(log_name, PATH_MAX, "%s/data/bin.log", g_runtime->home);
		}
        return 0;
	} else {
        return 1;
    }
}

/**
 * Open the synclog for the given synclog version.
 *
 * @param log_ver synclog version
 * @param log_name return the synclog file pathname
 * @param log_fd_ptr return the opened file descriptor
 */
static void 
open_synclog(unsigned int log_ver, char *log_name, int *log_fd_ptr) 
{
    if (get_synclog_filename(log_ver, log_name) != 0) {
        DERROR("sync log with version %u does not exist\n", log_ver);
    }
    *log_fd_ptr = open(log_name, O_RDONLY);
    if (*log_fd_ptr == -1) {
        DERROR("open file %s error! %s\n", log_name, strerror(errno));
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
    // seek
    if (lseek(fd, offset, SEEK_SET) == -1) {
        DERROR("lseek error! %s\n", strerror(errno));
        close(fd);
        MEMLINK_EXIT;    
    }
    // read
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
get_synclog_record_pos(unsigned int log_ver, unsigned int log_no, int *fd_ptr, char *log_name)
{
    if (get_synclog_filename(log_ver, log_name) != 0)
        return 0;

    *fd_ptr = open(log_name, O_RDONLY);
    if (*fd_ptr == -1) {
        DERROR("open file %s error! %s\n", log_name, strerror(errno));
        MEMLINK_EXIT;
    }

    unsigned int log_index_pos = get_index_pos(log_no);
    unsigned int log_pos = seek_and_read_int(*fd_ptr, log_index_pos);
    if (log_pos > 0) {
        DINFO("log record %u is at %u\n", log_no, log_pos);
    } else {
        DERROR("log record %u does not exist in log file %s\n", log_no, log_name);
    }
    return log_pos;
}

static struct timeval
create_interval(unsigned int seconds) 
{
    struct timeval tv;
    evutil_timerclear(&tv);
    tv.tv_sec = seconds;
    return tv;
}

/**
 * Read n bytes. Terminate program if error.
 */
static int
sthread_readn(int fd, void *ptr, size_t n)
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
sthread_writen(int fd, void *ptr, size_t n)
{
    int ret;
    if ((ret = writen(fd, ptr, n, g_cf->timeout)) != n) {
        DERROR("Unable to write %d byes, only %d bytes are written.\n", n, ret);
        MEMLINK_EXIT;
    }
}

/**
 * Read n bytes from the synclog file and write them to sync connection socket.
 */
static void 
transfer(SyncLogConn *conn,  void *ptr, size_t n)
{
    int ret;
    if ((ret = sthread_readn(conn->log_fd, ptr, n)) != n) {
        DERROR("Unable to read %d bytes, only %d bytes are read.\n", n, ret);
        MEMLINK_EXIT;
    }
    sthread_writen(conn->sock, ptr, n);
}


/**
 * Send available synclog records in the current open synclog file in 
 * SyncLogConn.
 */
static void
send_synclog_record(SyncLogConn *conn)
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
            && (log_pos = seek_and_read_int(conn->log_fd, conn->log_index_pos)) > 0) {
        if (lseek(conn->log_fd, log_pos, SEEK_SET) == -1) {
            DERROR("lseek error! %s\n", strerror(errno));
            close(conn->log_fd);
            MEMLINK_EXIT;
        }
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
    SyncLogConn *conn = arg;
    /*
     * Send history synclog files and open the current synclog file.
     */
    while (conn->log_ver < g_runtime->logver) {
        send_synclog_record(conn);
        (conn->log_ver)++;
        conn->log_index_pos = get_index_pos(0);
        open_synclog(conn->log_ver, conn->log_name, &conn->log_fd);
    }

    /*
     * Send available records in the current synclog file
     */
    send_synclog_record(conn);

    struct timeval interval = create_interval(g_cf->timeout);
    event_add(&conn->evt, &interval);
}

static SyncLogConn*
synclog_conn_init(int sock, unsigned int log_ver, unsigned int log_no)
{
    SyncLogConn* conn = synclog_conn_create();
    conn->sock = sock;
    conn->log_ver = log_ver;
    conn->log_index_pos = get_index_pos(log_no);
	return conn;
}

static void 
send_synclog_init(SyncLogConn* conn) 
{
    evtimer_set(&conn->evt, send_synclog, conn);
    event_base_set(g_runtime->sthread->base, &conn->evt);
    struct timeval interval = create_interval(0); 
    event_add(&conn->evt, &interval);
}

static int
cmd_sync(Conn* conn, char *data, int datalen) 
{
    int ret;
	int log_fd;
	char log_name[PATH_MAX];
    unsigned int log_ver;
    unsigned int log_no;
    cmd_sync_unpack(data, &log_ver, &log_no);
    DINFO("log version: %u, log record number: %u\n", log_ver, log_no);
    if ((get_synclog_record_pos(log_ver, log_no, &log_fd, log_name)) > 0) {
        ret = data_reply(conn, 0, NULL, 0);
        DINFO("Found sync log file (version = %u)\n", log_ver);
        SyncLogConn *syncConn = synclog_conn_init(conn->sock, log_ver, log_no);
		syncConn->log_fd = log_fd;
        strcpy(syncConn->log_name, log_name);
		send_synclog_init(syncConn);
    } else { 
        ret = data_reply(conn, 1, (char *)&(g_runtime->dumpver), sizeof(int));
        DINFO("Not found syn log file with version %u\n", log_ver);
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

static long long
get_file_size(int fd)
{
   long long file_size;
   if ((file_size = lseek(fd, 0, SEEK_END)) == -1) {
       DERROR("lseek error! %s", strerror(errno));
       MEMLINK_EXIT;
   }
   if (lseek(fd, 0, SEEK_SET) == -1) {
       DERROR("lseek error! %s", strerror(errno));
       MEMLINK_EXIT;
   }
   return file_size;
}


static void 
send_dump(int fd, short event, void *arg)
{
    DumpConn* dumpConn = arg;
    char buf[BUF_SIZE];
    int p_buf_size = BUF_SIZE * 3;
    char p_buf[p_buf_size];
    int ret;
    /*
     * The current log version may be changed if dump event happends. So it is 
     * saved here.
     */
    unsigned int log_ver = g_runtime->logver;

    DINFO("sending dump...\n");
    while ((ret = sthread_readn(dumpConn->dump_fd, buf, BUF_SIZE)) > 0) {
        DINFO("dump data: %s\n", formath(buf, ret, p_buf, p_buf_size));
        sthread_writen(dumpConn->sock, buf, ret);
    }
    dump_conn_destroy(dumpConn);

	SyncLogConn* syncConn = synclog_conn_init(dumpConn->sock, log_ver, 0);
	open_synclog(syncConn->log_ver, syncConn->log_name, &(syncConn->log_fd));
    send_synclog_init(syncConn); 
}

static void 
send_dump_init(int sock, int dump_fd)
{
    DumpConn* conn = dump_conn_create();
    conn->sock = sock;
    conn->dump_fd = dump_fd;

    evtimer_set(&conn->evt, send_dump, conn);
    event_base_set(g_runtime->sthread->base, &conn->evt);
    struct timeval interval = create_interval(0); 
    event_add(&conn->evt, &interval);
}

static int
cmd_get_dump(Conn* conn, char *data, int datalen)
{
    int ret;
    unsigned int dumpver;
    unsigned int size;
    int retcode;

    cmd_getdump_unpack(data, &dumpver, &size);
    DINFO("dump version: %u, synchronized data size: %u\n", dumpver, size);
    retcode = g_runtime->dumpver == dumpver ? 1 : 0;

    int dump_fd = open_dump(); 
    long long file_size = get_file_size(dump_fd);
    int retlen = sizeof(int) + sizeof(long long);
    char retrc[retlen];
    memcpy(retrc, &g_runtime->dumpver, sizeof(int));
    memcpy(retrc + sizeof(int), &file_size, sizeof(long long));
    ret = data_reply(conn, retcode, retrc, retlen);

    send_dump_init(conn->sock, dump_fd);

    return ret;
}

static void 
sthread_read(int fd, short event, void *arg) 
{
    SThread *st = (SThread*) arg;
    Conn *conn;
    int ret;

    DINFO("sthread_read...\n");
    conn = conn_create(fd);

    if (conn) {
        conn->port = g_cf->sync_port;
        conn->base = st->base;

        DINFO("new conn: %d\n", conn->sock);
        DINFO("change event to read.\n");
        ret = change_event(conn, EV_READ | EV_PERSIST, 1);
        if (ret < 0) {
            DERROR("change_event error: %d, close conn.\n", ret);
            conn_destroy(conn);
        }
    }
}

SThread*
sthread_create() 
{
    SThread *st = (SThread*)zz_malloc(sizeof(WThread));
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
sdata_ready(Conn *conn, char *data, int datalen) 
{
    int ret;
    char cmd;

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

    return 0;
}

void sthread_destroy(SThread *st) 
{
    zz_free(st);
}

SyncLogConn*
synclog_conn_create() 
{
    SyncLogConn* conn = (SyncLogConn*) sthread_malloc(sizeof(SyncLogConn));
    memset(conn, 0, sizeof(SyncLogConn));
    return conn;
}

void 
synclog_conn_destroy(SyncLogConn *conn) 
{
	event_del(&conn->evt);
	close(conn->sock);
	close(conn->log_fd);
	zz_free(conn);
}

DumpConn*
dump_conn_create() 
{
    DumpConn* conn = (DumpConn*) sthread_malloc(sizeof(DumpConn));
    memset(conn, 0, sizeof(DumpConn));
    return conn;
}

void
dump_conn_destroy(DumpConn *conn) 
{
    event_del(&conn->evt);
    close(conn->dump_fd);
    zz_free(conn);
}
