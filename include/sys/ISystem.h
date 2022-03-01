#include "Common.h"

#pragma once

namespace arx65::sys
{
    /* Holds a specific system configuration */
    class ISystem
    {
    public:
        /* Called when ready to begin. If using a 6502 CPU, this should launch it on another thread? Maybe not. */
        virtual void init();
        virtual void deinit();

        /* Called very frequently. Maybe can be used for processor control. */
        virtual void tick(double delta);

        /* Called when the main loop is ready for a graphics redraw (~60FPS). */
        virtual void drawGraphics(SDL_Renderer *renderer, double delta);
        
        /* Event handlers. These are called before tick and drawGraphics in the main loop. */
        virtual void keyPressEvent(SDL_Keysym k);
        virtual void keyReleaseEvent(SDL_Keysym k);
        virtual void keyTypeEvent(char *text);
    };
}