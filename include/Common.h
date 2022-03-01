#include <iomanip>
#include <bitset>
#include <cstdint>
#include <vector>
#include <iostream>
#include <chrono>
#include <ctime>
#include <cmath>
#include <string>
#include <cstring>
#include <fstream>
#include <map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define HEX(x, prin) std::setfill('0') << std::setw(x) << std::right << std::uppercase << std::hex << (int)prin << std::nouppercase << std::dec