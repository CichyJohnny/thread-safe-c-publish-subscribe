#ifndef LCL_QUEUE_H
#define LCL_QUEUE_H

// ==============================================
//
//  Version 1.1, 2025-01-16
//
// ==============================================

#include <pthread.h>
#include <stdbool.h>

#define MAX_SUBSCRIBERS 64


typedef struct {
    void *data;
    bool delivered[MAX_SUBSCRIBERS];
} Message;


typedef struct {
    pthread_t thread;
    int read_position;
    int new_messages;

    Subscriber* next;
} Subscriber;

// Struktura kolejki
typedef struct {
    Message *messages;
    int capacity;
    int size;
    int head;
    int tail;
    Subscriber *subscribers;
    int subscriber_count;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} TQueue;

TQueue* createQueue(int size);

void destroyQueue(TQueue *queue);

void subscribe(TQueue *queue, pthread_t thread);

void unsubscribe(TQueue *queue, pthread_t thread);

void addMsg(TQueue *queue, void *msg);

void* getMsg(TQueue *queue, pthread_t thread);

int getAvailable(TQueue *queue, pthread_t thread);

void removeMsg(TQueue *queue, void *msg);

void setSize(TQueue *queue, int size);

#endif //LCL_QUEUE_H
