#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include "queue.h"

void *initTest(void *) {
    TQueue *queue = createQueue(10);
    destroyQueue(queue);
    bool *success = malloc(sizeof(bool));
    *success = true;
    return success;
}

void *threadBusyWait(void *) {
    while (true) {}
}

void *singleSubscriberMultipleMessagesSequentialReceivedTest(void *) {
    bool *success = malloc(sizeof(bool));
    *success = true;

    TQueue *queue = createQueue(10);

    pthread_t* thread = malloc(sizeof(pthread_t));
    pthread_create(thread, NULL, threadBusyWait, NULL);
    subscribe(queue, *thread);
    int *msg = malloc(sizeof(int));
    *msg = 10;
    addMsg(queue, msg);
    const int available = getAvailable(queue, *thread);
    const int* msg2 = getMsg(queue, *thread);
    const int available2 = getAvailable(queue, *thread);
    if (available != 1 || available2 != 0 || *msg2 != 10 || *msg != *msg2) {
        destroyQueue(queue);
        free(msg);

        *success = false;
        return success;
    }

    int *msg3 = malloc(sizeof(int));
    *msg3 = 20;
    addMsg(queue, msg3);
    const int available3 = getAvailable(queue, *thread);
    int* msg4 = getMsg(queue, *thread);
    const int available4 = getAvailable(queue, *thread);
    if (available3 != 1 || available4 != 0 || *msg4 != 20 || *msg3 != *msg4) {
        destroyQueue(queue);
        free(msg);
        free(msg3);
        *success = false;
        return success;
    }


    destroyQueue(queue);
    free(msg);
    free(msg3);
    pthread_cancel(*thread);
    free(thread);

    return success;
}

void *singleSubscriberMultipleMessagesSingleReceivedTest(void *) {
    bool *success = malloc(sizeof(bool));
    *success = true;

    TQueue *queue = createQueue(10);

    pthread_t* thread = malloc(sizeof(pthread_t));
    pthread_create(thread, NULL, threadBusyWait, NULL);
    subscribe(queue, *thread);

    int *msg = malloc(sizeof(int));
    *msg = 10;
    addMsg(queue, msg);

    const int available = getAvailable(queue,*thread);

    int *msg2 = malloc(sizeof(int));
    *msg2 = 20;
    addMsg(queue, msg2);

    const int available2 = getAvailable(queue, *thread);
    const int* msg3 = getMsg(queue, *thread);
    const int available3 = getAvailable(queue, *thread);
    const int* msg4 = getMsg(queue, *thread);
    const int available4 = getAvailable(queue, *thread);

    if (
        available != 1 ||
        available2 != 2 ||
        available3 != 1 ||
        available4 != 0 ||
        *msg3 != 10 ||
        *msg4 != 20
        ) {
        destroyQueue(queue);
        free(msg);
        free(msg2);

        *success = false;
        return success;
    }

    destroyQueue(queue);
    free(msg);
    free(msg2);
    pthread_cancel(*thread);
    free(thread);

    return success;
}

void *multipleSubscribersSingleMessageTest(void *) {
    bool *success = malloc(sizeof(bool));
    *success = true;
    
    int size = 10;
    TQueue *queue =createQueue(size);

    pthread_t* thread = malloc(sizeof(pthread_t));
    pthread_create(thread, NULL, threadBusyWait, NULL);
    subscribe(queue, *thread);

    int *msg = malloc(sizeof(int));
    *msg = 10;
    addMsg(queue, msg);

    const int available = getAvailable(queue, *thread);
    if (available != 1) {
        destroyQueue(queue);
        free(msg);
        free(thread);

        *success = false;
        return success;
    }

    pthread_t* thread2 = malloc(sizeof(pthread_t));
    pthread_create(thread2, NULL, threadBusyWait, NULL);
    subscribe(queue, *thread2);
    const int available2 = getAvailable(queue, *thread2);
    if (available2 != 0) {
        destroyQueue(queue);
        free(msg);
        free(thread2);
        free(thread);

        *success = false;
        return success;
    }
    getMsg(queue, *thread);
    const int available3 = getAvailable(queue, *thread);
    if (available3 != 0) {
        destroyQueue(queue);
        free(msg);
        free(thread2);
        free(thread);

        *success = false;
        return success;
    }

    char* msg2 = malloc(sizeof(char) * 10);
    strcpy(msg2, "hello");
    addMsg(queue, msg2);
    const int available4 = getAvailable(queue, *thread);
    const int available5 = getAvailable(queue, *thread2);
    if (available4 != 1 || available5 != 1) {
        destroyQueue(queue);
        free(msg);
        free(thread2);
        free(thread);
        free(msg2);

        *success = false;
        return success;
    }

    const char* msg3 = getMsg(queue, *thread);
    const char* msg4 = getMsg(queue, *thread2);
    if (strcmp(msg3, "hello") != 0 || strcmp(msg4, "hello") != 0) {
        destroyQueue(queue);
        free(msg);
        free(thread2);
        free(thread);
        free(msg2);

        *success = false;
        return success;
    }

    destroyQueue(queue);
    free(msg);
    free(msg2);

    pthread_cancel(*thread2);
    free(thread2);

    pthread_cancel(*thread);
    free(thread);

    return success;
}

