#include "pdu.h"
#include <string>

        Data::Data(std::string payload) : payload(payload) {}
        std::string Data::getPayload() { return payload; }

        Segment::Segment(Data* data, int src_port, int dst_port, int seq_num, int total)
            : data(data), src_port(src_port), dst_port(dst_port), seq_num(seq_num), total(total) {}

        Data* Segment::getData()  { return data; }
        int Segment::getSrcPort() { return src_port; }
        int Segment::getDstPort() { return dst_port; }
        int Segment::getSeqNum()  { return seq_num; }
        int Segment::getTotal()   { return total; }

        Packet::Packet(Segment* segment, std::string src_ip, std::string dst_ip) : segment(segment), src_ip(src_ip), dst_ip(dst_ip) {}
        Segment* Packet::getSegment() { return segment; }
        std::string Packet::getSrcIp()     { return src_ip; }
        std::string Packet::getDstIp()     { return dst_ip; }

        Frame::Frame(Packet* packet, std::string src_mac, std::string dst_mac) : packet(packet), src_mac(src_mac), dst_mac(dst_mac) {}
        Packet* Frame::getPacket()             { return packet; }
        const std::string& Frame::getSrcMac() const { return src_mac; }
        const std::string& Frame::getDstMac() const { return dst_mac; }