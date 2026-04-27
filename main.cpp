#include <iostream>
#include <string>
#include <vector>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <cstdlib>
#include <unistd.h>

#define MSS 5

using namespace std;

class Transport;
class Internetwork;
class NetworkAccess;
class SharedMemory;

pthread_mutex_t mtx_cout = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_sync = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond_enc = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_dec = PTHREAD_COND_INITIALIZER;


bool can_enc = true;
bool can_dec = false;

struct Address {
    int src_port;
    string src_ip;
    string src_mac;
};

struct ThreadData {
    SharedMemory* shared_memory;
    Address* address;
};

/*
    x------x
    | PDUs |
    x------x
*/

class PDU {
    public:
        virtual ~PDU() = default;
};

class Data: public PDU {
    public:
        Data(string payload) : payload(payload) {}
        string getPayload() { return payload; }
    private:
        string payload;
};

class Segment: public PDU {
    public:
        Segment(Data* data, int src_port, int dst_port, int seq_num, int total)
            : data(data), src_port(src_port), dst_port(dst_port), seq_num(seq_num), total(total) {}

        Data* getData()  { return data; }
        int getSrcPort() { return src_port; }
        int getDstPort() { return dst_port; }
        int getSeqNum()  { return seq_num; }
        int getTotal()   { return total; }

    private:
        Data *data;
        int src_port, dst_port, seq_num, total;
};

class Packet: public PDU {
    public:
        Packet(Segment* segment, string src_ip, string dst_ip) : segment(segment), src_ip(src_ip), dst_ip(dst_ip) {}
        Segment* getSegment() { return segment; }
        string getSrcIp()     { return src_ip; }
        string getDstIp()     { return dst_ip; }
    private:
        Segment* segment;
        string src_ip, dst_ip;
};

class Frame: public PDU {
    public:
        Frame(Packet* packet, string src_mac, string dst_mac) : packet(packet), src_mac(src_mac), dst_mac(dst_mac) {}
        Packet* getPacket()             { return packet; }
        const string& getSrcMac() const { return src_mac; }
        const string& getDstMac() const { return dst_mac; }
    private:
        Packet *packet;
        string src_mac, dst_mac;
};

/*
    x-------x
    | LAYER |
    x-------x
*/

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

/*
    x--------------------x
    | SHARED MEMORY FIFO |
    x--------------------x
*/

struct Node {
    Frame *frame;
    Node *next = NULL;
};

class SharedMemory {
    public:
        SharedMemory() : head(NULL), tail(NULL) {
            pthread_mutex_init(&mtx, NULL);
        }

        void push(Frame* frame) {
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

        Frame* pop() {
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

        Frame* peek() {
            pthread_mutex_lock(&mtx);
                Frame* frame = (head != NULL) ? head->frame : NULL;
            pthread_mutex_unlock(&mtx);
            return frame;
        }

    private:
        pthread_mutex_t mtx;
        Node* head;
        Node* tail;
};

/*
    x--------x
    | DEVICE |
    x--------x
*/

class DEVice {
    public:
        DEVice(int src_port, int dst_port,
               string src_ip,  string dst_ip,
               string src_mac, string dst_mac,
               SharedMemory* shared_memory) {
            application    = new Application();
            transport      = new Transport(src_port, dst_port);
            internetwork   = new Internetwork(src_ip, dst_ip);
            network_access = new NetworkAccess(src_mac, dst_mac, shared_memory);

            application->setLayers(NULL, transport);
            transport->setLayers(application, internetwork);
            internetwork->setLayers(transport, network_access);
            network_access->setLayers(internetwork, NULL);
        }

        void send(string message) {
            pthread_mutex_lock(&mtx_sync);

            // Non può avvenire ENCAPSULATION
            if(!can_enc) {
                pthread_mutex_unlock(&mtx_sync);
                return; // Esco
            }

            can_enc = false;

            pthread_mutex_unlock(&mtx_sync);

            application->encapsulate(new Data(message));

            // ENCAPSULATION completata: sblocco THREADS per DECAPSULATION

            pthread_mutex_lock(&mtx_sync);

            can_dec = true; // La DECAPSULATION può avvenire

            pthread_cond_signal(&cond_dec); // DECAPSULATION possibile (dei dati da leggere sono presenti in coda)

            pthread_mutex_unlock(&mtx_sync);
        }

        void receive() {
            pthread_mutex_lock(&mtx_sync);

            // Non può avvenire DECAPSULATION (una DECAPSULATION non è ancora avvenuta)
            if(!can_dec) {
                pthread_mutex_unlock(&mtx_sync);
                return;
            }

            can_dec = false; // La DECAPSULATION non può avvenire (per gli altri THREADS)

            pthread_mutex_unlock(&mtx_sync);

            Frame* frame = network_access->check();

            if(frame == NULL) {
                pthread_mutex_lock(&mtx_sync);

                can_dec = true;

                pthread_cond_signal(&cond_dec);

                pthread_mutex_unlock(&mtx_sync);

                return;
            }

            PDU* pdu = network_access->decapsulate(frame);
            pdu = internetwork->decapsulate(pdu);
            pdu = transport->decapsulate(pdu);

            if(pdu != NULL) {
                Data* data = dynamic_cast<Data*>(application->decapsulate(pdu));
                if(data != NULL) {
                    pthread_mutex_lock(&mtx_cout);
                    
                    cout << endl << "\033[33m[DEV" << pthread_self() << " | Application] " << data->getPayload() << "\033[0m" << endl << endl;
                    
                    pthread_mutex_unlock(&mtx_cout);
                
                    sleep(1);
                }

                pthread_mutex_lock(&mtx_sync);

                can_enc = true;
                
                pthread_cond_signal(&cond_enc);

                pthread_mutex_unlock(&mtx_sync);
            } else {
                pthread_mutex_lock(&mtx_sync);

                can_dec = true;
                pthread_cond_signal(&cond_dec);

                pthread_mutex_unlock(&mtx_sync);
            }
        }

    private:
        Application   *application;
        Transport     *transport;
        Internetwork  *internetwork;
        NetworkAccess *network_access;
};

/*
    x-----------------x
    | IMPLEMENTAZIONI |
    x-----------------x
*/

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
    shared_memory->push(frame);
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
    if(frame->getDstMac() == src_mac) return shared_memory->pop();
    return NULL;
}

void NetworkAccess::setLayers(Layer* top, Layer* bottom) {
    internetwork = dynamic_cast<Internetwork*>(top);
}

/*
    x----------x
    | REGISTRY |
    x----------x
*/

class Registry {
    public:
        Registry() : ip_part_1(0), ip_part_2(0), ip_part_3(0), ip_part_4(0),
             mac_part_1(0), mac_part_2(0), mac_part_3(0), mac_part_4(0),
             mac_part_5(0), mac_part_6(0), port_counter(1000) {}

