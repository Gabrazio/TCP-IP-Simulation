#pragma once
#include <string>

class PDU {
    public:
        virtual ~PDU() = default;
};

class Data: public PDU {
    public:
        Data(std::string payload);
        std::string getPayload();
    private:
        std::string payload;
};

class Segment: public PDU {
    public:
        Segment(Data* data, int src_port, int dst_port, int seq_num, int total);

        Data* getData();
        int getSrcPort();
        int getDstPort();
        int getSeqNum();
        int getTotal();

    private:
        Data *data;
        int src_port, dst_port, seq_num, total;
};

class Packet: public PDU {
    public:
        Packet(Segment* segment, std::string src_ip, std::string dst_ip);
        Segment* getSegment();
        std::string getSrcIp();
        std::string getDstIp();
    private:
        Segment* segment;
        std::string src_ip, dst_ip;
};

class Frame: public PDU {
    public:
        Frame(Packet* packet, std::string src_mac, std::string dst_mac);
        Packet* getPacket();
        const std::string& getSrcMac() const;
        const std::string& getDstMac() const;
    private:
        Packet *packet;
        std::string src_mac, dst_mac;
};