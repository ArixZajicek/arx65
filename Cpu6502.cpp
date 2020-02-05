#include "Cpu6502.h"

Cpu6502::Cpu6502(Databus *bus)
{
	// Set the databus
	Bus = bus;

	// Initialize function pointer array to NOP for all instructions.
	for (int i = 0; i < 256; i++) instruction[i] = &Cpu6502::NOP;

	instruction[0x69] = &Cpu6502::ADC_Immediate;
	instruction[0x65] = &Cpu6502::ADC_ZP;
	instruction[0x75] = &Cpu6502::ADC_ZPX;
	instruction[0x6D] = &Cpu6502::ADC_Absolute;
	instruction[0x7D] = &Cpu6502::ADC_AbsoluteX;
	instruction[0x79] = &Cpu6502::ADC_AbsoluteY;
	instruction[0x61] = &Cpu6502::ADC_IndirectX;
	instruction[0x71] = &Cpu6502::ADC_IndirectY;

	instruction[0x29] = &Cpu6502::AND_Immediate;
	instruction[0x25] = &Cpu6502::AND_ZP;
	instruction[0x35] = &Cpu6502::AND_ZPX;
	instruction[0x2D] = &Cpu6502::AND_Absolute;
	instruction[0x3D] = &Cpu6502::AND_AbsoluteX;
	instruction[0x39] = &Cpu6502::AND_AbsoluteY;
	instruction[0x21] = &Cpu6502::AND_IndirectX;
	instruction[0x31] = &Cpu6502::AND_IndirectY;

	instruction[0x0A] = &Cpu6502::ASL_Accumulator;
	instruction[0x06] = &Cpu6502::ASL_ZP;
	instruction[0x16] = &Cpu6502::ASL_ZPX;
	instruction[0x0E] = &Cpu6502::ASL_Absolute;
	instruction[0x1E] = &Cpu6502::ASL_AbsoluteX;

	instruction[0x90] = &Cpu6502::BCC;
	instruction[0xB0] = &Cpu6502::BCS;
	instruction[0xF0] = &Cpu6502::BEQ;
	instruction[0x30] = &Cpu6502::BMI;
	instruction[0xD0] = &Cpu6502::BNE;
	instruction[0x10] = &Cpu6502::BPL;
	instruction[0x50] = &Cpu6502::BVC;
	instruction[0x70] = &Cpu6502::BVS;

	instruction[0x24] = &Cpu6502::BIT_ZP;
	instruction[0x2C] = &Cpu6502::BIT_Absolute;

	instruction[0x00] = &Cpu6502::BRK;

	instruction[0x18] = &Cpu6502::CLC;
	instruction[0xD8] = &Cpu6502::CLD;
	instruction[0x58] = &Cpu6502::CLI;
	instruction[0xB8] = &Cpu6502::CLV;

	instruction[0xC9] = &Cpu6502::CMP_Immediate;
	instruction[0xC5] = &Cpu6502::CMP_ZP;
	instruction[0xD5] = &Cpu6502::CMP_ZPX;
	instruction[0xCD] = &Cpu6502::CMP_Absolute;
	instruction[0xDD] = &Cpu6502::CMP_AbsoluteX;
	instruction[0xD9] = &Cpu6502::CMP_AbsoluteY;
	instruction[0xC1] = &Cpu6502::CMP_IndirectX;
	instruction[0xD1] = &Cpu6502::CMP_IndirectY;

	instruction[0xE0] = &Cpu6502::CPX_Immediate;
	instruction[0xE4] = &Cpu6502::CPX_ZP;
	instruction[0xEC] = &Cpu6502::CPX_Absolute;

	instruction[0xC0] = &Cpu6502::CPY_Immediate;
	instruction[0xC4] = &Cpu6502::CPY_ZP;
	instruction[0xCC] = &Cpu6502::CPY_Absolute;

	instruction[0xC6] = &Cpu6502::DEC_ZP;
	instruction[0xD6] = &Cpu6502::DEC_ZPX;
	instruction[0xCE] = &Cpu6502::DEC_Absolute;
	instruction[0xDE] = &Cpu6502::DEC_AbsoluteX;

	instruction[0xCA] = &Cpu6502::DEX;
	instruction[0x88] = &Cpu6502::DEY;

	instruction[0x49] = &Cpu6502::EOR_Immediate;
	instruction[0x45] = &Cpu6502::EOR_ZP;
	instruction[0x55] = &Cpu6502::EOR_ZPX;
	instruction[0x4D] = &Cpu6502::EOR_Absolute;
	instruction[0x5D] = &Cpu6502::EOR_AbsoluteX;
	instruction[0x59] = &Cpu6502::EOR_AbsoluteY;
	instruction[0x41] = &Cpu6502::EOR_IndirectX;
	instruction[0x51] = &Cpu6502::EOR_IndirectY;

	instruction[0xE6] = &Cpu6502::INC_ZP;
	instruction[0xF6] = &Cpu6502::INC_ZPX;
	instruction[0xEE] = &Cpu6502::INC_Absolute;
	instruction[0xFE] = &Cpu6502::INC_AbsoluteX;

	instruction[0xE8] = &Cpu6502::INX;
	instruction[0xC8] = &Cpu6502::INY;

	instruction[0x4C] = &Cpu6502::JMP_Absolute;
	instruction[0x6C] = &Cpu6502::JMP_Indirect;

	instruction[0x20] = &Cpu6502::JSR;

	instruction[0xA9] = &Cpu6502::LDA_Immediate;
	instruction[0xA5] = &Cpu6502::LDA_ZP;
	instruction[0xB5] = &Cpu6502::LDA_ZPX;
	instruction[0xAD] = &Cpu6502::LDA_Absolute;
	instruction[0xBD] = &Cpu6502::LDA_AbsoluteX;
	instruction[0xB9] = &Cpu6502::LDA_AbsoluteY;
	instruction[0xA1] = &Cpu6502::LDA_IndirectX;
	instruction[0xB1] = &Cpu6502::LDA_IndirectY;

	instruction[0xA2] = &Cpu6502::LDX_Immediate;
	instruction[0xA6] = &Cpu6502::LDX_ZP;
	instruction[0xB6] = &Cpu6502::LDX_ZPY;
	instruction[0xAE] = &Cpu6502::LDX_Absolute;
	instruction[0xBE] = &Cpu6502::LDX_AbsoluteY;

	instruction[0xA0] = &Cpu6502::LDY_Immediate;
	instruction[0xA4] = &Cpu6502::LDY_ZP;
	instruction[0xB4] = &Cpu6502::LDY_ZPX;
	instruction[0xAC] = &Cpu6502::LDY_Absolute;
	instruction[0xBC] = &Cpu6502::LDY_AbsoluteX;

	instruction[0x4A] = &Cpu6502::LSR_Accumulator;
	instruction[0x46] = &Cpu6502::LSR_ZP;
	instruction[0x56] = &Cpu6502::LSR_ZPX;
	instruction[0x4E] = &Cpu6502::LSR_Absolute;
	instruction[0x5E] = &Cpu6502::LSR_AbsoluteX;

	instruction[0xEA] = &Cpu6502::NOP;

	instruction[0x09] = &Cpu6502::ORA_Immediate;
	instruction[0x05] = &Cpu6502::ORA_ZP;
	instruction[0x15] = &Cpu6502::ORA_ZPX;
	instruction[0x0D] = &Cpu6502::ORA_Absolute;
	instruction[0x1D] = &Cpu6502::ORA_AbsoluteX;
	instruction[0x19] = &Cpu6502::ORA_AbsoluteY;
	instruction[0x01] = &Cpu6502::ORA_IndirectX;
	instruction[0x11] = &Cpu6502::ORA_IndirectY;

	instruction[0x48] = &Cpu6502::PHA;
	instruction[0x08] = &Cpu6502::PHP;
	instruction[0x68] = &Cpu6502::PLA;
	instruction[0x28] = &Cpu6502::PLP;

	instruction[0x2A] = &Cpu6502::ROL_Accumulator;
	instruction[0x26] = &Cpu6502::ROL_ZP;
	instruction[0x36] = &Cpu6502::ROL_ZPX;
	instruction[0x2E] = &Cpu6502::ROL_Absolute;
	instruction[0x3E] = &Cpu6502::ROL_AbsoluteX;

	instruction[0x6A] = &Cpu6502::ROR_Accumulator;
	instruction[0x66] = &Cpu6502::ROR_ZP;
	instruction[0x76] = &Cpu6502::ROR_ZPX;
	instruction[0x6E] = &Cpu6502::ROR_Absolute;
	instruction[0x7E] = &Cpu6502::ROR_AbsoluteX;

	instruction[0x40] = &Cpu6502::RTI;
	instruction[0x60] = &Cpu6502::RTS;

	instruction[0xE9] = &Cpu6502::SBC_Immediate;
	instruction[0xE5] = &Cpu6502::SBC_ZP;
	instruction[0xF5] = &Cpu6502::SBC_ZPX;
	instruction[0xED] = &Cpu6502::SBC_Absolute;
	instruction[0xFD] = &Cpu6502::SBC_AbsoluteX;
	instruction[0xF9] = &Cpu6502::SBC_AbsoluteY;
	instruction[0xE1] = &Cpu6502::SBC_IndirectX;
	instruction[0xF1] = &Cpu6502::SBC_IndirectY;

	instruction[0x38] = &Cpu6502::SEC;
	instruction[0xF8] = &Cpu6502::SED;
	instruction[0x78] = &Cpu6502::SEI;

	instruction[0x85] = &Cpu6502::STA_ZP;
	instruction[0x95] = &Cpu6502::STA_ZPX;
	instruction[0x8D] = &Cpu6502::STA_Absolute;
	instruction[0x9D] = &Cpu6502::STA_AbsoluteX;
	instruction[0x99] = &Cpu6502::STA_AbsoluteY;
	instruction[0x81] = &Cpu6502::STA_IndirectX;
	instruction[0x91] = &Cpu6502::STA_IndirectY;

	instruction[0x86] = &Cpu6502::STX_ZP;
	instruction[0x96] = &Cpu6502::STX_ZPY;
	instruction[0x8E] = &Cpu6502::STX_Absolute;

	instruction[0x84] = &Cpu6502::STY_ZP;
	instruction[0x94] = &Cpu6502::STY_ZPX;
	instruction[0x8C] = &Cpu6502::STY_Absolute;

	instruction[0xAA] = &Cpu6502::TAX;
	instruction[0xA8] = &Cpu6502::TAY;
	instruction[0xBA] = &Cpu6502::TSX;
	instruction[0x8A] = &Cpu6502::TXA;
	instruction[0x9A] = &Cpu6502::TXS;
	instruction[0x98] = &Cpu6502::TYA;

	// Initialize registers to 0, except PC which is initialized to value from reset vector.
	doRES();
}

