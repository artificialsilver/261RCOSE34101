#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

/* 정수 큐의 시작 위치와 삽입 위치를 초기 상태로 맞춘다. */
void queue_init(IntQueue *q) {
    q->front = 0;
    q->rear = 0;
}

int queue_empty(IntQueue *q) {
    return q->front == q->rear;
}

int queue_full(IntQueue *q) {
    return q->rear >= MAX_TIME;
}

/* ready queue 뒤쪽에 프로세스 배열 index를 추가한다. */
void queue_push(IntQueue *q, int value) {
    if (queue_full(q)) {
        printf("Queue overflow\n");
        exit(1);
    }

    q->items[q->rear++] = value;
}

/* ready queue 앞쪽에서 다음 실행 후보 index를 꺼낸다. */
int queue_pop(IntQueue *q) {
    if (queue_empty(q)) {
        printf("Queue underflow\n");
        exit(1);
    }

    return q->items[q->front++];
}
