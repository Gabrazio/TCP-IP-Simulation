#pragma once
#include <string>

class Registry {
    public:
        Registry();

        static Registry& getInstance() {
            static Registry instance;
            return instance;
        }

        std::string getIpAddress();

        std::string getMacAddress();

        int getPort();

        std::string toHex(int value);

    private:
        int ip_part_1, ip_part_2, ip_part_3, ip_part_4;
        int mac_part_1, mac_part_2, mac_part_3, mac_part_4, mac_part_5, mac_part_6;
        int port_counter;
};