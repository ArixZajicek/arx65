#include "Processor.h"

/* Simplify bus functions to just read and write. */
using arx65::Databus::read;
using arx65::Databus::write;

namespace arx65::Processor
{
	// Set of register info 
	RegisterSet R;

	// Array of function pointers, one for each possible instruction
	int (*instruction[256])();

	RegisterSet *getRegisters()
	{
		return &R;
	}

	RegisterSet getRegistersCopy()
	{
		return R;
	}

	int doNextInstruction()
	{
		return (*instruction[read(R.PC)])();
	}

	// Push a byte onto the stack.
	void PushStackGeneral(uint8_t num)
	{
		write(0x0100 | ((uint16_t)R.SP), num);
		--R.SP;
	}

	// Pull a byte back from the stack.
	uint8_t PullStackGeneral()
	{
		++R.SP;
		return read(0x0100 | ((uint16_t)R.SP));
	}

	void doNMI() {
		PushStackGeneral((R.PC >> 8) & 0x00FF);
		PushStackGeneral((R.PC) & 0x00FF);
		PushStackGeneral(R.Flags);
		R.PC = ((uint16_t)read(0xFFFA)) | (((uint16_t)read(0xFFFB)) << 8);
	}

	// Reset registers to initial values
	void doRES()
	{
		R.A = 0;
		R.X = 0;
		R.Y = 0;
		R.SP = 0xFF;
		R.Flags = FLAG_INTERRUPT;
		R.PC = read(0xFFFC) | (read(0xFFFD) << 8);
	}

	void doIRQ() {
		// Push current PC onto stack, high then low byte. Then push flags
		if (!(R.Flags & FLAG_INTERRUPT))
		{
			PushStackGeneral((R.PC >> 8) & 0x00FF);
			PushStackGeneral((R.PC) & 0x00FF);
			PushStackGeneral(R.Flags);
			R.PC = ((uint16_t)read(0xFFFE)) | (((uint16_t)read(0xFFFF)) << 8);
		}
	}

	uint8_t InvalidInstruction()
	{
		std::cout << "Invalid instruction 0x" << HEX(2, read(R.PC)) << " at 0x" << HEX(4, R.PC) << "." << std::endl;
		++R.PC;
		return 0;
	}

	/* Reolve a direct Zero Page address. (Advances PC + 1) */
	uint16_t ResolveZP()
	{
		return read(++R.PC);
	}

	/* Resolve a direct zero page X, with X offset, including wraparound. (Advances PC + 1) */
	uint16_t ResolveZPX()
	{
		return (read(++R.PC) + R.X) & 0x00FF;
	}

	/* Resolve a direct zero page Y, with Y offset, including wraparound. (Advances PC + 1) */
	uint16_t ResolveZPY()
	{
		return (read(++R.PC) + R.Y) & 0x00FF; // This & might not be necessary since both values are already of uint8_t type
	}

	/* Resolve a direct address (advances PC + 2)*/
	uint16_t ResolveAbsolute()
	{
		return (uint16_t)read(++R.PC) | (((uint16_t)read(++R.PC)) << 8);
	}

	/* Resolve direct address with X offset, and pageCrossed will be appropriately set (advances PC + 2)*/
	uint16_t ResolveAbsoluteX(bool &pageCrossed)
	{
		uint16_t addressBeforeAdding = (uint16_t)read(++R.PC) | (((uint16_t)read(++R.PC)) << 8);

		// To determine if a page was crossed, we just see if the most significant byte is bigger
		pageCrossed = (0xFF00 & (addressBeforeAdding + R.X) > (0xFF00 & addressBeforeAdding));
		return addressBeforeAdding + R.X;
	}

	/* Resolve direct address with Y offset, and pageCrossed will be appropriately set (advances PC + 2)*/
	uint16_t ResolveAbsoluteY(bool &pageCrossed)
	{
		uint16_t addressBeforeAdding = (uint16_t)read(++R.PC) | (((uint16_t)read(++R.PC)) << 8);

		// To determine if a page was crossed, we just see if the most significant byte is bigger
		pageCrossed = (0xFF00 & (addressBeforeAdding + R.Y) > (0xFF00 & addressBeforeAdding));
		return addressBeforeAdding + R.Y;
	}

