#include "Databus.h"


namespace arx65::Databus
{
	std::vector<BusConnection *> connections;

	uint8_t read(uint16_t address) 
	{
		for (BusConnection *b : connections)
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
		for (BusConnection *b : connections)
		{
			if (b->isAddressInRange(address, false))
			{
				b->write(address, byte);
				break;
			}
		}
	}

	void attach(BusConnection *device)
	{
		connections.push_back(device);
	}
}