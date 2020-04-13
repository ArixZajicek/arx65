#include "Common.h"
#include "Databus.h"

#pragma once

namespace arx65::Processor
{
	/* Masks for FLAG register */
	const uint8_t FLAG_NEGATIVE = 0x80;
	const uint8_t FLAG_OVERFLOW = 0x40;
	const uint8_t FLAG_BRK = 0x10;
	const uint8_t FLAG_DECIMAL = 0x08;
	const uint8_t FLAG_INTERRUPT = 0x04;
	const uint8_t FLAG_ZERO = 0x02;
	const uint8_t FLAG_CARRY = 0x01;

	/* The main register set */
	typedef struct {
		uint8_t A;
		uint8_t X;
		uint8_t Y;
		uint8_t Flags;
		uint8_t SP;
		uint16_t PC;
	} RegisterSet;

	RegisterSet *getRegisters();
	RegisterSet getRegistersCopy();

	// Main initialization
	void init();

	// Call the CPU to do the next intruction, and return the number of cycles that instruction takes.
	int doNextInstruction();

	// Non-Maskable Interrupt call, finds address from FFFA and FFFB (low/high) and executes regardless
	void doNMI();

	// Reset processor, of course, find address from FFFC, FFFD and go there.
	void doRES();

	// Interrupt Request, FFFE and FFFF, interrupt flag must not be set already, but will be set until completed
	void doIRQ();
	
};
