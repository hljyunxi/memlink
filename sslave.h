#ifndef MEMLINK_SSLAVE_H
#define MEMLINK_SSLAVE_H

typedef struct _sslave 
{
    int sock;
    struct event_base *base;
    struct event event;
} SSlave;

SSlave* sslave_create();
void sslave_destroy();

#endif
