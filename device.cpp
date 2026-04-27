#include "device.h"
#include <iostream>
#include <unistd.h>

pthread_mutex_t mtx_cout = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_sync = PTHREAD_MUTEX_INITIALIZER;

bool can_enc = true;
bool can_dec = false;

        Device::Device(int src_port, int dst_port,
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

        void Device::send(string message) {
            pthread_mutex_lock(&mtx_sync);

            if(!can_enc) {
                pthread_mutex_unlock(&mtx_sync);
                return;
            }

            can_enc = false;

            pthread_mutex_unlock(&mtx_sync);

            application->encapsulate(new Data(message));

            pthread_mutex_lock(&mtx_sync);

            can_dec = true;

            pthread_mutex_unlock(&mtx_sync);
        }

        void Device::receive() {
            pthread_mutex_lock(&mtx_sync);

            if(!can_dec) {
                pthread_mutex_unlock(&mtx_sync);
                return;
            }

            can_dec = false;

            pthread_mutex_unlock(&mtx_sync);

            Frame* frame = network_access->check();

            if(frame == NULL) {
                pthread_mutex_lock(&mtx_sync);

                can_dec = true;

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
                    
                    cout << endl << "\033[33m[DEV" << pthread_self() << "] " << data->getPayload() << "\033[0m" << endl << endl;
                    
                    pthread_mutex_unlock(&mtx_cout);
                
                    sleep(1);
                }

                pthread_mutex_lock(&mtx_sync);

                can_enc = true;
                
                pthread_mutex_unlock(&mtx_sync);
            } else {
                pthread_mutex_lock(&mtx_sync);

                can_dec = true;

                pthread_mutex_unlock(&mtx_sync);
            }
        }