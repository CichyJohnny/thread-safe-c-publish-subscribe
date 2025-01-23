#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "queue.h"

bool initTest() {
    TQueue *queue = createQueue(10);
    if (queue == NULL) {
        destroyQueue(queue);
        return false;
    }

    return true;
}

bool subscribeTest() {
    TQueue *queue = createQueue(10);
    subscribe(queue, pthread_self());

    if (queue->subscriber_count != 1) {
        destroyQueue(queue);
        return false;
    }
    if (queue->subscribers_head == NULL) {
        destroyQueue(queue);
        return false;
    }
    if (queue->subscribers_head->thread != pthread_self()) {
        destroyQueue(queue);
        return false;
    }
    if (queue->subscribers_head->next != NULL) {
        destroyQueue(queue);
        return false;
    }
    destroyQueue(queue);

    return true;
}

bool multipleSubscribersTest() {
    TQueue *queue = createQueue(10);
    pthread_t thread1, thread2, thread3;
    subscribe(queue, thread1);
    subscribe(queue, thread2);
    subscribe(queue, thread3);

    if (queue->subscriber_count != 3) {
        destroyQueue(queue);
        return false;
    }

    if (queue->subscribers_head == NULL) {
        destroyQueue(queue);
        return false;
    }
    if (queue->subscribers_head->thread != thread1) {
        destroyQueue(queue);
        return false;
    }

    if (queue->subscribers_head->next == NULL) {
        destroyQueue(queue);
        return false;
    }
    if (queue->subscribers_head->next->thread != thread2) {
        destroyQueue(queue);
        return false;
    }

    if (queue->subscribers_head->next->next == NULL) {
        destroyQueue(queue);
        return false;
    }
    if (queue->subscribers_head->next->next->thread != thread3) {
        destroyQueue(queue);
        return false;
    }

    if (queue->subscribers_head->next->next->next != NULL) {
        destroyQueue(queue);
        return false;
    }

    destroyQueue(queue);

    return true;
}

bool subscribeSelfTwice() {
    TQueue *queue = createQueue(10);
    subscribe(queue, pthread_self());
    subscribe(queue, pthread_self());

    if (queue->subscriber_count != 1) {
        printf("1\n");
        destroyQueue(queue);
        return false;
    }

    if (queue->subscribers_head == NULL) {
        printf("2\n");
        destroyQueue(queue);
        return false;
    }
    if (queue->subscribers_head->thread != pthread_self()) {
        printf("3\n");
        destroyQueue(queue);
        return false;
    }
    if (queue->subscribers_head->next != NULL) {
        printf("4\n");
        destroyQueue(queue);
        return false;
    }

    destroyQueue(queue);

    return true;
}

bool unsubscribeTest() {
    TQueue *queue = createQueue(10);
    pthread_t thread1, thread2, thread3;
    subscribe(queue, thread1);
    subscribe(queue, thread2);
    subscribe(queue, thread3);

    unsubscribe(queue, thread2);

    if (queue->subscriber_count != 2) {
        destroyQueue(queue);
        return false;
    }

    if (queue->subscribers_head == NULL) {
        destroyQueue(queue);
        return false;
    }
    if (queue->subscribers_head->thread != thread1) {
        destroyQueue(queue);
        return false;
    }

    if (queue->subscribers_head->next == NULL) {
        destroyQueue(queue);
        return false;
    }
    if (queue->subscribers_head->next->thread != thread3) {
        destroyQueue(queue);
        return false;
    }

    if (queue->subscribers_head->next->next != NULL) {
        destroyQueue(queue);
        return false;
    }

    destroyQueue(queue);

    return true;
}

