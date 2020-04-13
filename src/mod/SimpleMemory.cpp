#include "Common.h"
#include "mod/SimpleMemory.h"

SimpleMemory::SimpleMemory(uint16_t addressStart, uint16_t addressEnd, uint8_t fillValue, bool ROM) 
{
	start_addr = addressStart;
	end_addr = addressEnd;
	isDataExternal = false;
	isReadOnlyMemory = ROM;
	data_start = new uint8_t[addressEnd - addressStart + 1];

	for (int i = addressStart; i <= addressEnd; i++)
	{
		data_start[i - addressStart] = fillValue;
	}
}

SimpleMemory::SimpleMemory(uint16_t addressStart, uint16_t addressEnd, uint8_t *data, bool ROM) \
{
	start_addr = addressStart;
	end_addr = addressEnd;
	isDataExternal = true;
	isReadOnlyMemory = ROM;
	data_start = data;
}

SimpleMemory::~SimpleMemory() 
{
	if (isDataExternal == false) delete[] data_start;
}

bool SimpleMemory::isAddressInRange(uint16_t addr, bool read) 
{
	// Return true if reading or memory is writable, and address is in range.
	return (read || !isReadOnlyMemory) && (addr >= start_addr && addr <= end_addr);
}

uint8_t SimpleMemory::read(uint16_t address) 
{
	//if (!isReadOnlyMemory) std::cout << "Reading from (" << std::hex << (int)address << "), value of (" << std::hex << (int)data_start[address - start_addr] << ").";
	return data_start[address - start_addr];
}

void SimpleMemory::write(uint16_t address, uint8_t byte) 
{
	// Only do this for RAM, ROM isn't writable!
	if (!isReadOnlyMemory) {
		//std::cout << "Writing to (" << std::hex << (int)address << "). Overwriting (" << std::hex << (int)data_start[address - start_addr] << ") with (" << std::hex << (int)byte << ").";
		data_start[address - start_addr] = byte;
	}
}

