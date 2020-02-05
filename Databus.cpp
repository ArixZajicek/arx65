#include "Databus.h"


Databus::Databus() {}
Databus::~Databus()
{
	for (BusConnection *b : connections)
	{
		//delete b;
	}
}

uint8_t Databus::read(uint16_t address) 
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

void Databus::write(uint16_t address, uint8_t byte)
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

void Databus::attach(BusConnection *device)
{
	connections.push_back(device);
}
