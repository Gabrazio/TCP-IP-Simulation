#pragma once
#include "layer.h"

class Device {
    public:
        Device(int src_port, int dst_port,
               string src_ip,  string dst_ip,
               string src_mac, string dst_mac,
               SharedMemory* shared_memory);

        void send(string message);

        void receive();

    private:
        Application   *application;
        Transport     *transport;
        Internetwork  *internetwork;
        NetworkAccess *network_access;
};