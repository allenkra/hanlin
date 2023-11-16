#include <stdio.h>
#include <stdlib.h>
#include "safequeue.h"

// 创建一个新的优先队列
PriorityQueue* create_queue(int capacity) {
    PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    pq->items = (PQItem*)malloc(sizeof(PQItem) * capacity);
    pq->capacity = capacity;
    pq->size = 0;
    return pq;
}

// 重新分配队列的大小
// void resizePriorityQueue(PriorityQueue* pq, int new_capacity) {
//     pq->items = (PQItem*)realloc(pq->items, sizeof(PQItem) * new_capacity);
//     pq->capacity = new_capacity;
// }

// 插入操作
void enqueue(PriorityQueue* pq, int value, int priority) {
    // if (pq->size == pq->capacity) {
    //     resizePriorityQueue(pq, pq->capacity * 2);
    // }

    // insert the new item to tail
    int i = pq->size++;
    pq->items[i].value = value;
    pq->items[i].priority = priority;

    // resort the queue
    while (i != 0 && pq->items[i].priority < pq->items[(i - 1) / 2].priority) {
        PQItem temp = pq->items[i];
        pq->items[i] = pq->items[(i - 1) / 2];
        pq->items[(i - 1) / 2] = temp;
        i = (i - 1) / 2;
    }
}


// dequeue
int dequeue(PriorityQueue* pq) {
    if (pq->size == 0) {
        printf("Queue is empty\n");
        return -1;
    }

    PQItem root = pq->items[0];
    pq->items[0] = pq->items[--pq->size];

    // resort the queue
    int i = 0;
    while ((2 * i + 1) < pq->size) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;

        if (pq->items[left].priority < pq->items[i].priority) {
            smallest = left;
        }
        if (right < pq->size && pq->items[right].priority < pq->items[smallest].priority) {
            smallest = right;
        }
        if (smallest == i) {
            break;
        }

        PQItem temp = pq->items[i];
        pq->items[i] = pq->items[smallest];
        pq->items[smallest] = temp;
        i = smallest;
    }

    return root.value;
}



// // 主函数用于测试优先队列
// int main() {
//     PriorityQueue* pq = create_queue(5);
//     enqueue(pq, 10, 2);
//     enqueue(pq, 15, 1);
//     enqueue(pq, 20, 3);

//     printf("Dequeued element: %d\n", dequeue(pq));
//     printf("Dequeued element: %d\n", dequeue(pq));
//     printf("Dequeued element: %d\n", dequeue(pq));

//     return 0;
// }
