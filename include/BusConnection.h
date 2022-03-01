#include "Common.h"

#pragma once

namespace arx65::mod
{
	class BusConnection {
	public:
		// Return true if an address is in range of connected device
		virtual bool isAddressInRange(uint16_t address, bool read) = 0;

		// Read/write to the device, only called if address in range is true.
		virtual uint8_t read(uint16_t address) = 0;
		virtual void write(uint16_t address, uint8_t byte) = 0;
	};
}
