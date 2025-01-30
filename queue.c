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
    pthread_mutex_lock(&queue->mutex);
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

    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);

    free(queue);
}

void subscribe(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->mutex);

    Subscriber* sub = queue->subscribers_head;

    // Sprawdź czy wątek już istnieje
    while (sub != NULL) {
        if (pthread_equal(sub->thread, thread)) {
            pthread_mutex_unlock(&queue->mutex);
            return;
        }
        sub = sub->next;
    }

    // Dodaj nowy Subscriber na koniec listy
    Subscriber* new_sub = (Subscriber *)malloc(sizeof(Subscriber));
    new_sub->thread = thread;
    new_sub->new_messages = 0;
    new_sub->next = NULL;

    if (queue->subscribers_head == NULL) {
        queue->subscribers_head = new_sub;
    } else {
        sub = queue->subscribers_head;
        while (sub->next != NULL) {
            sub = sub->next;
        }
        sub->next = new_sub;
    }

    queue->subscriber_count++;
    pthread_mutex_unlock(&queue->mutex);
}


void unsubscribe(TQueue *queue, pthread_t thread) {
    pthread_mutex_lock(&queue->mutex);

    Subscriber *sub = queue->subscribers_head;
    int index = -1;
    if (!sub) {
        pthread_mutex_unlock(&queue->mutex);
        return;
    }

    if (pthread_equal(sub->thread, thread)) {
        index = queue->size - sub->new_messages;

        queue->subscribers_head = sub->next;
        free(sub);
        queue->subscriber_count--;
    } else {
        while (sub->next != NULL) {
            if (pthread_equal(sub->next->thread, thread)) {
                index = queue->size - sub->next->new_messages;

                Subscriber *next = sub->next->next;
                free(sub->next);
                sub->next = next;
                queue->subscriber_count--;
                break;
            }
            sub = sub->next;
        }
    }

    if (index == -1) {
        pthread_mutex_unlock(&queue->mutex);
        return;
    }


    Message* current = queue->messages_head;
    Message* prev = NULL;
    int count = 0;

    while (current != NULL) {
        if (count >= index) {
            current->undelivered--;
            if (current->undelivered == 0) {
                if (prev == NULL) {
                    queue->messages_head = current->next;
                } else {
                    prev->next = current->next;
                }

                Message* to_remove = current;
                current = current->next;

                free(to_remove);
                queue->size--;

                pthread_cond_signal(&queue->not_full);

                continue;
            }
        }
        prev = current;
        current = current->next;
        count++;
    }

    pthread_mutex_unlock(&queue->mutex);
}

void addMsg(TQueue *queue, void *msg) {
    pthread_mutex_lock(&queue->mutex);

    if (queue->subscriber_count == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return;
    }

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

    // Wait if no new messages
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

void removeMsg(TQueue *queue, void *msg) {
    pthread_mutex_lock(&queue->mutex);

    Message* mess = queue->messages_head;
    Message* prev = NULL;

    int index = 0;
    int found = 0;
    while (mess != NULL) {
        if (mess->data == msg) {

            if (prev == NULL) {
                queue->messages_head = mess->next;
            } else {
                prev->next = mess->next;
            }

            queue->size--;
            free(mess);
            pthread_cond_signal(&queue->not_full);

            found = 1;
            
            break;
        }

        prev = mess;
        mess = mess->next;
        index++;
    }

    if (found == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return;
    }

    Subscriber* sub = queue->subscribers_head;
    while (sub != NULL) {
        if (queue->size + 1 - sub->new_messages <= index) {
            sub->new_messages--;
        }
        sub = sub->next;
    }

    pthread_mutex_unlock(&queue->mutex);
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
