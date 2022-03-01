#include "Common.h"
#include "BusConnection.h"

#pragma once

using namespace std;

namespace arx65::mod
{
    class ACIA6551 : public BusConnection
    {
    private:
        uint16_t base_address;

        uint8_t command_register;
        uint8_t control_register;
        uint8_t status_register;

        vector<uint8_t> transmit, receive;
    
    public:
        ACIA6551(uint16_t address);

        // Required functions
        bool isAddressInRange(uint16_t addr, bool read);
		uint8_t read(uint16_t address);
		void write(uint16_t address, uint8_t byte);

        int bytesAvailable();
        uint8_t nextByte();
        void clearBytes();

        void sendByte(const uint8_t byte);
        void sendBytes(const uint8_t *byte, int len = -1);
    };
}