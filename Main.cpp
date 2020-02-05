#include <iostream>
#include <chrono>
#include <ctime>
#include <cmath>
#include <iomanip>
#include <bitset>
#include <string>
#include <cstring>
#include <fstream>
#include "base/SimpleMemory.h"
#include "Databus.h"
#include "Cpu6502.h"

using namespace std;

void launchCpu(Cpu6502 proc);

string OPT_FILENAME;
bool OPT_DEBUG;
bool OPT_BENCHMARK;
bool OPT_VERBOSE;
int OPT_ITERATIONS;

int main(int argc, char* args[])
{
	OPT_FILENAME = "";
	OPT_DEBUG = false;
	OPT_BENCHMARK = false;
	OPT_VERBOSE = false;
	OPT_ITERATIONS = 1;

	for (int i = 1; i < argc; i++)
	{
		// Verify each option is of '-X' format.
		if (strlen(args[i]) != 2 || args[i][0] != '-')
		{
			cout << "Invalid command line option '" << args[i] << "'. Use -h for help." << endl;
			return -1;
		}
		
		// parse the command
		switch(args[i][1])
		{
		case 'r':
			// ROM Input
			if ( i + 1 < argc)
			{
				OPT_FILENAME = args[++i];
			}
			else
			{
				cout << "Missing file after '-f'" << endl;
				return -1;
			}
			break;
		case 'd':
			OPT_DEBUG = true;
			break;
		case 'b':
			OPT_BENCHMARK = true;
			break;
		case 'v':
			OPT_VERBOSE = true;
			break;
		case 'i':
			try {
				int iter = stoi(args[++i]);
				if (iter < 1)
				{
					cout << "Iteration count must be at least 1." << endl;
					return -1;
				}
				OPT_ITERATIONS = iter;
			}
			catch (...)
			{
				cout << "There was an error parsing iteration count. Excpected an integer, got '" << args[i] << "'." << endl;
				return -1;
			}
			break;
		case 'h':
			cout << "See usage.txt for usage information." << endl;
			break;
		default:
			cout << "Unknown command line option '" << args[i] << "'. Use -h for help." << endl;
			return -1;
		}
	}

	if (OPT_FILENAME == "")
	{
		cout << "Must include an input ROM file. Use -r [file] to specify." << endl;
		return -1;
	}

	// Check if file exists
	ifstream romFile;
	romFile.open(OPT_FILENAME, ios::in | ios::binary);

	if (romFile.fail() || !romFile.is_open())
	{
		cout << "Error opening '" << OPT_FILENAME << "'." << endl;
		return -1;
	}

	// Get File Size and halt if size is incorrect
	romFile.seekg(0, ios::end);
	int romLength = romFile.tellg();
	romFile.seekg(0, ios::beg);

	if (romLength > 0xFFFF - 0x200)
	{
		cout << "ROM file too large. ROM size: " << romLength << " bytes, Maximum size: " << 0xFFFF-0x200 << " bytes." << endl; 
		return -1;
	}

	if (romLength < 4)
	{
		cout << "ROM file too small. Must be at least 4 bytes to include RES vector. File is only " << romLength << " bytes." << endl;
		return -1;
	}

	if (OPT_VERBOSE) cout << "ROM file found, " << romLength << " bytes long." << endl;
	
	// All is good, read in ROM data
	uint8_t rom_data[romLength];
	romFile.read((char *)rom_data, romLength);
	romFile.close();
	SimpleMemory programROM(0x10000 - romLength, 0xFFFF, rom_data, true);

	if (OPT_VERBOSE) cout << "Finished loading ROM at 0x" << HEX(4, 0x10000 - romLength) << "." << endl;
	
	// Now prepare our RAM
	SimpleMemory mainRAM(0x0000, 0xFFFF - romLength, (uint8_t)0x00, false);

	if (OPT_VERBOSE) cout << "Created RAM from 0x0000 to 0x" << HEX(4, 0xFFFF - romLength) << "." << endl;

	// Attach devices to the databus
	Databus bus;
	bus.attach(&mainRAM);
	bus.attach(&programROM);

	if (OPT_VERBOSE) cout << "Bus created, ROM and RAM attached." << endl;

	// Create and launch the CPU
	Cpu6502 proc(&bus);

	if (OPT_VERBOSE) cout << "CPU attached to bus." << endl;
	
	launchCpu(proc);
		
	return 0;
}

void launchCpu(Cpu6502 proc) 
{
	uint8_t nextInstruction = 0xFF;
	long totalCycles = 0;

	if (OPT_VERBOSE | OPT_DEBUG | OPT_BENCHMARK) cout << "    ---- Program Starting ----" << endl;
	
	if (OPT_DEBUG) cout << "  PC   (INS ) |  A   |  X   |  Y   |  SP  | NV BDIZC | Cycles" << endl;

	// Begin timer.
	auto bench_start = chrono::high_resolution_clock::now();

	for (int iter = 0; iter < OPT_ITERATIONS; iter++)
	{
		// Debug Loop
		if (OPT_DEBUG) while (nextInstruction != 0x00)
		{
			RegisterSet R = proc.R;
			cout << "0x" << HEX(4, R.PC) << " (0x" << HEX(2, proc.Bus->read(R.PC)) << ") | ";
			cout << "0x" << HEX(2, R.A) << " | ";
			cout << "0x" << HEX(2, R.X) << " | ";
			cout << "0x" << HEX(2, R.Y) << " | ";
			cout << "0x" << HEX(2, R.SP) << " | ";
			cout << bitset<8>(R.Flags) << " | ";
			int t = proc.doNextInstruction();
			cout << "[" << t << "]" << endl;
			totalCycles += t;
			nextInstruction = proc.Bus->read(proc.R.PC);
		}
		// Regular loop.
		else while (!(proc.R.Flags & Cpu6502::FLAG_BRK)) totalCycles += proc.doNextInstruction();

		proc.doRES();
	}
	
	// End timer.
	auto bench_end = chrono::high_resolution_clock::now();

	cout << endl << endl;
	if (OPT_ITERATIONS == 1) cout << "    ---- Program Completed in " << totalCycles << " cycles ----" << endl;
	else cout << "    ---- " << OPT_ITERATIONS << " iterations of program completed in " << totalCycles << " cycles ----" << endl;
	if (OPT_BENCHMARK)
	{
		chrono::duration<double> elapsed = bench_end - bench_start;
		double seconds = elapsed.count();
		cout << "Program took " << seconds << " seconds to complete." << endl;
		cout << "Equivalent Clock frequency: " << totalCycles / seconds / 1000000 << " MHz" << endl;
	}
}