void *multipleSubscribersMultipleMessagesTest(void *) {
    bool *success = malloc(sizeof(bool));
    *success = true;

    TQueue *queue = createQueue(10);

    pthread_t* thread = malloc(sizeof(pthread_t));
    pthread_create(thread, NULL, threadBusyWait, NULL);
    subscribe(queue, *thread);

    int *msg = malloc(sizeof(int));
    *msg = 10;
    addMsg(queue, msg);

    const int available = getAvailable(queue, *thread);
    if (available != 1) {
        destroyQueue(queue);
        free(msg);
        free(thread);

        *success = false;
        return success;
    }

    pthread_t* thread2 = malloc(sizeof(pthread_t));
    pthread_create(thread2, NULL, threadBusyWait, NULL);
    subscribe(queue, *thread2);
    const int available2 = getAvailable(queue, *thread2);
    if (available2 != 0) {
        destroyQueue(queue);
        free(msg);
        free(thread2);
        free(thread);

        *success = false;
        return success;
    }

    const int available3 = getAvailable(queue, *thread);
    if (available3 != 1) {
        destroyQueue(queue);
        free(msg);
        free(thread2);
        free(thread);

        *success = false;
        return success;
    }

    char* msg2 = malloc(sizeof(char) * 10);
    strcpy(msg2, "hello");
    addMsg(queue, msg2);
    const int available4 = getAvailable(queue, *thread);
    const int available5 = getAvailable(queue, *thread2);
    if (available4 != 2 || available5 != 1) {
        destroyQueue(queue);
        free(msg);
        free(thread2);
        free(thread);
        free(msg2);

        *success = false;
        return success;
    }

    const int* msg3 = getMsg(queue, *thread);
    const char* msg4 = getMsg(queue, *thread);
    const char* msg5 = getMsg(queue, *thread2);
    if (*msg3 != 10 || strcmp(msg4, "hello") != 0 || strcmp(msg5, "hello") != 0) {
        destroyQueue(queue);
        free(msg);
        free(thread2);
        free(thread);
        free(msg2);

        *success = false;
        return success;
    }

    destroyQueue(queue);
    free(msg);
    free(msg2);

    pthread_cancel(*thread2);
    free(thread2);

    pthread_cancel(*thread);
    free(thread);

    return success;
}

void *changeSizeTest(void *) {
    bool *success = malloc(sizeof(bool));
    *success = true;

    TQueue *queue = createQueue(10);
    pthread_t** threads = malloc(sizeof(pthread_t*) * 10);
    for (int i = 0; i < 10; i++) {
        threads[i] = malloc(sizeof(pthread_t));
        pthread_create(threads[i], NULL, threadBusyWait, NULL);
        subscribe(queue, *threads[i]);

        int *msg = malloc(sizeof(int));
        *msg = i * 10;
        addMsg(queue, msg);
    }

    setSize(queue, 5);
    for (int i = 0; i < 10; i++) {
        const int available = getAvailable(queue, *threads[i]);
        if (i < 5 && available != 5 || i >= 5 && available != 10 - i) {
            destroyQueue(queue);
            for (int j = 0; j < 10; j++) {
                free(threads[j]);
            }
            free(threads);

            *success = false;
            return success;
        }
    }

    destroyQueue(queue);
    for (int j = 0; j < 10; j++) {
        pthread_cancel(*threads[j]);
        free(threads[j]);
    }
    free(threads);

    return success;
}

