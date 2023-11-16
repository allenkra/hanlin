#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H

#include <stdio.h>
#include <stdlib.h>

// Item
typedef struct {
    int value;      // value
    int priority;   // priority
} PQItem;

// PQ
typedef struct {
    PQItem* items;   // works
    int capacity;    // max capacity
    int size;        // current size of queue
} PriorityQueue;


PriorityQueue* create_queue(int capacity);
void enqueue(PriorityQueue* pq, int value, int priority);
int dequeue(PriorityQueue* pq);


#endif // SAFEQUEUE_H