bool messagesAfterUnsubscribe() {
    TQueue *queue = createQueue(10);

    int *msg1 = malloc(sizeof(int));
    *msg1 = 10;
    addMsg(queue, msg1);

    if (queue->size != 0) {
        printf("1\n");
        destroyQueue(queue);
        free(msg1);
        return false;
    }

    pthread_t thread1;
    subscribe(queue, thread1);

    addMsg(queue, msg1);

    unsubscribe(queue, thread1);

    if (queue->size != 0) {
        printf("2\n");
        destroyQueue(queue);
        free(msg1);
        return false;
    }

    pthread_t thread2, thread3;
    subscribe(queue, thread1);
    subscribe(queue, thread2);

    addMsg(queue, msg1);
    addMsg(queue, msg1);
    addMsg(queue, msg1);

    getMsg(queue, thread2);
    getMsg(queue, thread2);

    if (queue->size != 3 || getAvailable(queue, thread1) != 3 || getAvailable(queue, thread2) != 1) {
        printf("3\n");
        destroyQueue(queue);
        free(msg1);
        return false;
    }

    unsubscribe(queue, thread1);

    if (queue->size != 1 || getAvailable(queue, thread2) != 1) {
        printf("4\n");
        destroyQueue(queue);
        free(msg1);
        return false;
    }

    destroyQueue(queue);
    free(msg1);

    return true;
}

bool unsubscribeFromEmptyTest() {
    TQueue *queue = createQueue(10);
    unsubscribe(queue, pthread_self());

    return true;
}

bool addMsgTest() {
    TQueue *queue = createQueue(10);
    subscribe(queue, pthread_self());
    int *msg = malloc(sizeof(int));
    *msg = 10;
    addMsg(queue, msg);

    if (queue->size != 1) {
        destroyQueue(queue);
        free(msg);
        return false;
    }
    if (queue->messages_head[0].data != msg) {
        destroyQueue(queue);
        free(msg);
        return false;
    }
    if (queue->messages_head[0].undelivered != queue->subscriber_count) {
        destroyQueue(queue);
        free(msg);
        return false;
    }
    if (queue->subscribers_head->new_messages != 1) {
        destroyQueue(queue);
        free(msg);
        return false;
    }

    destroyQueue(queue);
    free(msg);

    return true;
}

bool getAvaibleTest() {
    TQueue *queue = createQueue(10);
    subscribe(queue, pthread_self());
    int *msg = malloc(sizeof(int));
    *msg = 10;
    addMsg(queue, msg);

    if (getAvailable(queue, pthread_self()) != 1) {
        destroyQueue(queue);
        free(msg);
        return false;
    }

    addMsg(queue, msg);
    if (getAvailable(queue, pthread_self()) != 2) {
        destroyQueue(queue);
        free(msg);
        return false;
    }

    addMsg(queue, msg);
    if (getAvailable(queue, pthread_self()) != 3) {
        destroyQueue(queue);
        free(msg);
        return false;
    }
    getMsg(queue, pthread_self());
    if (getAvailable(queue, pthread_self()) != 2) {
        destroyQueue(queue);
        free(msg);
        return false;
    }

    destroyQueue(queue);
    free(msg);

    return true;
}

void* overflowTest_sender(void *args) {
    TQueue *queue = (TQueue *)args;

    int *msgs = malloc(3*sizeof(int));
    msgs[0] = 10;
    msgs[1] = 20;
    msgs[2] = 30;
    addMsg(queue, &msgs[0]);
    addMsg(queue, &msgs[1]);
    addMsg(queue, &msgs[2]);

    return msgs;
}

bool overflowTest() {
    TQueue *queue = createQueue(1);
    subscribe(queue, pthread_self());

    pthread_t sender;
    pthread_create(&sender, NULL, overflowTest_sender, queue);

    usleep(1000);
    if (queue->size != 1) {
        printf("1\n");
        destroyQueue(queue);
        return false;
    }
    if (getAvailable(queue, pthread_self()) != 1) {
        printf("2\n");
        destroyQueue(queue);
        return false;
    }
    if (queue->messages_head->undelivered != queue->subscriber_count) {
        printf("3\n");
        destroyQueue(queue);
        return false;
    }
    
    int *msg = (int*)getMsg(queue, pthread_self());

    usleep(1000);
    if (queue->size != 1) {
        printf("4\n");
        destroyQueue(queue);
        return false;
    }
    if (getAvailable(queue, pthread_self()) != 1) {
        printf("5\n");
        printf("%d\n", getAvailable(queue, pthread_self()));
        destroyQueue(queue);
        return false;
    }
    if (queue->messages_head[0].undelivered != queue->subscriber_count) {
        printf("6\n");
        destroyQueue(queue);
        return false;
    }
    if (msg == NULL || *msg != 10) {
        printf("7\n");
        destroyQueue(queue);
        return false;
    }

    msg = (int*)getMsg(queue, pthread_self());

    usleep(1000);
    if (queue->size != 1) {
        printf("8\n");
        destroyQueue(queue);
        return false;
    }
    if (getAvailable(queue, pthread_self()) != 1) {
        printf("9\n");
        destroyQueue(queue);
        return false;
    }
    if (queue->messages_head[0].undelivered != queue->subscriber_count) {
        printf("10\n");
        destroyQueue(queue);
        return false;
    }
    if (msg == NULL || *msg != 20) {
        printf("11\n");
        destroyQueue(queue);
        return false;
    }
    msg = (int*)getMsg(queue, pthread_self());

    usleep(1000);
    if (queue->size != 0) {
        printf("12\n");
        destroyQueue(queue);
        return false;
    }
    if (getAvailable(queue, pthread_self()) != 0) {
        printf("13\n");
        destroyQueue(queue);
        return false;
    }
    if (msg == NULL || *msg != 30) {
        printf("14\n");
        destroyQueue(queue);
        return false;
    }

    void *msgs;
    pthread_join(sender, &msgs);

    free(msgs);
    destroyQueue(queue);

    return true;
}