void *sender(void *args) {
    for (int i = 0; i < 10; i++) {
        usleep(30 * 1000);
        char* msg = malloc(sizeof(char) * 10);
        snprintf(msg, 10, "%d", i);
        addMsg(args, msg);
    }

    return NULL;
}

void *subscriber(void *args) {
    pthread_t thread = pthread_self();
    subscribe(args, thread);

    char* result = malloc(sizeof(char) * 11);
    for (int i = 0; i < 10; i++) {
        result[i] = 0;
    }

    for (int i = 0; i < 10; i++) {
        char* msg = getMsg(args, thread);
        strcat(result, msg);
    }
    strcat(result, "\0");

    return result;
}

void *parallelSenderSubscriberTest(void *) {
    bool *success = malloc(sizeof(bool));
    *success = true;

    TQueue *queue = createQueue(10);

    pthread_t* subscriberThread = malloc(sizeof(pthread_t));
    pthread_create(subscriberThread, NULL, subscriber, queue);

    pthread_t* senderThread = malloc(sizeof(pthread_t));
    pthread_create(senderThread, NULL, sender, queue);

    char *result;
    pthread_join(*subscriberThread, (void**)&result);

    printf("result: %s\n", result);
    *success = strcmp(result, "0123456789") == 0;

    destroyQueue(queue);
    free(senderThread);
    free(subscriberThread);

    return success;
}

typedef struct {
    int time;
    TQueue *queue;
} TimeAndQueueArgs;

void *subscriberDelayed(void *args) {
    TimeAndQueueArgs *threadArgs = args;
    subscribe(threadArgs->queue, pthread_self());

    char* result = malloc(sizeof(char) * 11);
    for (int i = 0; i < 10; i++)
        result[i] = 0;


    for (int i = 0; i < 10; i++) {
        usleep(threadArgs->time);
        char* msg = getMsg(threadArgs->queue, pthread_self());
        strcat(result, msg);

    }

    strcat(result, "\0");

    return result;
}

void *parallelSenderSubscribersTest(void *) {
    bool *success = malloc(sizeof(bool));
    *success = true;

    TQueue *queue = createQueue(10);

    pthread_t* subscriberThread0 = malloc(sizeof(pthread_t));
    TimeAndQueueArgs *args0 = malloc(sizeof(TimeAndQueueArgs));
    args0->time = 100 * 1000;
    args0->queue = queue;
    pthread_create(subscriberThread0, NULL, subscriberDelayed, args0);

    pthread_t* subscriberThread1 = malloc(sizeof(pthread_t));
    TimeAndQueueArgs *args1 = malloc(sizeof(TimeAndQueueArgs));
    args1->time = 150 * 1000;
    args1->queue = queue;
    pthread_create(subscriberThread1, NULL, subscriberDelayed, args1);

    pthread_t* senderThread = malloc(sizeof(pthread_t));
    pthread_create(senderThread, NULL, sender, queue);

    char *result0;
    pthread_join(*subscriberThread0, (void**)&result0);

    char *result1;
    pthread_join(*subscriberThread1, (void**)&result1);

    *success =  strcmp(result0, "0123456789") == 0 && strcmp(result1, "0123456789") == 0;

    destroyQueue(queue);
    free(senderThread);
    free(subscriberThread0);
    free(subscriberThread1);
    free(args0);
    free(args1);

    return success;
}

typedef struct {
    long double arr[1000][1000][100];
} BigMemoryChunk;

const unsigned long int threshold = 0xFFF;
void *senderBigMemory(void *args) {
    for (unsigned long int i = 0; i < threshold; i++)
        addMsg(args, malloc(sizeof(BigMemoryChunk)));

    return NULL;
}

void *subscriberBigMemory(void *args) {
    subscribe(args, pthread_self());

    for (unsigned long int i = 0; i < threshold; i++)
        getMsg(args, pthread_self());

    return NULL;
}