        static Registry& getInstance() {
            static Registry instance;
            return instance;
        }

        string getIpAddress() {
            if(ip_part_4 == 255) { ip_part_4 = 0;
                if(ip_part_3 == 255) { ip_part_3 = 0;
                    if(ip_part_2 == 255) { ip_part_2 = 0;
                        if(ip_part_1 == 255) return "";
                        else ip_part_1++;
                    } else ip_part_2++;
                } else ip_part_3++;
            } else ip_part_4++;
            return to_string(ip_part_1) + "." + to_string(ip_part_2) + "." +
                   to_string(ip_part_3) + "." + to_string(ip_part_4);
        }

        string getMacAddress() {
            if(mac_part_6 == 255) { mac_part_6 = 0;
                if(mac_part_5 == 255) { mac_part_5 = 0;
                    if(mac_part_4 == 255) { mac_part_4 = 0;
                        if(mac_part_3 == 255) { mac_part_3 = 0;
                            if(mac_part_2 == 255) { mac_part_2 = 0;
                                if(mac_part_1 == 255) return "";
                                else mac_part_1++;
                            } else mac_part_2++;
                        } else mac_part_3++;
                    } else mac_part_4++;
                } else mac_part_5++;
            } else mac_part_6++;
            return toHex(mac_part_1) + ":" + toHex(mac_part_2) + ":" +
                   toHex(mac_part_3) + ":" + toHex(mac_part_4) + ":" +
                   toHex(mac_part_5) + ":" + toHex(mac_part_6);
        }

        int getPort() { return port_counter++; }

        string toHex(int value) {
            string hex = "0123456789ABCDEF";
            return string(1, hex[value / 16]) + string(1, hex[value % 16]);
        }

    private:
        int ip_part_1, ip_part_2, ip_part_3, ip_part_4;
        int mac_part_1, mac_part_2, mac_part_3, mac_part_4, mac_part_5, mac_part_6;
        int port_counter;
};

/*
    x----------------x
    | MAIN & THREADS |
    x----------------x
*/

vector<Address*> global_addresses;

void* func(void* args) {
    srand(time(0) ^ pthread_self());
    ThreadData* td = (ThreadData*) args;

    Address* dst;
    do {
        dst = global_addresses[rand() % global_addresses.size()];
    } while(dst->src_mac == td->address->src_mac);

    DEVice DEVice(
        td->address->src_port, dst->src_port,
        td->address->src_ip,   dst->src_ip,
        td->address->src_mac,  dst->src_mac,
        td->shared_memory
    );

    while(true) {

        // POSSO INCAPSULARE ?
            if(rand() % 4 == 0) {
                do {
                    dst = global_addresses[rand() % global_addresses.size()];
                } while(dst->src_mac == td->address->src_mac);
                DEVice.send("Hello from DEV" + to_string(pthread_self()));
            }

        // POSSO DECAPSULARE ?
            DEVice.receive();

    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc != 2) {
        cout << "Uso: " << argv[0] << " <NUM_DEVICES>" << endl;
        return EXIT_FAILURE;
    }

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