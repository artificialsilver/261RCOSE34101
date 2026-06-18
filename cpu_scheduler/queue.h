#ifndef QUEUE_H
#define QUEUE_H

#include "types.h"

/* FCFS와 Round Robin에서 준비된 프로세스의 배열 index를 보관하는 큐이다. */
typedef struct {
    int items[MAX_TIME];
    int front;
    int rear;
} IntQueue;

void queue_init(IntQueue *q);
int queue_empty(IntQueue *q);
int queue_full(IntQueue *q);
void queue_push(IntQueue *q, int value);
int queue_pop(IntQueue *q);

#endif
