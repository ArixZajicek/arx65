#include "sys/Terminal.h"
#include "mod/SimpleMemory.h"
#include "mod/ACIA6551.h"
#include "Databus.h"
#include "Processor.h"
#include "gui/GraphicsWindow.h"

using namespace arx65::mod;
using namespace arx65::GUI;
using namespace std;

namespace arx65::sys
{
    /** This is specifically a debug view terminal. */
    Terminal::Terminal(int sWidth, int sHeight)
    {
        screen_width = sWidth;
        screen_height = sHeight;
        cursor_row = 0;
        cursor_column = 0;

        freerun = false;

        text_buffer.push_back("");
        nextEntry = "";

        // This loads the entire 64K of memory as RAM
        progRAM = new SimpleMemory(0x0000, 0xFFFF, 0x00, false);

        //const uint16_t PROG_START = 0x400;
        //progRAM->loadFromFile("../roms/test.o65", 0x00);

        // This loads chess!
        const uint16_t PROG_START = 0x1000;
        progRAM->loadFromFile("../roms/chess.o65", 0x1000);
        uint8_t vects[] = {PROG_START & 0x00FF, (PROG_START >> 8), PROG_START & 0x00FF, (PROG_START >> 8), PROG_START & 0x00FF, (PROG_START >> 8)};
        progRAM->copyFromMemory(vects, 0xFFFA, 6);

        // Input/Output chip (we use this to get screen info)
        acia = new ACIA6551(0x7F70);

        bus::attach(acia);
        bus::attach(progRAM);

        cpu::init();

        cpu::getRegisters()->PC = PROG_START;
    }

    Terminal::~Terminal()
    {
        bus::clear();
        delete progRAM;
        delete acia;
    }

    void Terminal::init()
    {
        font = loadTexture("../res/img/font.png");
        if (font == nullptr)
        {
            std::cerr << "Font failed, oh no!\r\n";
        }
    }

    /* Use this for processor control only */
    void Terminal::tick(double delta)
    {
        cpu::doNextInstruction();
    }

    void Terminal::drawGraphics(SDL_Renderer *r, double delta)
    {
        while(acia->bytesAvailable())
        {
            addToScreenBuffer(acia->nextByte());
        }

        SDL_Rect screen;
        SDL_RenderGetViewport(r, &screen);
        
        SDL_Rect source = {0, 0, 16, 16};
        SDL_Rect cursor = {0, 0, screen.w / screen_width, screen.h / screen_height};

        for (int row = 0; row < screen_height; row++)
        {
            string line = "";
            if (row < text_buffer.size()) line = text_buffer[row];

            for (int col = 0; col < screen_width; col++)
            {
                if (col < line.length())
                {
                    source.x = 16 * (line[col] % 16);
                    source.y = 16 * (line[col] / 16);
                }
                else
                {
                    source.x = 0;
                    source.y = 0;
                }
                
                SDL_RenderCopy(r, font, &source, &cursor);
                
                cursor.x += screen.w / screen_width;
            }
            cursor.x = 0;
            cursor.y += screen.h / screen_height;
        }
        
        if (chrono::high_resolution_clock::now().time_since_epoch().count() % 1000000000 > 500000000)
        {
            source.x = 16 * ('_' % 16);
            source.y = 16 * ('_' / 16);

            cursor.y = (text_buffer.size() - 1) * screen.h / screen_height;
            cursor.x = (text_buffer.at(text_buffer.size() - 1).length()) * screen.w / screen_width;
            SDL_RenderCopy(r, font, &source, &cursor);
        }

    }
    
    /* Event handlers. These are called before tick and drawGraphics in the main loop. */
    void Terminal::keyPressEvent(SDL_Keysym k)
    {

        //if (k.sym == SDLK_BACKSPACE) backspaceScreenBuffer();
        if (k.sym == SDLK_RETURN)
        {
            acia->sendByte('\n');
            /*
            nextEntry.push_back('\n');

            // Commit to serial interface
            acia->sendBytes(nextEntry.c_str, nextEntry.length());
            
            nextEntry = "";
            addToScreenBuffer('\n'); */
        }
        else if (k.sym == SDLK_RSHIFT)
        {
            //for (int i = 0; i < 10; i++) cpu::doNextInstructionDebug();
        }
        else if (k.sym == SDLK_RCTRL)
        {
            freerun = true;
        }
    }

    void Terminal::keyReleaseEvent(SDL_Keysym k)
    {
        if (k.sym == SDLK_RCTRL)
        {
            freerun = false;
        }
    }

    void Terminal::keyTypeEvent(char *text)
    {

        //nextEntry.push_back(text[0]);
        acia->sendByte(text[0]);
        //addToScreenBuffer(text[0]);
    }

    void Terminal::addToScreenBuffer(char c)
    {
        if (c < 32 && c != '\n') return;
        
        // Get row and column of new text location
        int row = text_buffer.size() - 1;
        int col = text_buffer.at(row).length();

        // If column is past the screen, reset to zero, add one to row
        if (col >= screen_width || c == '\n')
        {
            text_buffer.push_back("");
            ++row;
            col = 0;
        }

        if (c >= 32)
        {
            text_buffer.at(row).push_back(c);
        }

        // If there are too many rows, delete the first one
        while (text_buffer.size() > screen_height) text_buffer.erase(text_buffer.begin());
    }

    void Terminal::backspaceScreenBuffer()
    {
        if (nextEntry.length() > 0)
        {
            nextEntry.pop_back();

            if (text_buffer.at(text_buffer.size() - 1).length() > 0)
            {
                text_buffer.at(text_buffer.size() - 1).pop_back();
            }
            else
            {
                text_buffer.pop_back();
                text_buffer.at(text_buffer.size() - 1).pop_back();
            }
            
        }
    }
}