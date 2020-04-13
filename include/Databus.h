#include "Common.h"
#include "BusConnection.h"

#ifndef DATABUS_H
#define DATABUS_H

namespace arx65::Databus
{
	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t byte);
	void attach(BusConnection *device);
};

#endif
