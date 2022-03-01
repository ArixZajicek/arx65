#include "Common.h"
#include "mod/SimpleMemory.h"

using namespace std;

namespace arx65::mod
{
	SimpleMemory::SimpleMemory(uint16_t addressStart, uint16_t addressEnd, uint8_t fillValue, bool ROM) 
	{
		start_addr = addressStart;
		end_addr = addressEnd;
		isReadOnlyMemory = ROM;
		data_start = new uint8_t[addressEnd - addressStart + 1];

		for (int i = addressStart; i <= addressEnd; i++)
		{
			data_start[i - addressStart] = fillValue;
		}
	}

	SimpleMemory::~SimpleMemory() 
	{
		delete[] data_start;
	}

	bool SimpleMemory::loadFromFile(const char *filename, uint16_t addressStart)
	{
		// Check if file exists
		ifstream romFile;
		romFile.open(filename, ios::in | ios::binary);

		if (romFile.fail() || !romFile.is_open())
		{
			std::cerr << "Error opening '" << filename << "'.\r\n";
			return false;
		}

		// Get File Size and halt if size is incorrect
		romFile.seekg(0, ios::end);
		int romLength = romFile.tellg();
		romFile.seekg(0, ios::beg);

		if (romLength > end_addr - addressStart + 1)
		{
			std::cerr << "Input file too large for memory. Size: " << romLength << " bytes, Maximum size: " << end_addr-addressStart << " bytes when starting at " << HEX(4, addressStart) << ".\r\n"; 
			return false;
		}

		std::cout << "Memory file '" << filename << "' found, " << romLength << " bytes long.\r\n";
		
		romFile.read((char *)(data_start + addressStart - start_addr), romLength);
		romFile.close();
		
		std::cout << "Finished loading memory file '" << filename << "' at 0x" << HEX(4, addressStart) << ".\r\n";
		return true;
	}

	bool SimpleMemory::copyFromMemory(const uint8_t *data, uint16_t addressStart, uint16_t len)
	{
		if (addressStart < start_addr || addressStart > end_addr || addressStart + len - 1 < start_addr || addressStart + len - 1 > end_addr)
		{
			std::cerr << "Attempted copying out of memory bounds. Tried inserting into " << HEX(4, addressStart) << "-" << HEX(4, (addressStart + len))
				<< ", but memory is only available from " << HEX(4, start_addr) << "-" << HEX(4, end_addr) << ". Aborting.\r\n";
			return false;
		}

		for (int i = addressStart - start_addr; i < addressStart - start_addr + len; i++)
		{
			data_start[i] = data[i - addressStart + start_addr];
		}

		return true;
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
}
