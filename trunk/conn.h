#ifndef MEMLINK_CONN_H
#define MEMLINK_CONN_H

#include <stdio.h>
#include <event.h>
#include <sys/time.h>

#define CONN_MAX_READ_LEN   1024
#define CONN_READ   1
#define CONN_WRITE  2
#define CONN_SYNC   3

/*
    int     sock;\                    // socket file descriptor
    char    rbuf[CONN_MAX_READ_LEN];\ // read buffer
    int     rlen;\                    // length of data which have been read
    char    *wbuf;\                   // write buffer
    int     wsize;\                   // write buffer mem size
    int     wlen;\                    // write buffer length
    int     wpos;\                    // write buffer position
    int     port;\                    // server port
	struct event		evt;\
    struct event_base   *base;\
    struct timeval      ctime;\
	void	(*destroy)(Conn *conn);\
	int		(*ready)(Conn *conn, char *data, int datalen);


 */

#define CONN_MEMBER \
    int     sock;\
    char    rbuf[CONN_MAX_READ_LEN];\
    int     rlen;\
    char    *wbuf;\
    int     wsize;\
    int     wlen;\
    int     wpos;\
    int     port;\
	char	client_ip[16];\
	int     client_port;\
	struct event		evt;\
    struct event_base   *base;\
    struct timeval      ctime;\
	void	(*destroy)(struct _conn *conn);\
	int		(*ready)(struct _conn *conn, char *data, int datalen);\
	int		(*wrote)(struct _conn *conn);

typedef struct _conn
{
	CONN_MEMBER
}Conn;

Conn*   conn_create(int svrfd, int connsize);
void    conn_write(Conn *conn);
void    conn_destroy(Conn *conn);
int     conn_wrote(Conn *conn);
char*   conn_write_buffer(Conn *conn, int size);
char*   conn_write_buffer_append(Conn *conn, void *data, int datalen);
int     conn_write_buffer_head(Conn *conn, int retcode, int len);



#endif
