#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "safequeue.h"

// 创建一个新的优先队列
PriorityQueue* create_queue(int capacity) {
    PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    pq->items = (PQItem*)malloc(sizeof(PQItem) * capacity);
    pq->capacity = capacity;
    pq->size = 0;
    return pq;
}


void copy_info(request_info *a, request_info b) {
    a->client_fd = b.client_fd;
    a->delay = b.delay;
    if (b.path != NULL) {
        a->path = (char *)malloc(strlen(b.path) + 1);
        if (a->path == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        strcpy(a->path, b.path);
    } else {
        a->path = NULL;
    }
    if (b.buffer != NULL) {
        a->buffer = (char *)malloc(strlen(b.buffer) + 1);
        if (a->buffer == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        strcpy(a->buffer, b.buffer);
    } else {
        a->buffer = NULL;
    }
}
// 重新分配队列的大小
// void resizePriorityQueue(PriorityQueue* pq, int new_capacity) {
//     pq->items = (PQItem*)realloc(pq->items, sizeof(PQItem) * new_capacity);
//     pq->capacity = new_capacity;
// }


// 插入操作
void add_work(PriorityQueue* pq, request_info value, int priority) {
    // if (pq->size == pq->capacity) {
    //     resizePriorityQueue(pq, pq->capacity * 2);
    // }

    // insert the new item to tail
    int i = pq->size++;
    copy_info(&pq->items[i].value, value);
    pq->items[i].priority = priority;

    // resort the queue
    while (i != 0 && pq->items[i].priority > pq->items[(i - 1) / 2].priority) {
        PQItem temp = pq->items[i];
        pq->items[i] = pq->items[(i - 1) / 2];
        pq->items[(i - 1) / 2] = temp;
        i = (i - 1) / 2;
    }
}


// dequeue
int dequeue(PriorityQueue* pq, request_info *work) {
    if (pq->size == 0) {
        printf("Queue is empty\n");
        return -1;
    }

    copy_info(work, pq->items[0].value);
    free(pq->items[0].value.path);
    // free(pq->items[0].value.buffer);

    // update size and heap
    pq->size--;
    pq->items[0].priority = pq->items[pq->size].priority;
    copy_info(&pq->items[0].value, pq->items[pq->size].value);

    // pq->items[0] = pq->items[--pq->size];

    // work is the value for return, as a pointer
    // resort the queue
    int i = 0;
    while ((2 * i + 1) < pq->size) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int largest = i;

        if (left < pq->size && pq->items[left].priority > pq->items[i].priority) {
            largest = left;
        }
        if (right < pq->size && pq->items[right].priority > pq->items[largest].priority) {
            largest = right;
        }
        if (largest == i) {
            break;
        }

        PQItem temp = pq->items[i];
        pq->items[i] = pq->items[largest];
        pq->items[largest] = temp;
        i = largest;
    }

    // dequeue seccesfully
    return 1;
}


int get_work(PriorityQueue *pq, request_info *work) {
    if (dequeue(pq, work) == 1)
        return 1;
    // should block workthread itself
    // should cond_wait
    return -1;
}

int get_work_nonblocking(PriorityQueue *pq, request_info *work) {
    if (dequeue(pq, work) == 1)
        return 1;
    // called by listen thread and won't block
    return -1;
}