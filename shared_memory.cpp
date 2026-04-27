#include "shared_memory.h"

SharedMemory::SharedMemory() : head(NULL), tail(NULL) {
            pthread_mutex_init(&mtx, NULL);
        }

        void SharedMemory::enqueue(Frame* frame) {
            pthread_mutex_lock(&mtx);
                Node* node = new Node();
                node->frame = frame;
                if(tail == NULL) {
                    head = node;
                    tail = node;
                } else {
                    tail->next = node;
                    tail = node;
                }
            pthread_mutex_unlock(&mtx);
        }

        Frame* SharedMemory::dequeue() {
            pthread_mutex_lock(&mtx);
                if(head == NULL) {
                    pthread_mutex_unlock(&mtx);
                    return NULL;
                }
                Node* temp = head;
                head = head->next;
                if(head == NULL) tail = NULL;
                Frame* frame = temp->frame;
                delete temp;
            pthread_mutex_unlock(&mtx);
            return frame;
        }

        Frame* SharedMemory::peek() {
            pthread_mutex_lock(&mtx);
                Frame* frame = (head != NULL) ? head->frame : NULL;
            pthread_mutex_unlock(&mtx);
            return frame;
        }