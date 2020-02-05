#include <iomanip>
#include <bitset>
#include <cstdint>
#include <iostream>
#include "Databus.h"

#pragma once

#define HEX(x, prin) setfill('0') << setw(x) << right << uppercase << hex << (int)prin << nouppercase << dec

using namespace std;

// Standard Registers
struct RegisterSet {
	uint8_t A;
	uint8_t X;
	uint8_t Y;
	uint8_t Flags;
	uint8_t SP;
	uint16_t PC;
};

class Cpu6502
{
public:

	static const uint8_t FLAG_NEGATIVE = 0x80;
	static const uint8_t FLAG_OVERFLOW = 0x40;
	static const uint8_t FLAG_BRK = 0x10;
	static const uint8_t FLAG_DECIMAL = 0x08;
	static const uint8_t FLAG_INTERRUPT = 0x04;
	static const uint8_t FLAG_ZERO = 0x02;
	static const uint8_t FLAG_CARRY = 0x01;

	RegisterSet R;
	Databus *Bus;

	// Main initialization
	Cpu6502(Databus *bus);

	// Call the CPU to do the next intruction, and return the number of cycles that instruction takes.
	int doNextInstruction();

	// Non Mask Interrupt call, finds address from FFFA and FFFB (low/high) and executes regardless
	void doNMI();

	// Reset processor, of course, find address from FFFC, FFFD and go there.
	void doRES();

	// Interrupt Request, FFFE and FFFF, interrupt flag must not be set already, but will be set until completed
	void doIRQ();

private:
	// Array of function pointers, one for each possible instruction
	int (Cpu6502::*instruction[256])();

	uint8_t InvalidInstruction()
	{
		cout << "Invalid instruction 0x" << HEX(2, Bus->read(R.PC)) << " at 0x" << HEX(4, R.PC) << "." << endl;
		++R.PC;
		return 0;
	}

	/* Reolve a direct Zero Page address. (Advances PC + 1) */
	uint16_t ResolveZP()
	{
		return Bus->read(++R.PC);
	}

	/* Resolve a direct zero page X, with X offset, including wraparound. (Advances PC + 1) */
	uint16_t ResolveZPX()
	{
		return (Bus->read(++R.PC) + R.X) & 0x00FF;
	}

	/* Resolve a direct zero page Y, with Y offset, including wraparound. (Advances PC + 1) */
	uint16_t ResolveZPY()
	{
		return (Bus->read(++R.PC) + R.Y) & 0x00FF; // This & might not be necessary since both values are already of uint8_t type
	}

	/* Resolve a direct address (advances PC + 2)*/
	uint16_t ResolveAbsolute()
	{
		return (uint16_t)Bus->read(++R.PC) | (((uint16_t)Bus->read(++R.PC)) << 8);
	}

	/* Resolve direct address with X offset, and pageCrossed will be appropriately set (advances PC + 2)*/
	uint16_t ResolveAbsoluteX(bool &pageCrossed)
	{
		uint16_t addressBeforeAdding = (uint16_t)Bus->read(++R.PC) | (((uint16_t)Bus->read(++R.PC)) << 8);

		// To determine if a page was crossed, we just see if the most significant byte is bigger
		pageCrossed = (0xFF00 & (addressBeforeAdding + R.X) > (0xFF00 & addressBeforeAdding));
		return addressBeforeAdding + R.X;
	}

	/* Resolve direct address with Y offset, and pageCrossed will be appropriately set (advances PC + 2)*/
	uint16_t ResolveAbsoluteY(bool &pageCrossed)
	{
		uint16_t addressBeforeAdding = (uint16_t)Bus->read(++R.PC) | (((uint16_t)Bus->read(++R.PC)) << 8);

		// To determine if a page was crossed, we just see if the most significant byte is bigger
		pageCrossed = (0xFF00 & (addressBeforeAdding + R.Y) > (0xFF00 & addressBeforeAdding));
		return addressBeforeAdding + R.Y;
	}

	/* Resolves an indirect address at zero page + X, then resolve. (advances PC + 1) */
	uint16_t ResolveIndirectX()
	{
		uint8_t zpAddress = Bus->read(++R.PC + R.X);
		return ((uint16_t)Bus->read(zpAddress)) | ((uint16_t)Bus->read(zpAddress + 1) << 8);
	}

