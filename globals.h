#pragma once
#include "shared_memory.h"
#include <string>

struct Address {
    int src_port;
    std::string src_ip;
    std::string src_mac;
};

struct ThreadData {
    SharedMemory* shared_memory;
    Address* address;
};