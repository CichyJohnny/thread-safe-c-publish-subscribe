#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "queue.h"


void* subscribeReadUnsubscribe_receiver(void* args) {
    TQueue *queue = (TQueue*)args;

    subscribe(queue, pthread_self());

    bool check = true;

    for (int i = 0; i<20; i++) {
        int *msg = (int*)getMsg(queue, pthread_self());

        if (msg == NULL || *msg != i % 10) {
            printf("%d - %d instead of %d\n", i, *msg, i % 10);
            check = false;
        }
    }

    unsubscribe(queue, pthread_self());

    if (!check) {
        void *ret = (void*)false;
        return ret;
    }
    void *ret = (void*)true;
    return ret;
}

void* subscribeReadUnsubscribe_sender(void* args) {
    TQueue *queue = (TQueue*)args;

    pthread_t receivers[9];
    int *msgs[100];

    for (int i = 0; i<10; i++) {

        if (i != 9) {
            pthread_create(&receivers[i], NULL, subscribeReadUnsubscribe_receiver, queue);

            usleep(1000);
        }

        for (int j = 0; j<10; j++) {
            msgs[i * 10 + j] = (int*)malloc(sizeof(int));
            *msgs[i * 10 + j] = j;

            addMsg(queue, msgs[i * 10 + j]);
        }
    }

    bool results[9];
    for (int i = 0; i<9; i++) {
        pthread_join(receivers[i], (void**)&results[i]);
    }
    for (int i = 0; i<100; i++) {
        free(msgs[i]);
    }

    bool check = true;
    for (int i = 0; i<9; i++) {
        if (!results[i]) {
            check = false;
        }
    }

    if (!check) {
        void *ret = (void*)false;
        return ret;
    }
    void *ret = (void*)true;
    return ret;
}

bool subscribeReadUnsubscribe() {
    TQueue *queue = createQueue(20);

    pthread_t sender;
    pthread_create(&sender, NULL, subscribeReadUnsubscribe_sender, queue);

    bool result;
    pthread_join(sender, (void**)&result);

    return result;
}


int main() {
    if (!subscribeReadUnsubscribe()) {
        printf("subscribeReadUnsubscribe failed\n");
        return 1;
    }
    printf("subscribeReadUnsubscribe passed\n");

    return 0;
}