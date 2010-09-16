#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "queue.h"
#include "myconfig.h"
#include "logfile.h"
#include "zzmalloc.h"

Queue*      
queue_create()
{
    Queue   *q;

    q = (Queue*)zz_malloc(sizeof(Queue));
    if (NULL == q) {
        DERROR("malloc Queue error!\n");
        MEMLINK_EXIT;
        return NULL;
    }
    memset(q, 0, sizeof(Queue));
    
    int ret = pthread_mutex_init(&q->lock, NULL);
    if (-1 == ret) {
        DERROR("pthread_mutex_init error!\n");
        MEMLINK_EXIT;
        return NULL;
    }

    return q;
}

void        
queue_destroy(Queue *q)
{
    QueueItem   *item, *tmp;

    item = q->head;

    while (item) {
        tmp = item; 
        item = item->next;
        zz_free(tmp);
    }
    pthread_mutex_destroy(&q->lock);
    zz_free(q);
}

int         
queue_append(Queue *q, Conn *conn)
{
    int ret = 0;


    QueueItem *item = (QueueItem*)zz_malloc(sizeof(QueueItem));
    if (NULL == item) {
        DERROR("malloc QueueItem error!\n");
        MEMLINK_EXIT;
        //goto queue_append_over;
    }
    item->conn = conn;
    item->next = NULL;

    pthread_mutex_lock(&q->lock);
    
    if (q->foot == NULL) {
        q->foot = item;
        q->head = item;
    }else{
        q->foot->next = item;
        q->foot = item;
    }
//queue_append_over:
    pthread_mutex_unlock(&q->lock);
    return ret;
}

QueueItem*  
queue_get(Queue *q)
{
    QueueItem   *ret;

    pthread_mutex_lock(&q->lock);
    ret = q->head;
    q->head = q->foot = NULL;
    pthread_mutex_unlock(&q->lock);

    return ret;
}


void
queue_free(Queue *q, QueueItem *item)
{
    QueueItem   *tmp;

    while (item) {
        tmp = item;
        item = item->next;
        zz_free(tmp);
    }
}