	/* Resolves an indirect address at zero page, then add Y, then resolve. (advances PC + 1) */
	uint16_t ResolveIndirectY(bool &pageCrossed)
	{
		uint8_t zpAddress = Bus->read(++R.PC);
		uint16_t addressBeforeAdding = ((uint16_t)Bus->read(zpAddress)) | ((uint16_t)Bus->read(zpAddress + 1) << 8);

		// To determine if a page was crossed, we just see if the most significant byte is bigger
		pageCrossed = (0xFF00 & (addressBeforeAdding + R.Y) > (0xFF00 & addressBeforeAdding));
		return addressBeforeAdding + R.Y;
	}

	// All ADC instructions call on this one, once the number is retrieved.
	void ADC_General(uint8_t num)
	{	
		uint8_t oldA = R.A;
		if (R.Flags & FLAG_DECIMAL) 
		{
			uint8_t onesNibble = (R.A & 0x0F) + (num & 0x0F) + (R.Flags & FLAG_CARRY ? 1 : 0);
			uint8_t tensNibble = ((R.A & 0xF0) >> 4) + ((num & 0xF0) >> 4);
			
			if (onesNibble > 0x09)
			{
				onesNibble -= 10;
				++tensNibble;
			}

			bool carried = false;
			if (tensNibble > 0x09)
			{
				tensNibble -= 10;
				carried = true;
			}

			R.A = ((tensNibble << 4) & 0xF0) | (onesNibble & 0x0F);

			R.Flags &= ~(FLAG_ZERO | FLAG_NEGATIVE | FLAG_CARRY | FLAG_OVERFLOW);
			R.Flags |= (R.A == 0 ? FLAG_ZERO : 0)
				| ((0x80 & R.A) ? FLAG_NEGATIVE : 0)
				| (carried ? FLAG_CARRY : 0)
				| (((0x80 & oldA) ^ (0x80 & R.A)) ? FLAG_OVERFLOW : 0);
		}
		else
		{
			R.A += num + (R.Flags & FLAG_CARRY ? 1 : 0);
			
			R.Flags &= ~(FLAG_ZERO | FLAG_NEGATIVE | FLAG_CARRY | FLAG_OVERFLOW);
			R.Flags |= (R.A == 0 ? FLAG_ZERO : 0)
				| ((0x80 & R.A) ? FLAG_NEGATIVE : 0)
				| (((uint16_t)num + (uint16_t)oldA + (R.Flags & FLAG_CARRY ? 1 : 0) > UINT8_MAX) ? FLAG_CARRY : 0)
				| (((0x80 & oldA) ^ (0x80 & R.A)) ? FLAG_OVERFLOW : 0);
		}
	}

	void AND_General(uint8_t num)
	{
		R.A &= num;
		R.Flags &= ~(FLAG_ZERO | FLAG_NEGATIVE);
		R.Flags |= (R.A == 0 ? FLAG_ZERO : 0) | ((0x80 & R.A) ? FLAG_NEGATIVE : 0);
	}

	void ASL_General(uint8_t &num)
	{
		R.Flags &= ~(FLAG_ZERO | FLAG_NEGATIVE | FLAG_CARRY);
		R.Flags |= (0x80 & num ? FLAG_CARRY : 0);
		num <<= 1;
		R.Flags |= (R.A == 0 ? FLAG_ZERO : 0) | ((0x80 & R.A) ? FLAG_NEGATIVE : 0);
	}

	// Call this with all branch tests. Will set PC either way, and return number of cycles for entire branch.
	int BranchGeneral(bool doBranch)
	{
		if (doBranch) {
			uint16_t oldPC = R.PC + 2;
			R.PC += (int8_t)Bus->read(++R.PC);
			++R.PC;
			if (0xFF00 & R.PC != 0xFF00 & oldPC) return 4; // New page
			return 3; // Success but not a new page
		}
		R.PC += 2;
		return 2; // No branch.
	}

	void BIT_General(uint8_t num)
	{
		R.Flags &= ~(FLAG_ZERO | FLAG_OVERFLOW | FLAG_NEGATIVE);
		R.Flags |= (num & R.A == 0 ? FLAG_ZERO : 0) | (num & 0x40 ? FLAG_OVERFLOW : 0) | (num & 0x80 ? FLAG_NEGATIVE : 0);
	}
	
	void CMP_General(uint8_t reg, uint8_t mem)
	{
		R.Flags &= ~(FLAG_CARRY | FLAG_ZERO | FLAG_NEGATIVE);
		R.Flags |= (reg >= mem ? FLAG_CARRY : 0) | (reg == mem ? FLAG_ZERO : 0) | ((0x80 & (reg - mem)) ? FLAG_NEGATIVE : 0);
	}

	void DEC_General(uint8_t &num)
	{
		num--;
		R.Flags &= ~(FLAG_ZERO | FLAG_NEGATIVE);
		R.Flags |= (num == 0 ? FLAG_ZERO : 0) | ((0x80 & num) ? FLAG_NEGATIVE : 0);
	}

