#include "mod/ACIA6551.h"
#include "Processor.h"

namespace arx65::mod
{
    ACIA6551::ACIA6551(uint16_t address)
    {
        base_address = address;
        command_register = 0x02;
        control_register = 0x00;
    }

    bool ACIA6551::isAddressInRange(uint16_t addr, bool read)
    {
        return addr >= base_address && addr <= base_address + 3;
    }

    uint8_t ACIA6551::read(uint16_t address)
    {
        if (address == base_address)
        {   // Read received data
            if (receive.size())
            {
                uint8_t x = receive.at(0);
                receive.erase(receive.begin());
                return x;
            }
            return 0x00;
        }
        else if (address == base_address + 1)
        {   // Read status register (all can stay as 0 except read?)
            // Transmit empty should also be 1 so processor can send
            return (receive.size() > 0 ? 0x18 : 0x10);
        }
        else if (address == base_address + 2)
        {   // Read command register
            return command_register;
        }
        else if (address == base_address + 3)
        {   // Read control register
            return control_register;
        }

        return 0;
    }
    void ACIA6551::write(uint16_t address, uint8_t byte)
    {
        if (address == base_address)
        {   // Send transmitted data
            // Only do something if DTR is active (transmit/receive enable)
            if (control_register & 0x01) transmit.push_back(byte);
        }
        else if (address == base_address + 1)
        {   // Programmed reset (data doesn't care)
            transmit.clear();
            receive.clear();
            command_register |= 0x02;
            command_register &= 0xE2;
        }
        else if (address == base_address + 2)
        {   // Write to command register
            command_register = byte;
        }
        else if (address == base_address + 3)
        {   // Write to control register - DONE
            control_register = byte;
        }
    }

    int ACIA6551::bytesAvailable()
    {
        return transmit.size();
    }

    uint8_t ACIA6551::nextByte()
    {
        uint8_t x = transmit.at(0);
        transmit.erase(transmit.begin());
        return x;
    }

    void ACIA6551::clearBytes()
    {
        transmit.clear();
    }

    void ACIA6551::sendByte(const uint8_t byte)
    {
        // Only do something if DTR is active (transmit/receive enable)
        if (control_register & 0x01)
        {
            // TODO: Call IRQ of CPU if IRQ enabled
            if (0x02 & control_register) {
                status_register |= 0x80;
                arx65::cpu::doIRQ();
            }

            // Push the byte to be received
            receive.push_back(byte);

            // If echo enabled, then echo
            if (0x10 & control_register) transmit.push_back(byte);
        }
    }

    void ACIA6551::sendBytes(const uint8_t *byte, int len)
    {
        for (int i = 0; byte[i] != '\0' && (len < 0 || i < len); i++)
        {
            sendByte(byte[i]);
        }
    }
}