void *garbageCollector(void *args) {
    subscribe(args, pthread_self());

    for (unsigned long int i = 0; i < threshold; i++)
        free(getMsg(args, pthread_self()));

    return NULL;
}

void *memoryLeakTest(void *) {
    bool *success = malloc(sizeof(bool));
    *success = true;

    TQueue *queue = createQueue(5);

    pthread_t* subscriberThread = malloc(sizeof(pthread_t));
    pthread_create(subscriberThread, NULL, subscriberBigMemory, queue);

    pthread_t* subscriberThread0 = malloc(sizeof(pthread_t));
    pthread_create(subscriberThread0, NULL, subscriberBigMemory, queue);

    pthread_t* subscriberThread1 = malloc(sizeof(pthread_t));
    pthread_create(subscriberThread1, NULL, subscriberBigMemory, queue);

    pthread_t* subscriberThread2 = malloc(sizeof(pthread_t));
    pthread_create(subscriberThread2, NULL, subscriberBigMemory, queue);

    pthread_t* garbageCollectorThread = malloc(sizeof(pthread_t));
    pthread_create(garbageCollectorThread, NULL, garbageCollector, queue);

    pthread_t* senderThread = malloc(sizeof(pthread_t));
    pthread_create(senderThread, NULL, senderBigMemory, queue);

    pthread_join(*senderThread, NULL);
    pthread_join(*garbageCollectorThread, NULL);

    free(senderThread);
    free(subscriberThread);
    free(subscriberThread0);
    free(subscriberThread1);
    free(subscriberThread2);
    free(garbageCollectorThread);

    destroyQueue(queue);

    return success;
}


int main() {
    /*
     *  tests group: 0
     */
    pthread_t t0;
    pthread_create(&t0, NULL, initTest, NULL);

    pthread_t t1;
    pthread_create(&t1, NULL, singleSubscriberMultipleMessagesSequentialReceivedTest, NULL);

    pthread_t t2;
    pthread_create(&t2, NULL, singleSubscriberMultipleMessagesSingleReceivedTest, NULL);

    pthread_t t3;
    pthread_create(&t3, NULL, multipleSubscribersSingleMessageTest, NULL);

    pthread_t t4;
    pthread_create(&t4, NULL, multipleSubscribersMultipleMessagesTest, NULL);

    pthread_t t5;
    pthread_create(&t5, NULL, changeSizeTest, NULL);

    pthread_t t6;
    pthread_create(&t6, NULL, parallelSenderSubscriberTest, NULL);

    bool *result0;
    pthread_join(t0, (void**)&result0);
    printf("initTest: %s\n", *result0 ? "success" : "failed");

    bool *result1;
    pthread_join(t1, (void**)&result1);
    printf("singleSubscriberMultipleMessagesSequentialReceivedTest: %s\n", *result1 ? "success" : "failed");

    bool *result2;
    pthread_join(t2, (void**)&result2);
    printf("singleSubscriberMultipleMessagesSingleReceivedTest: %s\n", *result2 ? "success" : "failed");

    bool *result3;
    pthread_join(t3, (void**)&result3);
    printf("multipleSubscribersSingleMessageTest: %s\n", *result3 ? "success" : "failed");

    bool *result4;
    pthread_join(t4, (void**)&result4);
    printf("multipleSubscribersMultipleMessagesTest: %s\n", *result4 ? "success" : "failed");

    bool *result5;
    pthread_join(t5, (void**)&result5);
    printf("changeSizeTest: %s\n", *result5 ? "success" : "failed");

    bool *result6;
    pthread_join(t6, (void**)&result6);
    printf("parallelSenderSubscriberTest: %s\n", *result6 ? "success" : "failed");

    /*
     *  tests group: 1
     */
    pthread_t t7;
    pthread_create(&t7, NULL, parallelSenderSubscribersTest, NULL);

    pthread_t t8;
    pthread_create(&t8, NULL, memoryLeakTest, NULL);

    bool *result7;
    pthread_join(t7, (void**)&result7);
    printf("parallelSenderSubscribersTest: %s\n", *result7 ? "success" : "failed");

    bool *result8;
    pthread_join(t8, (void**)&result8);
    printf("memoryLeakTest: %s\n", *result8 ? "success" : "failed");

    return 0;
}