bool subscriberNotFoundTest() {
    TQueue *queue = createQueue(10);
    pthread_t subscriber, not_subscriber;
    subscribe(queue, subscriber);

    int *msg = malloc(sizeof(int));
    *msg = 10;
    addMsg(queue, msg);

    unsubscribe(queue, not_subscriber);

    if (getMsg(queue, not_subscriber) != NULL) {
        destroyQueue(queue);
        free(msg);
        return false;
    }
    if (getAvailable(queue, not_subscriber) != 0) {
        destroyQueue(queue);
        free(msg);
        return false;
    }

    destroyQueue(queue);
    free(msg);

    return true;
}

void* getMsgFromEmptyTest_sender(void *args) {
    TQueue *queue = (TQueue *)args;
    int *msg1 = malloc(sizeof(int));
    *msg1 = 10;
    usleep(10000);
    addMsg(queue, msg1);

    return msg1;
}

bool getMsgFromEmptyTest() {
    TQueue *queue = createQueue(10);
    subscribe(queue, pthread_self());

    if (getAvailable(queue, pthread_self()) != 0) {
        destroyQueue(queue);
        return false;
    }

    pthread_t sender;
    pthread_create(&sender, NULL, getMsgFromEmptyTest_sender, queue);

    if (*(int*)getMsg(queue, pthread_self()) != 10) {
        destroyQueue(queue);
        return false;
    }

    void *msg;
    pthread_join(sender, &msg);
    free(msg);
    destroyQueue(queue);

    return true;
}

void *multipleReceiverTest_senderWithDelay(void* args) {
    usleep(1000);
    TQueue *queue = (TQueue*)args;

    int *msgs = malloc(10 * sizeof(int));
    for (int i = 0; i<10; i++) {
        msgs[i] = (i + 1) * 10;
        usleep(10000);
        addMsg(queue, (void *)(intptr_t)msgs[i]);
    }

    return msgs;
}

void *multipleReceiverTest_sender(void* args) {
    usleep(1000);
    TQueue *queue = (TQueue*)args;

    int *msgs = malloc(10 * sizeof(int));
    for (int i = 0; i<10; i++) {
        msgs[i] = (i + 1) * 10;
        addMsg(queue, (void*)(intptr_t)msgs[i]);
    }

    return msgs;
}

void *multipleReceiverTest_receiver(void* args) {
    TQueue *queue = (TQueue*)args;
    subscribe(queue, pthread_self());

    int *msgs = malloc(10 * sizeof(int));
    for (int i = 0; i<10; i++) {
        msgs[i] = (int)(intptr_t)getMsg(queue, pthread_self());
    }

    return msgs;
}
void *multipleReceiverTest_receiverWithDelay(void* args) {
    TQueue *queue = (TQueue*)args;
    subscribe(queue, pthread_self());

    int *msgs = malloc(10 * sizeof(int));
    for (int i = 0; i<10; i++) {        
        usleep(10000);
        msgs[i] = (int)(intptr_t)getMsg(queue, pthread_self());
    }

    return msgs;
}

