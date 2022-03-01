#include "Common.h"
#include "sys/ISystem.h"
#include "mod/ACIA6551.h"
#include "mod/SimpleMemory.h"

#pragma once

namespace arx65::sys
{
    class Terminal : public ISystem
    {
    private:
        /* Holds the size of the screen in characters */
        int screen_width, screen_height;
        int cursor_row, cursor_column;
        std::vector<std::string> text_buffer;
        SDL_Texture *font;
        string nextEntry;

        arx65::mod::SimpleMemory *progRAM, *quickROM;
        arx65::mod::ACIA6551 *acia;

        bool freerun;

        void addToScreenBuffer(char c);
        void backspaceScreenBuffer();
    public:
        Terminal(int screenWidth, int screenHeight);
        ~Terminal();

        /* Called when ready to begin. If using a 6502 CPU, this should launch it on another thread? Maybe not. */
        void init();

        /* Called very frequently. Maybe can be used for processor control. */
        void tick(double delta);

        /* Called when the main loop is ready for a graphics redraw (~60FPS). */
        void drawGraphics(SDL_Renderer *renderer, double delta);
        
        /* Event handlers. These are called before tick and drawGraphics in the main loop. */
        void keyPressEvent(SDL_Keysym k);
        void keyReleaseEvent(SDL_Keysym k);
        virtual void keyTypeEvent(char *text);
    };
}