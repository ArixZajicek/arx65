#include "BusConnection.h"

#ifndef SIMPLEMEMORY_H
#define SIMPLEMEMORY_H
class SimpleMemory : public BusConnection
{
private:
	uint16_t start_addr, end_addr;
	uint8_t *data_start;
	bool isDataExternal;
	bool isReadOnlyMemory;

public:
	SimpleMemory(uint16_t addressStart, uint16_t addressEnd, uint8_t fillValue, bool ROM = false);
	SimpleMemory(uint16_t addressStart, uint16_t addressEnd, uint8_t *data, bool ROM = false);
	~SimpleMemory();

	bool isAddressInRange(uint16_t addr, bool read);
	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t byte);
};
#endif
