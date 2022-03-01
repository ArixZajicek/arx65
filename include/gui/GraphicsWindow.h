#include "Common.h"

#pragma once

namespace arx65::GUI
{
    // Start the main loop
    int mainGui(int argc, char *args[]);

    // Load an image into the texture buffer thingy
    SDL_Texture * loadTexture(const char *path);
}