	void EOR_General(uint8_t num)
	{
		R.A ^= num;
		R.Flags &= ~(FLAG_ZERO | FLAG_NEGATIVE);
		R.Flags |= (R.A == 0 ? FLAG_ZERO : 0) | ((0x80 & R.A) ? FLAG_NEGATIVE : 0);
	}

	void INC_General(uint8_t &num)
	{
		num++;
		R.Flags &= ~(FLAG_ZERO | FLAG_NEGATIVE);
		R.Flags |= (num == 0 ? FLAG_ZERO : 0) | ((0x80 & num) ? FLAG_NEGATIVE : 0);
	}

	void LD_General(uint8_t &reg, uint8_t num)
	{
		reg = num;
		R.Flags &= ~(FLAG_ZERO | FLAG_NEGATIVE);
		R.Flags |= (reg == 0 ? FLAG_ZERO : 0) | ((0x80 & reg) ? FLAG_NEGATIVE : 0);
	}

	void LSR_General(uint8_t &num)
	{
		R.Flags &= ~(FLAG_CARRY | FLAG_ZERO | FLAG_NEGATIVE);
		R.Flags |= (0x01 & num ? FLAG_CARRY : 0);
		num >>= 1;
		R.Flags |= (num == 0 ? FLAG_ZERO : 0) | ((0x80 & num) ? FLAG_NEGATIVE : 0);
	}

	void ORA_General(uint8_t num)
	{
		R.A |= num;
		R.Flags &= ~(FLAG_ZERO | FLAG_NEGATIVE);
		R.Flags |= (R.A == 0 ? FLAG_ZERO : 0) | ((0x80 & R.A) ? FLAG_NEGATIVE : 0);
	}

	void ROL_General(uint8_t &num)
	{
		bool oldCarry = R.Flags & FLAG_CARRY;

		R.Flags &= ~(FLAG_CARRY | FLAG_ZERO | FLAG_NEGATIVE);
		R.Flags |= (0x80 & num ? FLAG_CARRY : 0);

		// Do operation
		num <<= 1;

		// Set new bit 0
		if (oldCarry) num |= 0x01;

		R.Flags |= (num == 0 ? FLAG_ZERO : 0) | ((0x80 & num) ? FLAG_NEGATIVE : 0);
	}

	void ROR_General(uint8_t &num)
	{
		bool oldCarry = R.Flags & FLAG_CARRY;

		R.Flags &= ~(FLAG_CARRY | FLAG_ZERO | FLAG_NEGATIVE);
		R.Flags |= (0x01 & num ? FLAG_CARRY : 0);

		// Do operation
		num >>= 1;

		// Set new bit 7
		if (oldCarry) num |= 0x80;

		R.Flags |= (num == 0 ? FLAG_ZERO : 0) | ((0x80 & num) ? FLAG_NEGATIVE : 0);
	}

	void SBC_General(int8_t num)
	{
		uint8_t oldA = R.A;
		if (R.Flags & FLAG_DECIMAL)
		{
			int8_t onesNibble = (R.A & 0x0F) - (num & 0x0F) - (R.Flags & FLAG_CARRY ? 1 : 0);
			int8_t tensNibble = ((R.A & 0xF0) >> 4) - ((num & 0xF0) >> 4);

			if (onesNibble < 0x00)
			{
				onesNibble += 10;
				--tensNibble;
			}

			bool carried = false;
			if (tensNibble < 0x00)
			{
				tensNibble += 10;
				carried = true;
			}

			R.A = ((tensNibble << 4) & 0xF0) | (onesNibble & 0x0F);

			R.Flags &= ~(FLAG_ZERO | FLAG_NEGATIVE | FLAG_CARRY | FLAG_OVERFLOW);
			R.Flags |= (R.A == 0 ? FLAG_ZERO : 0)
				| ((0x80 & R.A) ? FLAG_NEGATIVE : 0)
				| (carried ? FLAG_CARRY : 0)
				| (((0x80 & oldA) ^ (0x80 & R.A)) ? FLAG_OVERFLOW : 0);
		}
		else
		{
			R.A -= num - (R.Flags & FLAG_CARRY ? 1 : 0);

			R.Flags &= ~(FLAG_ZERO | FLAG_NEGATIVE | FLAG_CARRY | FLAG_OVERFLOW);
			R.Flags |= (R.A == 0 ? FLAG_ZERO : 0)
				| ((0x80 & R.A) ? FLAG_NEGATIVE : 0)
				| (((int16_t)oldA - (int16_t)num - (R.Flags & FLAG_CARRY ? 1 : 0) < 0) ? FLAG_CARRY : 0)
				| (( (0x80 & oldA) ^ (0x80 & R.A)) ? FLAG_OVERFLOW : 0);
		}
	}

