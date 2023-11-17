#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H


typedef struct 
{
    int delay;
    int client_fd;
    char* path;
    char* buffer;
} request_info;




// Item
typedef struct {
    request_info value;      // work
    int priority;   // priority
} PQItem;



// PQ
typedef struct {
    PQItem* items;   // works
    int capacity;    // max capacity
    int size;        // current size of queue
} PriorityQueue;


PriorityQueue* create_queue(int capacity);
void add_work(PriorityQueue* pq, request_info value, int priority);
int dequeue(PriorityQueue* pq, request_info*);
int get_work(PriorityQueue *pq, request_info *work);
int get_work_nonblocking(PriorityQueue *pq, request_info *work);


#endif // SAFEQUEUE_H
