//
// Created by olivier on 25/09/15.
//

#ifndef RTP_QUEUE_H
#define RTP_QUEUE_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "pthread.h"
#include "debug.h"

struct zc_queue_elem {
    struct zc_queue_elem * next;
    struct zc_queue_elem * prev;
    void * value;
};

struct zc_queue {
    struct zc_queue_elem * first;
    struct zc_queue_elem * last;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

struct zc_queue create_queue();

void *queue_push_back_more(
        struct zc_queue *queue, void *_value, const unsigned short size, unsigned short copy);

void *queue_push_back(struct zc_queue *queue, void *_value, const unsigned short size);

void *queue_pop_front(struct zc_queue *queue);

void queue_cleanup(struct zc_queue *queue);

struct zc_queue *create_queue_pointer();

void queue_delete_queue_pointer(struct zc_queue *queue);

void *queue_get_elem(struct zc_queue *queue, void *elem);

void *queue_push_top_more(struct zc_queue *queue, void *_value, const unsigned short size, unsigned short copy);

void *queue_push_top(struct zc_queue *queue, void *_value, const unsigned short size);

#endif // RTP_QUEUE_H