	void T_General(uint8_t source, uint8_t &dest)
	{
		dest = source;
		R.Flags &= ~(FLAG_ZERO | FLAG_NEGATIVE);
		R.Flags |= (dest == 0 ? FLAG_ZERO : 0) | ((0x80 & dest) ? FLAG_NEGATIVE : 0);
	}

	// Push a byte onto the stack.
	void PushStackGeneral(uint8_t num)
	{
		Bus->write(0x0100 | ((uint16_t)R.SP), num);
		--R.SP;
	}

	// Pull a byte back from the stack.
	uint8_t PullStackGeneral()
	{
		++R.SP;
		return Bus->read(0x0100 | ((uint16_t)R.SP));
	}

	/*
	 **** SPECIFIC VERSIONS OF INSTRUCTIONS HERE ****
	 */
	int ADC_Immediate()
	{
		ADC_General(Bus->read(++R.PC));
		++R.PC;
		return 2;
	}

	int ADC_ZP()
	{
		ADC_General(Bus->read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int ADC_ZPX()
	{
		ADC_General(Bus->read(ResolveZPX()));
		++R.PC;
		return 4;
	}

	int ADC_Absolute()
	{
		ADC_General(Bus->read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int ADC_AbsoluteX()
	{
		bool pageCrossed;
		ADC_General(Bus->read(ResolveAbsoluteX(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int ADC_AbsoluteY()
	{
		bool pageCrossed;
		ADC_General(Bus->read(ResolveAbsoluteY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int ADC_IndirectX()
	{
		ADC_General(Bus->read(ResolveIndirectX()));
		++R.PC;
		return 6;
	}

	int ADC_IndirectY()
	{
		bool pageCrossed;
		ADC_General(Bus->read(ResolveIndirectY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 6 : 5;
	}

	int AND_Immediate()
	{
		AND_General(Bus->read(++R.PC));
		++R.PC;
		return 2;
	}

	int AND_ZP()
	{
		AND_General(Bus->read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int AND_ZPX()
	{
		AND_General(Bus->read(ResolveZPX()));
		++R.PC;
		return 4;
	}

	int AND_Absolute()
	{
		AND_General(Bus->read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int AND_AbsoluteX()
	{
		bool pageCrossed;
		AND_General(Bus->read(ResolveAbsoluteX(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int AND_AbsoluteY()
	{
		bool pageCrossed;
		AND_General(Bus->read(ResolveAbsoluteY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int AND_IndirectX()
	{
		AND_General(Bus->read(ResolveIndirectX()));
		++R.PC;
		return 6;
	}

	int AND_IndirectY()
	{
		bool pageCrossed;
		AND_General(Bus->read(ResolveIndirectY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 6 : 5;
	}

	int ASL_Accumulator()
	{
		ASL_General(R.A);
		++R.PC;
		return 2;
	}

	int ASL_ZP()
	{
		uint8_t address = ResolveZP();
		uint8_t num = Bus->read(address);
		ASL_General(num);
		Bus->write(address, num);
		++R.PC;
		return 5;
	}

	int ASL_ZPX()
	{
		uint8_t address = ResolveZPX();
		uint8_t num = Bus->read(address);
		ASL_General(num);
		Bus->write(address, num);
		++R.PC;
		return 6;
	}

	int ASL_Absolute()
	{
		uint8_t address = ResolveAbsolute();
		uint8_t num = Bus->read(address);
		ASL_General(num);
		Bus->write(address, num);
		++R.PC;
		return 6;
	}

	int ASL_AbsoluteX()
	{
		bool temp;
		uint8_t address = ResolveAbsoluteX(temp); // TODO: This doesn't actually need this variable, maybe provide an implementation that ignores new pages.
		uint8_t num = Bus->read(address);
		ASL_General(num);
		Bus->write(address, num);
		++R.PC;
		return 7;
	}

	int BCC()
	{
		return BranchGeneral(!(R.Flags & FLAG_CARRY));
	}

	int BCS()
	{
		return BranchGeneral(R.Flags & FLAG_CARRY);
	}

	int BEQ()
	{
		return BranchGeneral(R.Flags & FLAG_ZERO);
	}

	int BIT_ZP()
	{
		BIT_General(Bus->read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int BIT_Absolute()
	{
		BIT_General(Bus->read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int BMI()
	{
		return BranchGeneral(R.Flags & FLAG_NEGATIVE);
	}

	int BNE()
	{
		return BranchGeneral(!(R.Flags & FLAG_ZERO));
	}

	int BPL()
	{
		return BranchGeneral(!(R.Flags & FLAG_NEGATIVE));
	}

	int BRK()
	{
		R.PC += 2; // 6502 Claims that BRK is a 1 byte instruction, but see http://nesdev.com/the%20%27B%27%20flag%20&%20BRK%20instruction.txt
		doIRQ();
		R.Flags |= FLAG_BRK;
		return 7;
	}

	int BVC()
	{
		return BranchGeneral(!(R.Flags & FLAG_OVERFLOW));
	}

	int BVS()
	{
		return BranchGeneral(R.Flags & FLAG_OVERFLOW);
	}

	int CLC()
	{
		R.Flags &= ~FLAG_CARRY;
		++R.PC;
		return 2;
	}

	int CLD()
	{
		R.Flags &= ~FLAG_DECIMAL;
		++R.PC;
		return 2;
	}

	int CLI()
	{
		R.Flags &= ~FLAG_INTERRUPT;
		++R.PC;
		return 2;
	}

	int CLV()
	{
		R.Flags &= ~FLAG_OVERFLOW;
		++R.PC;
		return 2;
	}

	int CMP_Immediate()
	{
		CMP_General(R.A, Bus->read(++R.PC));
		++R.PC;
		return 2;
	}

	int CMP_ZP()
	{
		CMP_General(R.A, Bus->read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int CMP_ZPX()
	{
		CMP_General(R.A, Bus->read(ResolveZPX()));
		++R.PC;
		return 4;
	}

	int CMP_Absolute()
	{
		CMP_General(R.A, Bus->read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int CMP_AbsoluteX()
	{
		bool pageCrossed;
		CMP_General(R.A, Bus->read(ResolveAbsoluteX(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int CMP_AbsoluteY()
	{
		bool pageCrossed;
		CMP_General(R.A, Bus->read(ResolveAbsoluteY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int CMP_IndirectX()
	{
		CMP_General(R.A, Bus->read(ResolveIndirectX()));
		++R.PC;
		return 6;
	}

	int CMP_IndirectY()
	{
		bool pageCrossed;
		CMP_General(R.A, Bus->read(ResolveIndirectY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 6 : 5;
	}

	int CPX_Immediate()
	{
		CMP_General(R.X, Bus->read(++R.PC));
		++R.PC;
		return 2;
	}

	int CPX_ZP()
	{
		CMP_General(R.X, Bus->read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int CPX_Absolute()
	{
		CMP_General(R.X, Bus->read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int CPY_Immediate()
	{
		CMP_General(R.Y, Bus->read(++R.PC));
		++R.PC;
		return 2;
	}

	int CPY_ZP()
	{
		CMP_General(R.Y, Bus->read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int CPY_Absolute()
	{
		CMP_General(R.Y, Bus->read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int DEC_ZP()
	{
		uint16_t address = ResolveZPX();
		uint8_t num = Bus->read(address);
		DEC_General(num);
		Bus->write(address, num);
		++R.PC;
		return 5;
	}

	int DEC_ZPX()
	{
		uint16_t address = ResolveZPX();
		uint8_t num = Bus->read(address);
		DEC_General(num);
		Bus->write(address, num);
		++R.PC;
		return 6;
	}

	int DEC_Absolute()
	{
		uint16_t address = ResolveAbsolute();
		uint8_t num = Bus->read(address);
		DEC_General(num);
		Bus->write(address, num);
		++R.PC;
		return 6;
	}

	int DEC_AbsoluteX()
	{
		bool t;
		uint16_t address = ResolveAbsoluteX(t);
		uint8_t num = Bus->read(address);
		DEC_General(num);
		Bus->write(address, num);
		++R.PC;
		return 7;
	}

	int DEX()
	{
		DEC_General(R.X);
		++R.PC;
		return 2;
	}

	int DEY()
	{
		DEC_General(R.Y);
		++R.PC;
		return 2;
	}

	int EOR_Immediate()
	{
		EOR_General(Bus->read(++R.PC));
		++R.PC;
		return 2;
	}

	int EOR_ZP()
	{
		EOR_General(Bus->read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int EOR_ZPX()
	{
		EOR_General(Bus->read(ResolveZPX()));
		++R.PC;
		return 4;
	}

	int EOR_Absolute()
	{
		EOR_General(Bus->read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int EOR_AbsoluteX()
	{
		bool pageCrossed;
		EOR_General(Bus->read(ResolveAbsoluteX(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int EOR_AbsoluteY()
	{
		bool pageCrossed;
		EOR_General(Bus->read(ResolveAbsoluteY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int EOR_IndirectX()
	{
		EOR_General(Bus->read(ResolveIndirectX()));
		++R.PC;
		return 6;
	}

	int EOR_IndirectY()
	{
		bool pageCrossed;
		EOR_General(Bus->read(ResolveIndirectY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 6 : 5;
	}

	int INC_ZP()
	{
		uint16_t address = ResolveZPX();
		uint8_t num = Bus->read(address);
		INC_General(num);
		Bus->write(address, num);
		++R.PC;
		return 5;
	}

	int INC_ZPX()
	{
		uint16_t address = ResolveZPX();
		uint8_t num = Bus->read(address);
		INC_General(num);
		Bus->write(address, num);
		++R.PC;
		return 6;
	}

	int INC_Absolute()
	{
		uint16_t address = ResolveAbsolute();
		uint8_t num = Bus->read(address);
		INC_General(num);
		Bus->write(address, num);
		++R.PC;
		return 6;
	}

	int INC_AbsoluteX()
	{
		bool t;
		uint16_t address = ResolveAbsoluteX(t);
		uint8_t num = Bus->read(address);
		INC_General(num);
		Bus->write(address, num);
		++R.PC;
		return 7;
	}

	int INX()
	{
		INC_General(R.X);
		++R.PC;
		return 2;
	}

	int INY()
	{
		INC_General(R.Y);
		++R.PC;
		return 2;
	}

	int JMP_Absolute()
	{
		uint16_t jmpAddress = ((uint16_t)Bus->read(++R.PC)) | (((uint16_t)Bus->read(++R.PC)) << 8);
		R.PC = jmpAddress;
		return 3;
	}

	int JMP_Indirect()
	{
		uint16_t indirectAddress = ((uint16_t)Bus->read(++R.PC)) | (((uint16_t)Bus->read(++R.PC)) << 8);
		uint16_t jmpAddress = ((uint16_t)Bus->read(indirectAddress)) | (((uint16_t)Bus->read(indirectAddress + 1)) << 8);
		R.PC = jmpAddress;
		return 5;
	}

	int JSR()
	{
		uint16_t jmpAddress = ((uint16_t)Bus->read(++R.PC)) | (((uint16_t)Bus->read(++R.PC)) << 8);
		PushStackGeneral((uint8_t)((R.PC >> 8) & 0x00FF)); // Push High byte onto stack
		PushStackGeneral((uint8_t)(R.PC & 0x00FF)); // Push low byte onto stack
		R.PC = jmpAddress; // Jump to new address
		return 6;
	}

	int LDA_Immediate()
	{
		LD_General(R.A, Bus->read(++R.PC));
		++R.PC;
		return 2;
	}

	int LDA_ZP()
	{
		LD_General(R.A, Bus->read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int LDA_ZPX()
	{
		LD_General(R.A, Bus->read(ResolveZPX()));
		++R.PC;
		return 4;
	}

	int LDA_Absolute()
	{
		LD_General(R.A, Bus->read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int LDA_AbsoluteX()
	{
		bool pageCrossed;
		LD_General(R.A, Bus->read(ResolveAbsoluteX(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int LDA_AbsoluteY()
	{
		bool pageCrossed;
		LD_General(R.A, Bus->read(ResolveAbsoluteY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int LDA_IndirectX()
	{
		LD_General(R.A, Bus->read(ResolveIndirectX()));
		++R.PC;
		return 6;
	}

	int LDA_IndirectY()
	{
		bool pageCrossed;
		LD_General(R.A, Bus->read(ResolveIndirectY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 6 : 5;
	}

	int LDX_Immediate()
	{
		LD_General(R.X, Bus->read(++R.PC));
		++R.PC;
		return 2;
	}

	int LDX_ZP()
	{
		LD_General(R.X, Bus->read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int LDX_ZPY()
	{
		LD_General(R.X, Bus->read(ResolveZPY()));
		++R.PC;
		return 4;
	}

	int LDX_Absolute()
	{
		LD_General(R.X, Bus->read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int LDX_AbsoluteY()
	{
		bool pageCrossed;
		LD_General(R.X, Bus->read(ResolveAbsoluteY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int LDY_Immediate()
	{
		LD_General(R.Y, Bus->read(++R.PC));
		++R.PC;
		return 2;
	}

	int LDY_ZP()
	{
		LD_General(R.Y, Bus->read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int LDY_ZPX()
	{
		LD_General(R.Y, Bus->read(ResolveZPX()));
		++R.PC;
		return 4;
	}

	int LDY_Absolute()
	{
		LD_General(R.Y, Bus->read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int LDY_AbsoluteX()
	{
		bool pageCrossed;
		LD_General(R.Y, Bus->read(ResolveAbsoluteX(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int LSR_Accumulator()
	{
		LSR_General(R.A);
		++R.PC;
		return 2;
	}

	int LSR_ZP()
	{
		uint8_t address = ResolveZP();
		uint8_t num = Bus->read(address);
		LSR_General(num);
		Bus->write(address, num);
		++R.PC;
		return 5;
	}

	int LSR_ZPX()
	{
		uint8_t address = ResolveZPX();
		uint8_t num = Bus->read(address);
		LSR_General(num);
		Bus->write(address, num);
		++R.PC;
		return 6;
	}

	int LSR_Absolute()
	{
		uint8_t address = ResolveAbsolute();
		uint8_t num = Bus->read(address);
		LSR_General(num);
		Bus->write(address, num);
		++R.PC;
		return 6;
	}

	int LSR_AbsoluteX()
	{
		bool temp;
		uint8_t address = ResolveAbsoluteX(temp); // TODO: This doesn't actually need this variable, maybe provide an implementation that ignores new pages.
		uint8_t num = Bus->read(address);
		LSR_General(num);
		Bus->write(address, num);
		++R.PC;
		return 7;
	}

	int NOP()
	{
		++R.PC;
		return 2;
	}

	int ORA_Immediate()
	{
		ORA_General(Bus->read(++R.PC));
		++R.PC;
		return 2;
	}

	int ORA_ZP()
	{
		ORA_General(Bus->read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int ORA_ZPX()
	{
		ORA_General(Bus->read(ResolveZPX()));
		++R.PC;
		return 4;
	}

	int ORA_Absolute()
	{
		ORA_General(Bus->read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int ORA_AbsoluteX()
	{
		bool pageCrossed;
		ORA_General(Bus->read(ResolveAbsoluteX(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int ORA_AbsoluteY()
	{
		bool pageCrossed;
		ORA_General(Bus->read(ResolveAbsoluteY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int ORA_IndirectX()
	{
		ORA_General(Bus->read(ResolveIndirectX()));
		++R.PC;
		return 6;
	}

	int ORA_IndirectY()
	{
		bool pageCrossed;
		ORA_General(Bus->read(ResolveIndirectY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 6 : 5;
	}

	int PHA()
	{
		PushStackGeneral(R.A);
		++R.PC;
		return 3;
	}

	int PHP()
	{
		PushStackGeneral(R.Flags);
		++R.PC;
		return 3;
	}

	int PLA()
	{
		R.A = PullStackGeneral();
		++R.PC;
		return 4;
	}

	int PLP()
	{
		R.Flags = PullStackGeneral();
		++R.PC;
		return 4;
	}

	int ROL_Accumulator()
	{
		ROL_General(R.A);
		++R.PC;
		return 2;
	}

	int ROL_ZP()
	{
		uint8_t address = ResolveZP();
		uint8_t num = Bus->read(address);
		ROL_General(num);
		Bus->write(address, num);
		++R.PC;
		return 5;
	}

	int ROL_ZPX()
	{
		uint8_t address = ResolveZPX();
		uint8_t num = Bus->read(address);
		ROL_General(num);
		Bus->write(address, num);
		++R.PC;
		return 6;
	}

	int ROL_Absolute()
	{
		uint8_t address = ResolveAbsolute();
		uint8_t num = Bus->read(address);
		ROL_General(num);
		Bus->write(address, num);
		++R.PC;
		return 6;
	}

	int ROL_AbsoluteX()
	{
		bool temp;
		uint8_t address = ResolveAbsoluteX(temp); // TODO: This doesn't actually need this variable, maybe provide an implementation that ignores new pages.
		uint8_t num = Bus->read(address);
		ROL_General(num);
		Bus->write(address, num);
		++R.PC;
		return 7;
	}

	int ROR_Accumulator()
	{
		ROR_General(R.A);
		++R.PC;
		return 2;
	}

	int ROR_ZP()
	{
		uint8_t address = ResolveZP();
		uint8_t num = Bus->read(address);
		ROR_General(num);
		Bus->write(address, num);
		++R.PC;
		return 5;
	}

	int ROR_ZPX()
	{
		uint8_t address = ResolveZPX();
		uint8_t num = Bus->read(address);
		ROR_General(num);
		Bus->write(address, num);
		++R.PC;
		return 6;
	}

	int ROR_Absolute()
	{
		uint8_t address = ResolveAbsolute();
		uint8_t num = Bus->read(address);
		ROR_General(num);
		Bus->write(address, num);
		++R.PC;
		return 6;
	}

	int ROR_AbsoluteX()
	{
		bool temp;
		uint8_t address = ResolveAbsoluteX(temp); // TODO: This doesn't actually need this variable, maybe provide an implementation that ignores new pages.
		uint8_t num = Bus->read(address);
		ROR_General(num);
		Bus->write(address, num);
		++R.PC;
		return 7;
	}

	int RTI()
	{
		R.Flags = PullStackGeneral();
		R.PC = ((uint16_t)PullStackGeneral()) | (((uint16_t)PullStackGeneral()) << 8);
		return 6;
	}

	int RTS()
	{
		R.PC = ((uint16_t)PullStackGeneral()) | (((uint16_t)PullStackGeneral()) << 8) + 1;
		return 6;
	}

	int SBC_Immediate()
	{
		SBC_General(Bus->read(++R.PC));
		++R.PC;
		return 2;
	}

	int SBC_ZP()
	{
		SBC_General(Bus->read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int SBC_ZPX()
	{
		SBC_General(Bus->read(ResolveZPX()));
		++R.PC;
		return 4;
	}

	int SBC_Absolute()
	{
		SBC_General(Bus->read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int SBC_AbsoluteX()
	{
		bool pageCrossed;
		SBC_General(Bus->read(ResolveAbsoluteX(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int SBC_AbsoluteY()
	{
		bool pageCrossed;
		SBC_General(Bus->read(ResolveAbsoluteY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int SBC_IndirectX()
	{
		SBC_General(Bus->read(ResolveIndirectX()));
		++R.PC;
		return 6;
	}

	int SBC_IndirectY()
	{
		bool pageCrossed;
		SBC_General(Bus->read(ResolveIndirectY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 6 : 5;
	}

	int SEC()
	{
		R.Flags |= FLAG_CARRY;
		++R.PC;
		return 2;
	}

	int SED()
	{
		R.Flags |= FLAG_DECIMAL;
		++R.PC;
		return 2;
	}

	int SEI()
	{
		R.Flags |= FLAG_INTERRUPT;
		++R.PC;
		return 2;
	}

	int STA_ZP()
	{
		Bus->write(ResolveZP(), R.A);
		++R.PC;
		return 3;
	}

	int STA_ZPX()
	{
		Bus->write(ResolveZPX(), R.A);
		++R.PC;
		return 4;
	}

	int STA_Absolute()
	{
		Bus->write(ResolveAbsolute(), R.A);
		++R.PC;
		return 4;
	}

	int STA_AbsoluteX()
	{
		bool pageCrossed;
		Bus->write(ResolveAbsoluteX(pageCrossed), R.A);
		++R.PC;
		return 5;
	}

	int STA_AbsoluteY()
	{
		bool pageCrossed;
		Bus->write(ResolveAbsoluteY(pageCrossed), R.A);
		++R.PC;
		return 5;
	}

	int STA_IndirectX()
	{
		Bus->write(ResolveIndirectX(), R.A);
		++R.PC;
		return 6;
	}

	int STA_IndirectY()
	{
		bool pageCrossed;
		Bus->write(ResolveIndirectY(pageCrossed), R.A);
		++R.PC;
		return 6;
	}

	int STX_ZP()
	{
		Bus->write(ResolveZP(), R.X);
		++R.PC;
		return 3;
	}

	int STX_ZPY()
	{
		Bus->write(ResolveZPY(), R.X);
		++R.PC;
		return 4;
	}

	int STX_Absolute()
	{
		Bus->write(ResolveAbsolute(), R.X);
		++R.PC;
		return 4;
	}

	int STY_ZP()
	{
		Bus->write(ResolveZP(), R.Y);
		++R.PC;
		return 3;
	}

	int STY_ZPX()
	{
		Bus->write(ResolveZPX(), R.Y);
		++R.PC;
		return 4;
	}

	int STY_Absolute()
	{
		Bus->write(ResolveAbsolute(), R.Y);
		++R.PC;
		return 4;
	}

	int TAX()
	{
		T_General(R.A, R.X);
		++R.PC;
		return 2;
	}

	int TAY()
	{
		T_General(R.A, R.Y);
		++R.PC;
		return 2;
	}

	int TSX()
	{
		T_General(R.SP, R.X);
		++R.PC;
		return 2;
	}

	int TXA()
	{
		T_General(R.X, R.A);
		++R.PC;
		return 2;
	}

	int TXS()
	{
		R.SP = R.X;
		++R.PC;
		return 2;
	}

	int TYA()
	{
		T_General(R.Y, R.A);
		++R.PC;
		return 2;
	}
};
