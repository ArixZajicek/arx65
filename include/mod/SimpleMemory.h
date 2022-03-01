#include "BusConnection.h"

#ifndef SIMPLEMEMORY_H
#define SIMPLEMEMORY_H
namespace arx65::mod
{
	class SimpleMemory : public BusConnection
	{
	private:
		uint16_t start_addr, end_addr;
		uint8_t *data_start;
		bool isReadOnlyMemory;

	public:
		SimpleMemory(uint16_t addressStart, uint16_t addressEnd, uint8_t fillValue, bool ROM = false);
		~SimpleMemory();

		bool loadFromFile(const char *filename, uint16_t addressStart);
		bool copyFromMemory(const uint8_t *data, uint16_t addressStart, uint16_t len);

		bool isAddressInRange(uint16_t addr, bool read);
		uint8_t read(uint16_t address);
		void write(uint16_t address, uint8_t byte);
	};
}
#endif
