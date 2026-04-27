#pragma once
#include "shared_memory.h"
#include <vector>
#include <string>

using namespace std;

class Transport;
class Internetwork;
class NetworkAccess;

class Layer {
    public:
        virtual void encapsulate(PDU* pdu) = 0;
        virtual PDU* decapsulate(PDU* pdu) = 0;
        virtual void setLayers(Layer* top, Layer* bottom) = 0;
        virtual ~Layer() = default;
};

class Application: public Layer {
    public:
        Application();
        void encapsulate(PDU* pdu);
        PDU* decapsulate(PDU* pdu);
        void setLayers(Layer* top, Layer* bottom);
    private:
        Transport *transport;
};

class Transport: public Layer {
    public:
        Transport(int src_port, int dst_port) : src_port(src_port), dst_port(dst_port) {}
        void encapsulate(PDU* pdu);
        PDU* decapsulate(PDU* pdu);
        void setLayers(Layer* top, Layer* bottom);
    private:
        Application* application;
        Internetwork* internetwork;
        int src_port, dst_port;
        vector<Segment*> buffer;
};

class Internetwork: public Layer {
    public:
        Internetwork(string src_ip, string dst_ip);
        void encapsulate(PDU* pdu);
        PDU* decapsulate(PDU* pdu);
        void setLayers(Layer* top, Layer* bottom);
    private:
        string src_ip, dst_ip;
        Transport* transport;
        NetworkAccess* network_access;
};

class NetworkAccess: public Layer {
    public:
        NetworkAccess(string src_mac, string dst_mac, SharedMemory* shared_memory);
        void encapsulate(PDU* pdu);
        PDU* decapsulate(PDU* pdu);
        Frame* check();
        void setLayers(Layer* top, Layer* bottom);
    private:
        string src_mac, dst_mac;
        Internetwork* internetwork;
        SharedMemory* shared_memory;
};