	/* Resolves an indirect address at zero page + X, then resolve. (advances PC + 1) */
	uint16_t ResolveIndirectX()
	{
		uint8_t zpAddress = read(++R.PC + R.X);
		return ((uint16_t)read(zpAddress)) | ((uint16_t)read(zpAddress + 1) << 8);
	}

	/* Resolves an indirect address at zero page, then add Y, then resolve. (advances PC + 1) */
	uint16_t ResolveIndirectY(bool &pageCrossed)
	{
		uint8_t zpAddress = read(++R.PC);
		uint16_t addressBeforeAdding = ((uint16_t)read(zpAddress)) | ((uint16_t)read(zpAddress + 1) << 8);

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
			R.PC += (int8_t)read(++R.PC);
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

	/*
	 **** SPECIFIC VERSIONS OF INSTRUCTIONS HERE ****
	 */
	int ADC_Immediate()
	{
		ADC_General(read(++R.PC));
		++R.PC;
		return 2;
	}

	int ADC_ZP()
	{
		ADC_General(read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int ADC_ZPX()
	{
		ADC_General(read(ResolveZPX()));
		++R.PC;
		return 4;
	}

	int ADC_Absolute()
	{
		ADC_General(read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int ADC_AbsoluteX()
	{
		bool pageCrossed;
		ADC_General(read(ResolveAbsoluteX(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int ADC_AbsoluteY()
	{
		bool pageCrossed;
		ADC_General(read(ResolveAbsoluteY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int ADC_IndirectX()
	{
		ADC_General(read(ResolveIndirectX()));
		++R.PC;
		return 6;
	}

	int ADC_IndirectY()
	{
		bool pageCrossed;
		ADC_General(read(ResolveIndirectY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 6 : 5;
	}

	int AND_Immediate()
	{
		AND_General(read(++R.PC));
		++R.PC;
		return 2;
	}

	int AND_ZP()
	{
		AND_General(read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int AND_ZPX()
	{
		AND_General(read(ResolveZPX()));
		++R.PC;
		return 4;
	}

	int AND_Absolute()
	{
		AND_General(read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int AND_AbsoluteX()
	{
		bool pageCrossed;
		AND_General(read(ResolveAbsoluteX(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int AND_AbsoluteY()
	{
		bool pageCrossed;
		AND_General(read(ResolveAbsoluteY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int AND_IndirectX()
	{
		AND_General(read(ResolveIndirectX()));
		++R.PC;
		return 6;
	}

	int AND_IndirectY()
	{
		bool pageCrossed;
		AND_General(read(ResolveIndirectY(pageCrossed)));
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
		uint8_t num = read(address);
		ASL_General(num);
		write(address, num);
		++R.PC;
		return 5;
	}

	int ASL_ZPX()
	{
		uint8_t address = ResolveZPX();
		uint8_t num = read(address);
		ASL_General(num);
		write(address, num);
		++R.PC;
		return 6;
	}

	int ASL_Absolute()
	{
		uint8_t address = ResolveAbsolute();
		uint8_t num = read(address);
		ASL_General(num);
		write(address, num);
		++R.PC;
		return 6;
	}

	int ASL_AbsoluteX()
	{
		bool temp;
		uint8_t address = ResolveAbsoluteX(temp); // TODO: This doesn't actually need this variable, maybe provide an implementation that ignores new pages.
		uint8_t num = read(address);
		ASL_General(num);
		write(address, num);
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
		BIT_General(read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int BIT_Absolute()
	{
		BIT_General(read(ResolveAbsolute()));
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
		CMP_General(R.A, read(++R.PC));
		++R.PC;
		return 2;
	}

	int CMP_ZP()
	{
		CMP_General(R.A, read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int CMP_ZPX()
	{
		CMP_General(R.A, read(ResolveZPX()));
		++R.PC;
		return 4;
	}

	int CMP_Absolute()
	{
		CMP_General(R.A, read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int CMP_AbsoluteX()
	{
		bool pageCrossed;
		CMP_General(R.A, read(ResolveAbsoluteX(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int CMP_AbsoluteY()
	{
		bool pageCrossed;
		CMP_General(R.A, read(ResolveAbsoluteY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int CMP_IndirectX()
	{
		CMP_General(R.A, read(ResolveIndirectX()));
		++R.PC;
		return 6;
	}

	int CMP_IndirectY()
	{
		bool pageCrossed;
		CMP_General(R.A, read(ResolveIndirectY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 6 : 5;
	}

	int CPX_Immediate()
	{
		CMP_General(R.X, read(++R.PC));
		++R.PC;
		return 2;
	}

	int CPX_ZP()
	{
		CMP_General(R.X, read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int CPX_Absolute()
	{
		CMP_General(R.X, read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int CPY_Immediate()
	{
		CMP_General(R.Y, read(++R.PC));
		++R.PC;
		return 2;
	}

	int CPY_ZP()
	{
		CMP_General(R.Y, read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int CPY_Absolute()
	{
		CMP_General(R.Y, read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int DEC_ZP()
	{
		uint16_t address = ResolveZPX();
		uint8_t num = read(address);
		DEC_General(num);
		write(address, num);
		++R.PC;
		return 5;
	}

	int DEC_ZPX()
	{
		uint16_t address = ResolveZPX();
		uint8_t num = read(address);
		DEC_General(num);
		write(address, num);
		++R.PC;
		return 6;
	}

	int DEC_Absolute()
	{
		uint16_t address = ResolveAbsolute();
		uint8_t num = read(address);
		DEC_General(num);
		write(address, num);
		++R.PC;
		return 6;
	}

	int DEC_AbsoluteX()
	{
		bool t;
		uint16_t address = ResolveAbsoluteX(t);
		uint8_t num = read(address);
		DEC_General(num);
		write(address, num);
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
		EOR_General(read(++R.PC));
		++R.PC;
		return 2;
	}

	int EOR_ZP()
	{
		EOR_General(read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int EOR_ZPX()
	{
		EOR_General(read(ResolveZPX()));
		++R.PC;
		return 4;
	}

	int EOR_Absolute()
	{
		EOR_General(read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int EOR_AbsoluteX()
	{
		bool pageCrossed;
		EOR_General(read(ResolveAbsoluteX(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int EOR_AbsoluteY()
	{
		bool pageCrossed;
		EOR_General(read(ResolveAbsoluteY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int EOR_IndirectX()
	{
		EOR_General(read(ResolveIndirectX()));
		++R.PC;
		return 6;
	}

	int EOR_IndirectY()
	{
		bool pageCrossed;
		EOR_General(read(ResolveIndirectY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 6 : 5;
	}

	int INC_ZP()
	{
		uint16_t address = ResolveZPX();
		uint8_t num = read(address);
		INC_General(num);
		write(address, num);
		++R.PC;
		return 5;
	}

	int INC_ZPX()
	{
		uint16_t address = ResolveZPX();
		uint8_t num = read(address);
		INC_General(num);
		write(address, num);
		++R.PC;
		return 6;
	}

	int INC_Absolute()
	{
		uint16_t address = ResolveAbsolute();
		uint8_t num = read(address);
		INC_General(num);
		write(address, num);
		++R.PC;
		return 6;
	}

	int INC_AbsoluteX()
	{
		bool t;
		uint16_t address = ResolveAbsoluteX(t);
		uint8_t num = read(address);
		INC_General(num);
		write(address, num);
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
		uint16_t jmpAddress = ((uint16_t)read(++R.PC)) | (((uint16_t)read(++R.PC)) << 8);
		R.PC = jmpAddress;
		return 3;
	}

	int JMP_Indirect()
	{
		uint16_t indirectAddress = ((uint16_t)read(++R.PC)) | (((uint16_t)read(++R.PC)) << 8);
		uint16_t jmpAddress = ((uint16_t)read(indirectAddress)) | (((uint16_t)read(indirectAddress + 1)) << 8);
		R.PC = jmpAddress;
		return 5;
	}

	int JSR()
	{
		uint16_t jmpAddress = ((uint16_t)read(++R.PC)) | (((uint16_t)read(++R.PC)) << 8);
		PushStackGeneral((uint8_t)((R.PC >> 8) & 0x00FF)); // Push High byte onto stack
		PushStackGeneral((uint8_t)(R.PC & 0x00FF)); // Push low byte onto stack
		R.PC = jmpAddress; // Jump to new address
		return 6;
	}

	int LDA_Immediate()
	{
		LD_General(R.A, read(++R.PC));
		++R.PC;
		return 2;
	}

	int LDA_ZP()
	{
		LD_General(R.A, read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int LDA_ZPX()
	{
		LD_General(R.A, read(ResolveZPX()));
		++R.PC;
		return 4;
	}

	int LDA_Absolute()
	{
		LD_General(R.A, read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int LDA_AbsoluteX()
	{
		bool pageCrossed;
		LD_General(R.A, read(ResolveAbsoluteX(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int LDA_AbsoluteY()
	{
		bool pageCrossed;
		LD_General(R.A, read(ResolveAbsoluteY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int LDA_IndirectX()
	{
		LD_General(R.A, read(ResolveIndirectX()));
		++R.PC;
		return 6;
	}

	int LDA_IndirectY()
	{
		bool pageCrossed;
		LD_General(R.A, read(ResolveIndirectY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 6 : 5;
	}

	int LDX_Immediate()
	{
		LD_General(R.X, read(++R.PC));
		++R.PC;
		return 2;
	}

	int LDX_ZP()
	{
		LD_General(R.X, read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int LDX_ZPY()
	{
		LD_General(R.X, read(ResolveZPY()));
		++R.PC;
		return 4;
	}

	int LDX_Absolute()
	{
		LD_General(R.X, read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int LDX_AbsoluteY()
	{
		bool pageCrossed;
		LD_General(R.X, read(ResolveAbsoluteY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int LDY_Immediate()
	{
		LD_General(R.Y, read(++R.PC));
		++R.PC;
		return 2;
	}

	int LDY_ZP()
	{
		LD_General(R.Y, read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int LDY_ZPX()
	{
		LD_General(R.Y, read(ResolveZPX()));
		++R.PC;
		return 4;
	}

	int LDY_Absolute()
	{
		LD_General(R.Y, read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int LDY_AbsoluteX()
	{
		bool pageCrossed;
		LD_General(R.Y, read(ResolveAbsoluteX(pageCrossed)));
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
		uint8_t num = read(address);
		LSR_General(num);
		write(address, num);
		++R.PC;
		return 5;
	}

	int LSR_ZPX()
	{
		uint8_t address = ResolveZPX();
		uint8_t num = read(address);
		LSR_General(num);
		write(address, num);
		++R.PC;
		return 6;
	}

	int LSR_Absolute()
	{
		uint8_t address = ResolveAbsolute();
		uint8_t num = read(address);
		LSR_General(num);
		write(address, num);
		++R.PC;
		return 6;
	}

	int LSR_AbsoluteX()
	{
		bool temp;
		uint8_t address = ResolveAbsoluteX(temp); // TODO: This doesn't actually need this variable, maybe provide an implementation that ignores new pages.
		uint8_t num = read(address);
		LSR_General(num);
		write(address, num);
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
		ORA_General(read(++R.PC));
		++R.PC;
		return 2;
	}

	int ORA_ZP()
	{
		ORA_General(read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int ORA_ZPX()
	{
		ORA_General(read(ResolveZPX()));
		++R.PC;
		return 4;
	}

	int ORA_Absolute()
	{
		ORA_General(read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int ORA_AbsoluteX()
	{
		bool pageCrossed;
		ORA_General(read(ResolveAbsoluteX(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int ORA_AbsoluteY()
	{
		bool pageCrossed;
		ORA_General(read(ResolveAbsoluteY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int ORA_IndirectX()
	{
		ORA_General(read(ResolveIndirectX()));
		++R.PC;
		return 6;
	}

	int ORA_IndirectY()
	{
		bool pageCrossed;
		ORA_General(read(ResolveIndirectY(pageCrossed)));
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
		uint8_t num = read(address);
		ROL_General(num);
		write(address, num);
		++R.PC;
		return 5;
	}

	int ROL_ZPX()
	{
		uint8_t address = ResolveZPX();
		uint8_t num = read(address);
		ROL_General(num);
		write(address, num);
		++R.PC;
		return 6;
	}

	int ROL_Absolute()
	{
		uint8_t address = ResolveAbsolute();
		uint8_t num = read(address);
		ROL_General(num);
		write(address, num);
		++R.PC;
		return 6;
	}

	int ROL_AbsoluteX()
	{
		bool temp;
		uint8_t address = ResolveAbsoluteX(temp); // TODO: This doesn't actually need this variable, maybe provide an implementation that ignores new pages.
		uint8_t num = read(address);
		ROL_General(num);
		write(address, num);
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
		uint8_t num = read(address);
		ROR_General(num);
		write(address, num);
		++R.PC;
		return 5;
	}

	int ROR_ZPX()
	{
		uint8_t address = ResolveZPX();
		uint8_t num = read(address);
		ROR_General(num);
		write(address, num);
		++R.PC;
		return 6;
	}

	int ROR_Absolute()
	{
		uint8_t address = ResolveAbsolute();
		uint8_t num = read(address);
		ROR_General(num);
		write(address, num);
		++R.PC;
		return 6;
	}

	int ROR_AbsoluteX()
	{
		bool temp;
		uint8_t address = ResolveAbsoluteX(temp); // TODO: This doesn't actually need this variable, maybe provide an implementation that ignores new pages.
		uint8_t num = read(address);
		ROR_General(num);
		write(address, num);
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
		SBC_General(read(++R.PC));
		++R.PC;
		return 2;
	}

	int SBC_ZP()
	{
		SBC_General(read(ResolveZP()));
		++R.PC;
		return 3;
	}

	int SBC_ZPX()
	{
		SBC_General(read(ResolveZPX()));
		++R.PC;
		return 4;
	}

	int SBC_Absolute()
	{
		SBC_General(read(ResolveAbsolute()));
		++R.PC;
		return 4;
	}

	int SBC_AbsoluteX()
	{
		bool pageCrossed;
		SBC_General(read(ResolveAbsoluteX(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int SBC_AbsoluteY()
	{
		bool pageCrossed;
		SBC_General(read(ResolveAbsoluteY(pageCrossed)));
		++R.PC;
		return pageCrossed ? 5 : 4;
	}

	int SBC_IndirectX()
	{
		SBC_General(read(ResolveIndirectX()));
		++R.PC;
		return 6;
	}

	int SBC_IndirectY()
	{
		bool pageCrossed;
		SBC_General(read(ResolveIndirectY(pageCrossed)));
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
		write(ResolveZP(), R.A);
		++R.PC;
		return 3;
	}

	int STA_ZPX()
	{
		write(ResolveZPX(), R.A);
		++R.PC;
		return 4;
	}

	int STA_Absolute()
	{
		write(ResolveAbsolute(), R.A);
		++R.PC;
		return 4;
	}

	int STA_AbsoluteX()
	{
		bool pageCrossed;
		write(ResolveAbsoluteX(pageCrossed), R.A);
		++R.PC;
		return 5;
	}

	int STA_AbsoluteY()
	{
		bool pageCrossed;
		write(ResolveAbsoluteY(pageCrossed), R.A);
		++R.PC;
		return 5;
	}

	int STA_IndirectX()
	{
		write(ResolveIndirectX(), R.A);
		++R.PC;
		return 6;
	}

	int STA_IndirectY()
	{
		bool pageCrossed;
		write(ResolveIndirectY(pageCrossed), R.A);
		++R.PC;
		return 6;
	}

	int STX_ZP()
	{
		write(ResolveZP(), R.X);
		++R.PC;
		return 3;
	}

	int STX_ZPY()
	{
		write(ResolveZPY(), R.X);
		++R.PC;
		return 4;
	}

	int STX_Absolute()
	{
		write(ResolveAbsolute(), R.X);
		++R.PC;
		return 4;
	}

	int STY_ZP()
	{
		write(ResolveZP(), R.Y);
		++R.PC;
		return 3;
	}

	int STY_ZPX()
	{
		write(ResolveZPX(), R.Y);
		++R.PC;
		return 4;
	}

	int STY_Absolute()
	{
		write(ResolveAbsolute(), R.Y);
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

	void init()
	{
		// Initialize function pointer array to NOP for all instructions.
		for (int i = 0; i < 256; i++) instruction[i] = &NOP;

		instruction[0x69] = &ADC_Immediate;
		instruction[0x65] = &ADC_ZP;
		instruction[0x75] = &ADC_ZPX;
		instruction[0x6D] = &ADC_Absolute;
		instruction[0x7D] = &ADC_AbsoluteX;
		instruction[0x79] = &ADC_AbsoluteY;
		instruction[0x61] = &ADC_IndirectX;
		instruction[0x71] = &ADC_IndirectY;

		instruction[0x29] = &AND_Immediate;
		instruction[0x25] = &AND_ZP;
		instruction[0x35] = &AND_ZPX;
		instruction[0x2D] = &AND_Absolute;
		instruction[0x3D] = &AND_AbsoluteX;
		instruction[0x39] = &AND_AbsoluteY;
		instruction[0x21] = &AND_IndirectX;
		instruction[0x31] = &AND_IndirectY;

		instruction[0x0A] = &ASL_Accumulator;
		instruction[0x06] = &ASL_ZP;
		instruction[0x16] = &ASL_ZPX;
		instruction[0x0E] = &ASL_Absolute;
		instruction[0x1E] = &ASL_AbsoluteX;

		instruction[0x90] = &BCC;
		instruction[0xB0] = &BCS;
		instruction[0xF0] = &BEQ;
		instruction[0x30] = &BMI;
		instruction[0xD0] = &BNE;
		instruction[0x10] = &BPL;
		instruction[0x50] = &BVC;
		instruction[0x70] = &BVS;

		instruction[0x24] = &BIT_ZP;
		instruction[0x2C] = &BIT_Absolute;

		instruction[0x00] = &BRK;

		instruction[0x18] = &CLC;
		instruction[0xD8] = &CLD;
		instruction[0x58] = &CLI;
		instruction[0xB8] = &CLV;

		instruction[0xC9] = &CMP_Immediate;
		instruction[0xC5] = &CMP_ZP;
		instruction[0xD5] = &CMP_ZPX;
		instruction[0xCD] = &CMP_Absolute;
		instruction[0xDD] = &CMP_AbsoluteX;
		instruction[0xD9] = &CMP_AbsoluteY;
		instruction[0xC1] = &CMP_IndirectX;
		instruction[0xD1] = &CMP_IndirectY;

		instruction[0xE0] = &CPX_Immediate;
		instruction[0xE4] = &CPX_ZP;
		instruction[0xEC] = &CPX_Absolute;

		instruction[0xC0] = &CPY_Immediate;
		instruction[0xC4] = &CPY_ZP;
		instruction[0xCC] = &CPY_Absolute;

		instruction[0xC6] = &DEC_ZP;
		instruction[0xD6] = &DEC_ZPX;
		instruction[0xCE] = &DEC_Absolute;
		instruction[0xDE] = &DEC_AbsoluteX;

		instruction[0xCA] = &DEX;
		instruction[0x88] = &DEY;

		instruction[0x49] = &EOR_Immediate;
		instruction[0x45] = &EOR_ZP;
		instruction[0x55] = &EOR_ZPX;
		instruction[0x4D] = &EOR_Absolute;
		instruction[0x5D] = &EOR_AbsoluteX;
		instruction[0x59] = &EOR_AbsoluteY;
		instruction[0x41] = &EOR_IndirectX;
		instruction[0x51] = &EOR_IndirectY;

		instruction[0xE6] = &INC_ZP;
		instruction[0xF6] = &INC_ZPX;
		instruction[0xEE] = &INC_Absolute;
		instruction[0xFE] = &INC_AbsoluteX;

		instruction[0xE8] = &INX;
		instruction[0xC8] = &INY;

		instruction[0x4C] = &JMP_Absolute;
		instruction[0x6C] = &JMP_Indirect;

		instruction[0x20] = &JSR;

		instruction[0xA9] = &LDA_Immediate;
		instruction[0xA5] = &LDA_ZP;
		instruction[0xB5] = &LDA_ZPX;
		instruction[0xAD] = &LDA_Absolute;
		instruction[0xBD] = &LDA_AbsoluteX;
		instruction[0xB9] = &LDA_AbsoluteY;
		instruction[0xA1] = &LDA_IndirectX;
		instruction[0xB1] = &LDA_IndirectY;

		instruction[0xA2] = &LDX_Immediate;
		instruction[0xA6] = &LDX_ZP;
		instruction[0xB6] = &LDX_ZPY;
		instruction[0xAE] = &LDX_Absolute;
		instruction[0xBE] = &LDX_AbsoluteY;

		instruction[0xA0] = &LDY_Immediate;
		instruction[0xA4] = &LDY_ZP;
		instruction[0xB4] = &LDY_ZPX;
		instruction[0xAC] = &LDY_Absolute;
		instruction[0xBC] = &LDY_AbsoluteX;

		instruction[0x4A] = &LSR_Accumulator;
		instruction[0x46] = &LSR_ZP;
		instruction[0x56] = &LSR_ZPX;
		instruction[0x4E] = &LSR_Absolute;
		instruction[0x5E] = &LSR_AbsoluteX;

		instruction[0xEA] = &NOP;

		instruction[0x09] = &ORA_Immediate;
		instruction[0x05] = &ORA_ZP;
		instruction[0x15] = &ORA_ZPX;
		instruction[0x0D] = &ORA_Absolute;
		instruction[0x1D] = &ORA_AbsoluteX;
		instruction[0x19] = &ORA_AbsoluteY;
		instruction[0x01] = &ORA_IndirectX;
		instruction[0x11] = &ORA_IndirectY;

		instruction[0x48] = &PHA;
		instruction[0x08] = &PHP;
		instruction[0x68] = &PLA;
		instruction[0x28] = &PLP;

		instruction[0x2A] = &ROL_Accumulator;
		instruction[0x26] = &ROL_ZP;
		instruction[0x36] = &ROL_ZPX;
		instruction[0x2E] = &ROL_Absolute;
		instruction[0x3E] = &ROL_AbsoluteX;

		instruction[0x6A] = &ROR_Accumulator;
		instruction[0x66] = &ROR_ZP;
		instruction[0x76] = &ROR_ZPX;
		instruction[0x6E] = &ROR_Absolute;
		instruction[0x7E] = &ROR_AbsoluteX;

		instruction[0x40] = &RTI;
		instruction[0x60] = &RTS;

		instruction[0xE9] = &SBC_Immediate;
		instruction[0xE5] = &SBC_ZP;
		instruction[0xF5] = &SBC_ZPX;
		instruction[0xED] = &SBC_Absolute;
		instruction[0xFD] = &SBC_AbsoluteX;
		instruction[0xF9] = &SBC_AbsoluteY;
		instruction[0xE1] = &SBC_IndirectX;
		instruction[0xF1] = &SBC_IndirectY;

		instruction[0x38] = &SEC;
		instruction[0xF8] = &SED;
		instruction[0x78] = &SEI;

		instruction[0x85] = &STA_ZP;
		instruction[0x95] = &STA_ZPX;
		instruction[0x8D] = &STA_Absolute;
		instruction[0x9D] = &STA_AbsoluteX;
		instruction[0x99] = &STA_AbsoluteY;
		instruction[0x81] = &STA_IndirectX;
		instruction[0x91] = &STA_IndirectY;

		instruction[0x86] = &STX_ZP;
		instruction[0x96] = &STX_ZPY;
		instruction[0x8E] = &STX_Absolute;

		instruction[0x84] = &STY_ZP;
		instruction[0x94] = &STY_ZPX;
		instruction[0x8C] = &STY_Absolute;

		instruction[0xAA] = &TAX;
		instruction[0xA8] = &TAY;
		instruction[0xBA] = &TSX;
		instruction[0x8A] = &TXA;
		instruction[0x9A] = &TXS;
		instruction[0x98] = &TYA;

		// Initialize registers to 0, except PC which is initialized to value from reset vector.
		doRES();
	}
}