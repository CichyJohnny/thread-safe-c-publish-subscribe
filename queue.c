#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "queue.h"


TQueue *createQueue(int size) {
    TQueue *queue = (TQueue *)malloc(sizeof(TQueue));
    queue->messages_head = NULL;
    queue->subscribers_head = NULL;
    
    queue->capacity = size;
    queue->size = 0;
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

    Subscriber *sub = queue->subscribers_head;
    while (sub != NULL) {
        Subscriber *next = sub->next;
        free(sub);
        sub = next;
    }

    Message *mess = queue->messages_head;
    while (mess != NULL) {
        Message *next = mess->next;
        free(mess);
        mess = next;
    }

    free(queue);
}

void subscribe(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->mutex);

    Subscriber* sub = queue->subscribers_head;
    if (sub == NULL) {
        queue->subscribers_head = (Subscriber *)malloc(sizeof(Subscriber));
        sub = queue->subscribers_head;
    } else {
        if (pthread_equal(sub->thread, thread)) {
                pthread_mutex_unlock(&queue->mutex);
                return;
            }
        while (sub->next != NULL) {
            if (pthread_equal(sub->thread, thread)) {
                pthread_mutex_unlock(&queue->mutex);
                return;
            }
            sub = sub->next;
        }
        sub->next = (Subscriber *)malloc(sizeof(Subscriber));
        sub = sub->next;
    }

    sub->thread = thread;
    sub->new_messages = 0;
    sub->next = NULL;

    queue->subscriber_count++;

    pthread_mutex_unlock(&queue->mutex);
}


void unsubscribe(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->mutex);

    Subscriber *sub = queue->subscribers_head;
    if (!sub) {
        pthread_mutex_unlock(&queue->mutex);
        return;
    }
    if (pthread_equal(sub->thread, thread)) {
        queue->subscribers_head = sub->next;
        free(sub);
        queue->subscriber_count--;
    } else {
        while (sub->next != NULL) {
            if (pthread_equal(sub->next->thread, thread)) {
                Subscriber *next = sub->next->next;
                free(sub->next);
                sub->next = next;
                queue->subscriber_count--;
                break;
            }
            sub = sub->next;
        }
    }

    pthread_mutex_unlock(&queue->mutex);
}

void addMsg(TQueue *queue, void *msg) {
    pthread_mutex_lock(&queue->mutex);

    while (queue->size == queue->capacity) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    
    Message* mess = queue->messages_head;
    if (mess == NULL) {
        queue->messages_head = (Message *)malloc(sizeof(Message));
        mess = queue->messages_head;
    } else {
        while (mess->next != NULL) {
            mess = mess->next;
        }
        mess->next = (Message *)malloc(sizeof(Message));
        mess = mess->next;
    }

    mess->data = msg;
    mess->undelivered = queue->subscriber_count;
    mess->next = NULL;

    Subscriber* sub = queue->subscribers_head;
    int i = 0;
    while (sub != NULL) {
        sub->new_messages++;
        sub = sub->next;
    }

    queue->size++;

    pthread_cond_broadcast(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
}

void *getMsg(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->mutex);

    // Look for subscriber
    Subscriber *sub = NULL;
    Subscriber *temp = queue->subscribers_head;
    if (temp != NULL) {
        if (pthread_equal(temp->thread, thread)) {
            sub = temp;
        } else {
            while (temp->next != NULL) {
                temp = temp->next;
                if (pthread_equal(temp->thread, thread)) {
                    sub = temp;
                    break;
                }
            }
        }
    }
    if (sub == NULL) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }

    // Wait if no new messages_head
    if (sub->new_messages == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    // Find least recent message
    Message* message = queue->messages_head;
    Message* prev = NULL;
    int index = queue->size - sub->new_messages;

    for (int i = 0; i < index; i++) {
        prev = message;
        message = message->next;
    }


    void *msg = message->data;

    // Remove message if all subscribers received it
    message->undelivered--;
    if (message->undelivered == 0) {
        if (prev == NULL) {
            queue->messages_head = message->next;
        } else {
            prev->next = message->next;
        }

        queue->size--;
        free(message);
        pthread_cond_signal(&queue->not_full);
    }

    sub->new_messages--;

    pthread_mutex_unlock(&queue->mutex);
    
    return msg;
}


int getAvailable(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->mutex);

    Subscriber *temp = queue->subscribers_head;
    Subscriber *sub = NULL;

    if (temp != NULL) {
        if (pthread_equal(temp->thread, thread)) {
            sub = temp;
        } else {
            while (temp->next != NULL) {
                temp = temp->next;
                if (pthread_equal(temp->thread, thread)) {
                    sub = temp;
                    break;
                }
            }
        }
    }

    if (!sub) {
        pthread_mutex_unlock(&queue->mutex);
        return 0;
    }

    int count = sub->new_messages;

    pthread_mutex_unlock(&queue->mutex);

    return count;
}

void setSize(TQueue *queue, int size) {
    pthread_mutex_lock(&queue->mutex);

    if (size < queue->size) {
        int to_remove = queue->size - size;
        for (int i = 0; i < to_remove; i++) {
            Message* message = queue->messages_head;
            queue->messages_head = queue->messages_head->next;
            free(message);
            queue->size--;
        }

        Subscriber* subscriber = queue->subscribers_head;
        while (subscriber != NULL) {
            if (subscriber->new_messages > size) {
                subscriber->new_messages = size;
            }
            subscriber = subscriber->next;
        }
    } else if (size > queue->size) {
        pthread_cond_signal(&queue->not_full);
    }

    queue->capacity = size;
    
    pthread_mutex_unlock(&queue->mutex);
}
