#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "queue.h"

#define MAX_SUBSCRIBERS 64

TQueue *createQueue(int size) {
    TQueue *queue = (TQueue *)malloc(sizeof(TQueue));
    queue->messages = (Message *)malloc(size * sizeof(Message));
    queue->subscribers = (Subscriber *)malloc(MAX_SUBSCRIBERS * sizeof(Subscriber));
    
    queue->capacity = size;
    queue->size = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->subscriber_count = 0;

    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->not_full, NULL);
    pthread_cond_init(&queue->not_empty, NULL);

    return queue;
}

void destroyQueue(TQueue *queue) {
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_full);
    pthread_cond_destroy(&queue->not_empty);

    free(queue->messages);
    free(queue->subscribers);
    free(queue);
}

void subscribe(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->mutex);

    if (queue->subscriber_count < MAX_SUBSCRIBERS) {
        queue->subscribers[queue->subscriber_count].thread = thread;
        queue->subscribers[queue->subscriber_count].read_position = queue->tail;
        queue->subscribers[queue->subscriber_count].new_messages = 0;
        queue->subscriber_count++;
    }
    pthread_mutex_unlock(&queue->mutex);
}


void unsubscribe(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->mutex);

    for (int i = 0; i < queue->subscriber_count; i++) {
        if (pthread_equal(queue->subscribers[i].thread, thread)) {
            memmove(&queue->subscribers[i], &queue->subscribers[i + 1], (queue->subscriber_count - i - 1) * sizeof(pthread_t));
            queue->subscriber_count--;
            break;
        }
    }

    pthread_mutex_unlock(&queue->mutex);
}

void addMsg(TQueue *queue, void *msg) {
    pthread_mutex_lock(&queue->mutex);

    while (queue->size == queue->capacity) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }

    Message *message = &(queue->messages[queue->tail]);
    message->data = msg;
    for (int i = 0; i < MAX_SUBSCRIBERS; i++) {
        message->delivered[i] = false;
    }
    for (int i = 0; i < queue->subscriber_count; i++) {
        queue->subscribers[i].new_messages++;
    }

    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;

    pthread_cond_broadcast(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
}

void *getMsg(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->mutex);

    int subscriber_index = -1;
    Subscriber *subscriber = NULL;
    for (int i = 0; i < queue->subscriber_count; i++) {
        if (pthread_equal(queue->subscribers[i].thread, thread)) {
            subscriber_index = i;
            subscriber = &queue->subscribers[i];
            break;
        }
    }

    if (subscriber_index == -1) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }

    if (subscriber->new_messages == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    
    for (int i = 0; i < subscriber->new_messages; i++) {
        int index = (subscriber->read_position + i) % queue->capacity;
        
        if (!queue->messages[index].delivered[subscriber_index]) {
            queue->messages[index].delivered[subscriber_index] = true;

            bool all_delivered = true;
            for (int j = 0; j < queue->subscriber_count; j++) {
                if (!queue->messages[index].delivered[j]) {
                    all_delivered = false;
                    break;
                }
            }

            if (all_delivered) {
                queue->head = (queue->head + 1) % queue->capacity;
                queue->size--;
                pthread_cond_signal(&queue->not_full);
            }

            subscriber->read_position = (subscriber->read_position + 1) % queue->capacity;
            subscriber->new_messages--;
            void *msg = queue->messages[index].data;

            pthread_mutex_unlock(&queue->mutex);
            return msg;
        }
    }
}


int getAvailable(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->mutex);

    int subscriber_index = -1;
    for (int i = 0; i < queue->subscriber_count; i++) {
        if (pthread_equal(queue->subscribers[i].thread, thread)) {
            subscriber_index = i;
            break;
        }
    }

    if (subscriber_index == -1) {
        pthread_mutex_unlock(&queue->mutex);
        return 0;
    }

    int count = queue->subscribers[subscriber_index].new_messages;

    pthread_mutex_unlock(&queue->mutex);

    return count;
}

void setSize(TQueue *queue, int size) {
    pthread_mutex_lock(&queue->mutex);

    if (size < queue->size) {
        int to_remove = queue->size - size;

        for (int i = 0; i < to_remove; i++) {
            for (int j = 0; j < queue->subscriber_count; j++) {
                if (queue->head == queue->subscribers[j].read_position) {
                    queue->subscribers[j].read_position = (queue->subscribers[j].read_position + 1) % queue->capacity;
                    queue->subscribers[j].new_messages--;
                }
            }
            queue->head = (queue->head + 1) % queue->capacity;
            queue->size--;
        }
    }

    queue->messages = (Message *)realloc(queue->messages, size * sizeof(Message));
    queue->capacity = size;
    
    pthread_mutex_unlock(&queue->mutex);
}
