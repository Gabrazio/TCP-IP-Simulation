#include "layer.h"
#include <iostream>
#include <pthread.h>

#define MSS 5

extern pthread_mutex_t mtx_cout;

Application::Application() {}

void Application::encapsulate(PDU* pdu) {
    Data* data = dynamic_cast<Data*>(pdu);
    pthread_mutex_lock(&mtx_cout);
    cout << "\033[32m[DEV" << pthread_self() << " | Application] Data { payload: \""
         << data->getPayload() << "\" }"  << endl;
    pthread_mutex_unlock(&mtx_cout);
    transport->encapsulate(pdu);
}

PDU* Application::decapsulate(PDU* pdu) {
    Data* data = dynamic_cast<Data*>(pdu);
    pthread_mutex_lock(&mtx_cout);
    cout << "[DEV" << pthread_self() << " | Application] Data { payload: \""
         << data->getPayload() << "\" }}\033[0m"  << endl;
    pthread_mutex_unlock(&mtx_cout);
    return data;
}

void Application::setLayers(Layer* top, Layer* bottom) {
    transport = dynamic_cast<Transport*>(bottom);
}

void Transport::encapsulate(PDU* pdu) {
    Data* data = dynamic_cast<Data*>(pdu);
    string payload = data->getPayload();
    int total = (payload.size() + MSS - 1) / MSS;

    pthread_mutex_lock(&mtx_cout);
    cout << "[DEV" << pthread_self() << " | Transport] Segmenting into "
         << total << " segment(s) (MSS=" << MSS << ")"  << endl;
    pthread_mutex_unlock(&mtx_cout);

    for(int i = 0; i < total; i++) {
        string chunk = payload.substr(i * MSS, MSS);
        Segment* segment = new Segment(new Data(chunk), src_port, dst_port, i, total);

        pthread_mutex_lock(&mtx_cout);
        cout << "\033[32m[DEV" << pthread_self() << " | Transport] Segment { seq="
             << i << "/" << total-1 << ", payload=\"" << chunk << "\" }"  << endl;
        pthread_mutex_unlock(&mtx_cout);

        internetwork->encapsulate(segment);
    }
}

PDU* Transport::decapsulate(PDU* pdu) {
    Segment* segment = dynamic_cast<Segment*>(pdu);
    buffer.push_back(segment);

    pthread_mutex_lock(&mtx_cout);
    cout << "[DEV" << pthread_self() << " | Transport] Segment { seq="
         << segment->getSeqNum() << "/" << segment->getTotal()-1
         << ", payload=\"" << segment->getData()->getPayload() << "\" }"
         << " [" << buffer.size() << "/" << segment->getTotal() << " received]"
          << endl;
    pthread_mutex_unlock(&mtx_cout);

    if((int)buffer.size() < segment->getTotal())
        return NULL;

    string full_payload = "";
    for(Segment* s : buffer)
        full_payload += s->getData()->getPayload();

    buffer.clear();
    return new Data(full_payload);
}

void Transport::setLayers(Layer* top, Layer* bottom) {
    application  = dynamic_cast<Application*>(top);
    internetwork = dynamic_cast<Internetwork*>(bottom);
}

Internetwork::Internetwork(string src_ip, string dst_ip) : src_ip(src_ip), dst_ip(dst_ip) {}

void Internetwork::encapsulate(PDU* pdu) {
    Packet* packet = new Packet(dynamic_cast<Segment*>(pdu), src_ip, dst_ip);
    pthread_mutex_lock(&mtx_cout);
    cout << "[DEV" << pthread_self() << " | Internetwork] Packet { src_ip: "
         << src_ip << ", dst_ip: " << dst_ip << " }"  << endl;
    pthread_mutex_unlock(&mtx_cout);
    network_access->encapsulate(packet);
}

PDU* Internetwork::decapsulate(PDU* pdu) {
    Packet* packet = dynamic_cast<Packet*>(pdu);
    pthread_mutex_lock(&mtx_cout);
    cout << "[DEV" << pthread_self() << " | Internetwork] Packet { src_ip: "
         << packet->getSrcIp() << ", dst_ip: " << packet->getDstIp() << " }"  << endl;
    pthread_mutex_unlock(&mtx_cout);
    return packet->getSegment();
}

void Internetwork::setLayers(Layer* top, Layer* bottom) {
    transport      = dynamic_cast<Transport*>(top);
    network_access = dynamic_cast<NetworkAccess*>(bottom);
}

NetworkAccess::NetworkAccess(string src_mac, string dst_mac, SharedMemory* shared_memory)
    : src_mac(src_mac), dst_mac(dst_mac), shared_memory(shared_memory) {}

void NetworkAccess::encapsulate(PDU* pdu) {
    Frame* frame = new Frame(dynamic_cast<Packet*>(pdu), src_mac, dst_mac);
    pthread_mutex_lock(&mtx_cout);
    cout << "[DEV" << pthread_self() << " | Network Access] Frame { src_mac: "
         << src_mac << ", dst_mac: " << dst_mac << " }\033[0m"  << endl;
    pthread_mutex_unlock(&mtx_cout);
    shared_memory->enqueue(frame);
}

PDU* NetworkAccess::decapsulate(PDU* pdu) {
    Frame* frame = dynamic_cast<Frame*>(pdu);
    pthread_mutex_lock(&mtx_cout);
    cout << "\033[35m[DEV" << pthread_self() << " | Network Access] Frame { src_mac: "
         << frame->getSrcMac() << ", dst_mac: " << frame->getDstMac() << " }"  << endl;
    pthread_mutex_unlock(&mtx_cout);
    return frame->getPacket();
}

Frame* NetworkAccess::check() {
    Frame* frame = shared_memory->peek();
    if(frame == NULL) return NULL;
    if(frame->getDstMac() == src_mac) return shared_memory->dequeue();
    return NULL;
}

void NetworkAccess::setLayers(Layer* top, Layer* bottom) {
    internetwork = dynamic_cast<Internetwork*>(top);
}