int Cpu6502::doNextInstruction()
{
	return (*this.*instruction[Bus->read(R.PC)])();
}

void Cpu6502::doNMI() {
	PushStackGeneral((R.PC >> 8) & 0x00FF);
	PushStackGeneral((R.PC) & 0x00FF);
	PushStackGeneral(R.Flags);
	R.PC = ((uint16_t)Bus->read(0xFFFA)) | (((uint16_t)Bus->read(0xFFFB)) << 8);
}

// Reset registers to initial values
void Cpu6502::doRES()
{
	R.A = 0;
	R.X = 0;
	R.Y = 0;
	R.SP = 0xFF;
	R.Flags = FLAG_INTERRUPT;
	R.PC = Bus->read(0xFFFC) | (Bus->read(0xFFFD) << 8);
}

void Cpu6502::doIRQ() {
	// Push current PC onto stack, high then low byte. Then push flags
	if (!(R.Flags & FLAG_INTERRUPT))
	{
		PushStackGeneral((R.PC >> 8) & 0x00FF);
		PushStackGeneral((R.PC) & 0x00FF);
		PushStackGeneral(R.Flags);
		R.PC = ((uint16_t)Bus->read(0xFFFE)) | (((uint16_t)Bus->read(0xFFFF)) << 8);
	}
}
