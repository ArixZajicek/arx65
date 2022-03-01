#include "Databus.h"


namespace arx65::bus
{
	std::vector<arx65::mod::BusConnection *> connections;

	uint8_t read(uint16_t address) 
	{
		for (arx65::mod::BusConnection *b : connections)
		{
			if (b->isAddressInRange(address, true))
			{
				return b->read(address);
			}
		}

		return 0;
	}

	void write(uint16_t address, uint8_t byte)
	{
		for (arx65::mod::BusConnection *b : connections)
		{
			if (b->isAddressInRange(address, false))
			{
				b->write(address, byte);
				break;
			}
		}
	}

	void attach(arx65::mod::BusConnection *device)
	{
		connections.push_back(device);
	}

	void clear()
	{
		connections.clear();
	}
}