bool multipleReceiverTest() {
    TQueue *queue = createQueue(5);

    pthread_t sender1, receiver1, receiver2;

    pthread_create(&receiver1, NULL, multipleReceiverTest_receiver, queue);
    pthread_create(&receiver2, NULL, multipleReceiverTest_receiver, queue);
    pthread_create(&sender1, NULL, multipleReceiverTest_senderWithDelay, queue);

    void *received1, *received2, *sent1;
    pthread_join(receiver1, &received1);
    pthread_join(receiver2, &received2);
    pthread_join(sender1, &sent1);

    for (int i = 0; i<10; i++) {
        int a = ((int*)received1)[i];
        int b = ((int*)received2)[i];
        if (a != (i + 1) * 10 || b != (i + 1) * 10) {
            free(received1);
            free(received2);
            free(sent1);
            destroyQueue(queue);
            return false;
        }
    }

    free(received1);
    free(received2);
    free(sent1);
    destroyQueue(queue);

    TQueue *queue2 = createQueue(5);
    pthread_t sender2, receiver3, receiver4;

    pthread_create(&sender2, NULL, multipleReceiverTest_sender, queue2);
    pthread_create(&receiver3, NULL, multipleReceiverTest_receiverWithDelay, queue2);
    pthread_create(&receiver4, NULL, multipleReceiverTest_receiverWithDelay, queue2);

    void *received3, *received4, *sent2;
    pthread_join(sender2, &sent2);
    pthread_join(receiver3, &received3);
    pthread_join(receiver4, &received4);

    for (int i = 0; i<10; i++) {
        int a = ((int*)received3)[i];
        int b = ((int*)received4)[i];
        if (((int*)received3)[i] != (i + 1) * 10 || ((int*)received4)[i] != (i + 1) * 10) {
            free(received3);
            free(received4);
            free(sent2);
            destroyQueue(queue2);
            return false;
        }
    }

    free(received3);
    free(received4);
    free(sent2);
    destroyQueue(queue2);

    return true;
}

bool setSizeTest() {
    TQueue *queue = createQueue(10);

    if (queue->capacity != 10) {
        destroyQueue(queue);
        return false;
    }
    setSize(queue, 20);
    if (queue->capacity != 20) {
        destroyQueue(queue);
        return false;
    }
    destroyQueue(queue);

    return true;
}

bool decreaseSetSize() {
    TQueue *queue = createQueue(4);
    pthread_t thread1, thread2;

    int *msg1 = malloc(sizeof(int));
    *msg1 = 10;
    int *msg2 = malloc(sizeof(int));
    *msg2 = 20;
    int *msg3 = malloc(sizeof(int));
    *msg3 = 30;

    subscribe(queue, thread1);
    addMsg(queue, msg1);

    subscribe(queue, thread2);
    addMsg(queue, msg2);
    addMsg(queue, msg3);

    if (queue->size != 3 || getAvailable(queue, thread1) != 3 || getAvailable(queue, thread2) != 2) {
        printf("1\n");
        free(msg1);
        free(msg2);
        free(msg3);
        destroyQueue(queue);
        return false;
    }

    setSize(queue, 1);

    if (queue->size != 1 || getAvailable(queue, thread1) != 1 || getAvailable(queue, thread1) != 1) {
        printf("2\n");
        free(msg1);
        free(msg2);
        free(msg3);
        destroyQueue(queue);
        return false;
    }
    int *aa = (int*)getMsg(queue, thread1);
    int *bb = (int*)getMsg(queue, thread2);

    if (*aa != 30 || *bb != 30) {
        printf("3\n");
        free(msg1);
        free(msg2);
        free(msg3);
        destroyQueue(queue);
        return false;
    }
    if (queue->size != 0 || getAvailable(queue, thread1) != 0 || getAvailable(queue, thread1) != 0) {
        printf("4\n");
        free(msg1);
        free(msg2);
        free(msg3);
        destroyQueue(queue);
        return false;
    }

    free(msg1);
    free(msg2);
    free(msg3);
    destroyQueue(queue);

    return true;
}

