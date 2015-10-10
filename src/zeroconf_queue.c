//
// Created by olivier on 25/09/15.
//


#include "zeroconf_queue.h"

struct zc_queue create_queue() {
    struct zc_queue queue = {
            .first = NULL,
            .last = NULL,
            .mutex = NULL,
    };
    check(pthread_mutex_init(&queue.mutex, NULL) == 0, "ERROR DURING MUTEX INIT");
    check(pthread_cond_init(&queue.cond, NULL) == 0, "ERROR DURING CONDITION INIT");
    return queue;
    error:
    exit(1);
}

void *queue_push_back_more(
        struct zc_queue *queue, void *_value, const unsigned short size, unsigned short copy) {
    // Alocate memory for the new queue_elem
    struct zc_queue_elem * elem = malloc(sizeof(struct zc_queue_elem));
    if (copy) {
        // Allocate memory into the heap for the given pointer
        elem->value = malloc(size);
        // Copy memory
        memcpy(elem->value, _value, size);
    }else{
        // Just store the pointer
        elem->value = _value;
    }
    elem->next = NULL; // It's the last elem
    pthread_mutex_lock(&queue->mutex);
    elem->prev = queue->last; // Before it, is the old last elem
    if (elem->prev != NULL) {
        // If there was someone before, make it know that it isn't the last anymore
        elem->prev->next = elem;
    }else{
        // If not we also are the first elem
        queue->first = elem;
    }
    queue->last = elem; // Tell the queue that we are the new last elem
    pthread_cond_broadcast(&queue->cond); // Signal a change into the queue
    pthread_mutex_unlock(&queue->mutex);
    return elem->value;
}

void *queue_push_back(struct zc_queue *queue, void *_value, const unsigned short size) {
    return queue_push_back_more(queue, _value, size, 1);
}

void *queue_pop_front(struct zc_queue *queue) {
    pthread_mutex_lock(&queue->mutex);
    if (queue->first == NULL){
        // If the queue is empty return NULL
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }
    void * value = queue->first->value;
    // Store pointer to the first fsm_queue_elem in order to free it later
    struct zc_queue_elem *fsm_elem_to_free = queue->first;
    // Tell the queue that the new first element have change
    queue->first = queue->first->next;
    // Free the fsm_queue_elem which stored the value
    free(fsm_elem_to_free);
    if(queue->first != NULL) {
        // Tell the new first element that there isn't something behind it anymore
        queue->first->prev = NULL;
    }else{
        // If it was the last element, put last pointer of the queue to NULL
        queue->last = NULL;
    }
    pthread_mutex_unlock(&queue->mutex);
    return value;
}

void queue_cleanup(struct zc_queue *queue) {
    while(queue->first != NULL){
        free(queue_pop_front(queue));
    }
}

struct zc_queue *create_queue_pointer() {
    // Create a fsm_queue
    struct zc_queue _q = create_queue();
    // Allocate memory into the heap
    struct zc_queue * queue = malloc(sizeof(struct zc_queue));
    // Copy stack fsm_queue to heap memory
    memcpy(queue, &_q, sizeof(struct zc_queue));
    // Return pointer to the heap memory
    return queue;
}

void queue_delete_queue_pointer(struct zc_queue *queue) {
    queue_cleanup(queue);
    free(queue);
}

void *queue_get_elem(struct zc_queue *queue, void *elem) {
    pthread_mutex_lock(&queue->mutex);
    struct zc_queue_elem *cursor = queue->first;
    while(cursor != NULL){
        if (cursor->value == elem){
            // Found it, now we will remove it
            if(cursor->next == NULL){
                // Last item of the queue
                queue->last = cursor->prev;
            }else{
                cursor->next->prev = cursor->prev;
            }
            if(cursor->prev == NULL){
                // First item of the queue
                queue->first = cursor->next;
            }else{
                cursor->prev->next = cursor->next;
            }
            free(cursor);   // Freeing the fsm_queue_elem to avoid memory leaks
            pthread_mutex_unlock(&queue->mutex);
            return elem;
        }
        cursor = cursor->next;
    }
    pthread_mutex_unlock(&queue->mutex);
    return NULL;
}

void *queue_push_top_more(struct zc_queue *queue, void *_value, const unsigned short size, unsigned short copy) {
    // Alocate memory for the new fsm_queue_elem
    struct zc_queue_elem * elem = malloc(sizeof(struct zc_queue_elem));
    if (copy) {
        // Allocate memory into the heap for the given pointer
        elem->value = malloc(size);
        // Copy memory
        memcpy(elem->value, _value, size);
    }else{
        // Just store the pointer
        elem->value = _value;
    }
    elem->prev = NULL; // It's the fisrt elem
    pthread_mutex_lock(&queue->mutex);
    elem->next = queue->first; // Before it, is the old first elem
    if (elem->next != NULL) {
        // If there was someone before, make it know that it isn't the first anymore
        elem->next->prev = elem;
    }else{
        // If not we also are the last elem
        queue->last = elem;
    }
    queue->first = elem; // Tell the queue that we are the new last elem
    pthread_cond_broadcast(&queue->cond); // Signal a change into the queue
    pthread_mutex_unlock(&queue->mutex);
    return elem->value;
}

void *queue_push_top(struct zc_queue *queue, void *_value, const unsigned short size) {
    return queue_push_top_more(queue, _value, size, 1);
}