#pragma once
#include "pdu.h"
#include <semaphore.h>
#include <pthread.h>

struct Node {
    Frame *frame;
    Node *next = NULL;
};

class SharedMemory {
    public:
        SharedMemory();

        void enqueue(Frame* frame);

        Frame* dequeue();

        Frame* peek();

    private:
        pthread_mutex_t mtx;
        Node* head;
        Node* tail;
};