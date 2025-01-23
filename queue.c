#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "queue.h"


TQueue *createQueue(int size) {
    TQueue *queue = (TQueue *)malloc(sizeof(TQueue));
    queue->messages = (Message *)malloc(size * sizeof(Message));
    queue->subscribers_head = NULL;
    
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

    Subscriber *temp = queue->subscribers_head;
    while (temp != NULL) {
        Subscriber *next = temp->next;
        free(temp);
        temp = next;
    }

    free(queue->messages);
    free(queue);
}

void subscribe(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->mutex);

    Subscriber* temp = queue->subscribers_head;
    if (temp == NULL) {
        queue->subscribers_head = (Subscriber *)malloc(sizeof(Subscriber));
        temp = queue->subscribers_head;
    } else {
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = (Subscriber *)malloc(sizeof(Subscriber));
        temp = temp->next;
    }
    
    temp->thread = thread;
    temp->read_position = queue->tail;
    temp->new_messages = 0;
    temp->next = NULL;

    queue->subscriber_count++;

    pthread_mutex_unlock(&queue->mutex);
}


void unsubscribe(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->mutex);

    Subscriber *temp = queue->subscribers_head;
    if (pthread_equal(temp->thread, thread)) {
        queue->subscribers_head = temp->next;
        free(temp);
        queue->subscriber_count--;
    } else {
        while (temp->next != NULL) {
            if (pthread_equal(temp->next->thread, thread)) {
                Subscriber *next = temp->next->next;
                free(temp->next);
                temp->next = next;
                queue->subscriber_count--;
                break;
            }
            temp = temp->next;
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
    message->undelivered = queue->subscriber_count;

    Subscriber* temp = queue->subscribers_head;
    while (temp != NULL) {
        temp->new_messages++;
        temp = temp->next;
    }

    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;

    pthread_cond_broadcast(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
}

void *getMsg(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->mutex);

    Subscriber *subscriber = NULL;
    Subscriber *temp = queue->subscribers_head;
    if (temp != NULL) {
        if (pthread_equal(temp->thread, thread)) {
            subscriber = temp;
        } else {
            while (temp->next != NULL) {
                temp = temp->next;
                if (pthread_equal(temp->thread, thread)) {
                    subscriber = temp;
                    break;
                }
            }
        }
    }
    if (!subscriber) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }

    if (subscriber->new_messages == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    
    int index = subscriber->read_position;
    
    queue->messages[index].undelivered--;

    if (queue->messages[index].undelivered == 0) {
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


int getAvailable(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->mutex);

    Subscriber *temp = queue->subscribers_head;
    Subscriber *subscriber = NULL;

    if (temp != NULL) {
        if (pthread_equal(temp->thread, thread)) {
            subscriber = temp;
        } else {
            while (temp->next != NULL) {
                temp = temp->next;
                if (pthread_equal(temp->thread, thread)) {
                    subscriber = temp;
                    break;
                }
            }
        }
    }

    if (!subscriber) {
        pthread_mutex_unlock(&queue->mutex);
        return 0;
    }

    int count = subscriber->new_messages;

    pthread_mutex_unlock(&queue->mutex);

    return count;
}

void setSize(TQueue *queue, int size) {
    pthread_mutex_lock(&queue->mutex);

    if (size < queue->size) {
        int to_remove = queue->size - size;

        for (int i = 0; i < to_remove; i++) {
            Subscriber* temp = queue->subscribers_head;

            while (temp != NULL) {
                if (queue->head == temp->read_position) {
                    temp->read_position = (temp->read_position + 1) % queue->capacity;
                    temp->new_messages--;
                }
                temp = temp->next;
            }

            queue->head = (queue->head + 1) % queue->capacity;
            queue->size--;
        }
    }

    queue->messages = (Message *)realloc(queue->messages, size * sizeof(Message));
    queue->capacity = size;
    
    pthread_mutex_unlock(&queue->mutex);
}
