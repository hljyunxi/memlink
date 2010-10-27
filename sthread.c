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

static int 
compare_int ( const void *a , const void *b ) 
{ 
    return *(int *)a - *(int *)b; 
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

static int
check_ver_pos(int logid, unsigned int logver, unsigned int logpos)
{
    char logname[PATH_MAX];
    int fd;
    unsigned int version;

    if (logid >= 0) 
        snprintf(logname, PATH_MAX, "%s/data/bin.log.%d", g_runtime->home, logid);
    else 
        snprintf(logname, PATH_MAX, "%s/data/bin.log", g_runtime->home);

    fd = open(logname, O_RDONLY);
    if (fd == -1) {
        DERROR("open file %s error! %s\n", logname, strerror(errno));
        MEMLINK_EXIT;
    }
    if (lseek(fd, sizeof(short), SEEK_SET) == -1) {
        DERROR("seek file %s error! %s\n", logname, strerror(errno));
        goto ERROR;
    }
    if (read(fd, &version, sizeof(int)) == -1) {
        DERROR("read file %s error! %s\n", logname, strerror(errno));
        goto ERROR;
    }
    DINFO("version: %d\n", version);

    int index_pos;
    int pos;
    if (version == logver) {
        index_pos = sizeof(short) + sizeof(int) + sizeof(int) + logpos * sizeof(int);
        if (lseek(fd, index_pos, SEEK_SET) == -1) {
            DERROR("seek file %s error! %s\n", logname, strerror(errno));
            goto ERROR;
        }
        if (read(fd, &pos, sizeof(4)) == -1) {
            DERROR("read file %s error! %s\n", logname, strerror(errno));
            goto ERROR;
        }
        DINFO("index %d points to %d\n", logpos, pos);
        if (pos <= 0) {
            DERROR("log record with index %d does not exist in log file %s\n", logpos, logname);
            return 0;
        }
        close(fd);
        return pos;
    } else {
        close(fd);
        return 0;
    }

ERROR:
    close(fd);
    MEMLINK_EXIT;    
}

static int
find_synclog(unsigned int logver, unsigned int logpos) 
{
    DIR *data_dir;
    struct dirent *node;
    int i;
    int j;
    int logids[10000] = {0};
    int minid = g_runtime->dumplogver;
    int pos;

    data_dir = opendir(g_cf->datadir);
    if (data_dir == NULL) {
        DERROR("opendir %s error: %s\n", g_cf->datadir, strerror(errno));
        MEMLINK_EXIT;
    }

    i = 0;
    while ((node = readdir(data_dir)) != NULL) {
        if (strncmp(node->d_name, "bin.log.", 8) == 0) {
            int binid = atoi(&node->d_name[8]);
            if (binid > minid) {
                logids[i++] = binid;
            }
        }
    }
    closedir(data_dir);
    qsort(logids, i, sizeof(int), compare_int);
    logids[i] = -1; // for bin.log

    DINFO("there are %d history log files\n", i);

    for (j = i; j >= 0; j--) 
        if ((pos = check_ver_pos(logids[j], logver, logpos)) > 0) 
            return pos;
    return -1;
}

static int
cmd_sync(Conn* conn, char *data, int datalen) 
{
    int ret;
    unsigned int logver;
    unsigned int log_index_pos;
    cmd_sync_unpack(data, &logver, &log_index_pos);
    DINFO("log version: %d, log index position: %d\n", logver, log_index_pos);
    if ((find_synclog(logver, log_index_pos)) >= 0) {
        ret = data_reply(conn, 0, NULL, NULL, 0);
        DINFO("Found sync log file (version = %d)\n", logver);
    } else { 
        ret = data_reply(conn, 1, NULL, (char *)&(g_runtime->dumpver), sizeof(int));
        DINFO("Not found syn log file with version %d\n", logver);
    }

    // TODO if requested log exists, send log
    return ret;
}

static int
cmd_sync_dump(Conn* conn, char *data, int datalen)
{
    int ret;
    unsigned int dumpver;
    unsigned int size;
    int retcode;

    //cmd_sync_dump_unpack(data, &dumpver, &size);
    cmd_getdump_unpack(data, &dumpver, &size);
    DINFO("dump version: %d, synchronized data size: %d\n", dumpver, size);
    retcode = g_runtime->dumpver == dumpver ? 1 : 0;
    ret = data_reply(conn, retcode, NULL, NULL, 0);

    // TODO send dump file
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
        DINFO("change event to read.");
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
            ret = cmd_sync_dump(conn, data, datalen);
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
