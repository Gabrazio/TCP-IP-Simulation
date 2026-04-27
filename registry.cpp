#include "registry.h"

        Registry::Registry() : ip_part_1(0), ip_part_2(0), ip_part_3(0), ip_part_4(0),
             mac_part_1(0), mac_part_2(0), mac_part_3(0), mac_part_4(0),
             mac_part_5(0), mac_part_6(0), port_counter(1000) {}

        std::string Registry::getIpAddress() {
            if(ip_part_4 == 255) { ip_part_4 = 0;
                if(ip_part_3 == 255) { ip_part_3 = 0;
                    if(ip_part_2 == 255) { ip_part_2 = 0;
                        if(ip_part_1 == 255) return "";
                        else ip_part_1++;
                    } else ip_part_2++;
                } else ip_part_3++;
            } else ip_part_4++;
            return std::to_string(ip_part_1) + "." + std::to_string(ip_part_2) + "." +
                   std::to_string(ip_part_3) + "." + std::to_string(ip_part_4);
        }

        std::string Registry::getMacAddress() {
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

        int Registry::getPort() { return port_counter++; }

        std::string Registry::toHex(int value) {
            std::string hex = "0123456789ABCDEF";
            return std::string(1, hex[value / 16]) + std::string(1, hex[value % 16]);
        }