void *increaseSetSizeTest_sender(void *args) {
    TQueue *queue = (TQueue *)args;
    subscribe(queue, pthread_self());

    int *msg1 = malloc(sizeof(int));
    *msg1 = 10;
    int *msg2 = malloc(sizeof(int));
    *msg2 = 20;
    int *msg3 = malloc(sizeof(int));
    *msg3 = 30;
    addMsg(queue, msg1);
    addMsg(queue, msg2);
    addMsg(queue, msg3);

    usleep(1000);
    free(msg1);
    free(msg2);
    free(msg3);
}

bool increaseSetSizeTest() {
    TQueue *queue = createQueue(1);
    subscribe(queue, pthread_self());
    
    pthread_t sender;
    pthread_create(&sender, NULL, increaseSetSizeTest_sender, queue);

    if(*(int*)getMsg(queue, pthread_self()) != 10) {
        printf("2\n");
        destroyQueue(queue);
        return false;
    }

    setSize(queue, 3);

    if (queue->capacity != 3) {
        printf("3\n");
        destroyQueue(queue);
        return false;
    }

    int *aa = (int*)getMsg(queue, pthread_self());
    int *bb = (int*)getMsg(queue, pthread_self());
    if (*aa != 20 || *bb != 30) {
        printf("4\n");
        destroyQueue(queue);
        return false;
    }
    destroyQueue(queue);

    return true;
}

void* deadLockTest_sender(void* args) {
    TQueue *queue = (TQueue*)args;

    int *msg1 = malloc(sizeof(int));
    *msg1 = 10;

    usleep(1000);
    printf("sending\n");
    addMsg(queue, msg1);
    printf("sent\n");
}

void* deadLockTest_subscriber(void* args) {
    TQueue *queue = (TQueue*)args;
    subscribe(queue, pthread_self());

    printf("receiving\n");
    getMsg(queue, pthread_self());
    printf("received\n");
}

int main() {
    if (!initTest()) {
        printf("initTest failed\n");
        return 1;
    }
    printf("initTest passed\n");
    if (!subscribeTest()) {
        printf("subscribeTest failed\n");
        return 1;
    }
    printf("subscribeTest passed\n");
    if (!multipleSubscribersTest()) {
        printf("multipleSubscribersTest failed\n");
        return 1;
    }
    printf("multipleSubscribersTest passed\n");
    if (!subscribeSelfTwice()) {
        printf("subscribeSelfTwice failed\n");
        return 1;
    }
    printf("subscribeSelfTwice passed\n");
    if (!unsubscribeTest()) {
        printf("unsubscribeTest failed\n");
        return 1;
    }
    printf("unsubscribeTest passed\n");
    if (!messagesAfterUnsubscribe()) {
        printf("messagesAfterUnsubscribe failed\n");
        return 1;
    }
    printf("messagesAfterUnsubscribe passed\n");
    if (!unsubscribeFromEmptyTest()) {
        printf("unsubscribeFromEmptyTest failed\n");
        return 1;
    }
    printf("unsubscribeFromEmptyTest passed\n");
    if (!addMsgTest()) {
        printf("addMsgTest failed\n");
        return 1;
    }
    printf("addMsgTest passed\n");
    if (!getAvaibleTest()) {
        printf("getAvaibleTest failed\n");
        return 1;
    }
    printf("getAvaibleTest passed\n");
    if (!overflowTest()) {
        printf("overflowTest failed\n");
        return 1;
    }
    printf("overflowTest passed\n");
    if (!subscriberNotFoundTest()) {
        printf("subscriberNotFoundTest failed\n");
        return 1;
    }
    printf("subscriberNotFoundTest passed\n");
    if (!getMsgFromEmptyTest()) {
        printf("getMsgFromEmptyTest failed\n");
        return 1;
    }
    printf("getMsgFromEmptyTest passed\n");
    if (!multipleReceiverTest()) {
        printf("multipleReceiverTest failed\n");
        return 1;
    }
    printf("multipleReceiverTest passed\n");
    if (!setSizeTest()) {
        printf("setSizeTest failed\n");
        return 1;
    }
    printf("setSizeTest passed\n");
    if (!decreaseSetSize()) {
        printf("decreaseSetSize failed\n");
        return 1;
    }
    printf("decreaseSetSize passed\n");
    if (!increaseSetSizeTest()) {
        printf("increaseSetSizeTest failed\n");
        return 1;
    }
    printf("increaseSetSizeTest passed\n");
    
    return 0;
}