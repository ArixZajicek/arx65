#include <cstdint>
#include <vector>
#include "base/BusConnection.h"

#pragma once
class Databus
{
private:
	std::vector<BusConnection *> connections;

public:
	Databus();
	~Databus();
	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t byte);
	void attach(BusConnection *device);
};

