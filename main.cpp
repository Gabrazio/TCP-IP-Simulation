#include <iostream>
#include <string>
#include <vector>
#include <pthread.h>
#include <time.h>
#include <cstdlib>
#include "layer.h"
#include "registry.h"
#include "pdu.h"
#include "shared_memory.h"
#include "device.h"
#include "globals.h"

using namespace std;

vector<Address*> global_addresses;

void* func(void* args) {
    srand(time(0) ^ pthread_self());
    ThreadData* td = (ThreadData*) args;

    Address* dst;
    do {
        dst = global_addresses[rand() % global_addresses.size()];
    } while(dst->src_mac == td->address->src_mac);

    Device Device(
        td->address->src_port, dst->src_port,
        td->address->src_ip,   dst->src_ip,
        td->address->src_mac,  dst->src_mac,
        td->shared_memory
    );

    while(true) {
            if(rand() % 4 == 0) {
                do {
                    dst = global_addresses[rand() % global_addresses.size()];
                } while(dst->src_mac == td->address->src_mac);
                Device.send("Hello from DEV" + to_string(pthread_self()));
            }

            Device.receive();
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc != 2) {
        cout << "Uso: " << argv[0] << " <NUM_DEVICES>" << endl;
        return EXIT_FAILURE;
    }

    system("cls");

    SharedMemory* shared_memory = new SharedMemory();
    int n = atoi(argv[1]);
    Registry& registry = Registry::getInstance();
    pthread_t* threads = new pthread_t[n];

    for(int i = 0; i < n; i++) {
        Address* addr = new Address{
            registry.getPort(),
            registry.getIpAddress(),
            registry.getMacAddress()
        };
        global_addresses.push_back(addr);
        ThreadData* td = new ThreadData{ shared_memory, addr };
        pthread_create(&threads[i], NULL, func, td);
    }

    for(int i = 0; i < n; i++)
        pthread_join(threads[i], NULL);

    delete[] threads;
    return EXIT_SUCCESS;
}