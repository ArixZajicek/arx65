#include "Common.h"
#include "gui/GraphicsWindow.h"
#include "sys/ISystem.h"
#include "sys/Terminal.h"

using namespace std;
using arx65::sys::ISystem;
using arx65::sys::Terminal;

namespace arx65::GUI
{
    bool initializeSdl();
    int loop();
    void deinitializeSdl();

    // Screen dimensions
    const int SCREEN_WIDTH = 1280;
    const int SCREEN_HEIGHT = 720;

    // Screen refresh rate
    const double FRAMERATE = 60;

    // Common SDL variables
    SDL_Window *window;
    SDL_Renderer *renderer;

    // Active system
    ISystem *system;

    /* Main entrypoint is here */
    int mainGui(int argc, char *args[])
    {
        // Initialize SDL
        if (!initializeSdl()) return -1;

        //TODO
        // Maybe here we can parse arguments and decide which system we want to set up.
        system = (ISystem *)new Terminal(80, 48);
        system->init();

        // Start main program loop
        loop();

        // De-initialize the active system? Probably won't actually do this here
        system->deinit();

        // Unload resources and close SDL
        deinitializeSdl();

        return 0;
    }

    /* Quickly load a texture into a standard map */
    SDL_Texture * loadTexture(const char *path)
    {
        // This will be the texture we use
        SDL_Texture *tempTex = nullptr;
        
        // Load image from path using image library
        SDL_Surface *tempSur = IMG_Load(path);

        if(tempSur == nullptr)
        {
            cout << "Failed to load image from path \"" << path << "\", SDL Error: " << SDL_GetError() << endl;
            return nullptr;
        }

        tempTex = SDL_CreateTextureFromSurface(renderer, tempSur);
        
        if (tempTex == nullptr)
        {
            cout << "Unable to load texture \"" << path << "\", SDL Error: " << SDL_GetError() << endl;
            return nullptr;
        }
        
        SDL_FreeSurface(tempSur);
        
        return tempTex;
    }
    
    /* Initialize the SDL window */
    bool initializeSdl()
    {
        window = nullptr;
        renderer = nullptr;
    
        int res = SDL_Init(SDL_INIT_VIDEO);
        
        if (res < 0)
        {
            cout << "SDL could not initialize. SDL Error: '" << SDL_GetError() << "' (" << res << ")" << endl;
            return false; 
        }

        window = SDL_CreateWindow("arx65", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

        if (window == nullptr)
        {
            cout << "The window could not be created. SDL Error: " << SDL_GetError() << endl;
            return false;
        }
        
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

        if (renderer == nullptr)
        {
            cout << "The renderer could not be created. SDL Error: " << SDL_GetError() << endl;
            return false;
        }

        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

        int imageFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imageFlags) & imageFlags))
        {
            cout << "SDL Image could not be initialized. SDL Error: " << IMG_GetError() << endl;
            return false;
        }

        return true;
    }

    /* The main program loop runs here. */
    int loop()
    {
        bool doLoop = true;
        SDL_Event e;
        
        auto nextDraw = chrono::high_resolution_clock::now();
        long first = nextDraw.time_since_epoch().count();

        while (doLoop)
        {
            // First, handle button events
            while(SDL_PollEvent(&e) != 0)
            {
                if (e.type == SDL_QUIT)
                    doLoop = false;
                else if( e.type == SDL_KEYDOWN )
                    system->keyPressEvent(e.key.keysym);
                else if(e.type == SDL_KEYUP)
                    system->keyReleaseEvent(e.key.keysym);
                else if(e.type == SDL_TEXTINPUT)
                    system->keyTypeEvent(e.text.text);
            }
            
            // Now, perform regular update
            system->tick(0);

            // Now, update the screen by passing the current drawing surface.
            // Note that even though most of the time, it doesn't change, it's possible for the 
            // surface to change without predictability, such as window size change, etc.
            auto now = chrono::high_resolution_clock::now();
            if (now > nextDraw)
            {
                //cout << "New frame. " << (now.time_since_epoch().count() - first) / 1000 << endl;
                
                SDL_RenderClear(renderer);

                system->drawGraphics(renderer, 0);
                
                SDL_RenderPresent(renderer);
                
                // Update when we should next draw.
                while (nextDraw < now) nextDraw += chrono::nanoseconds((int)(1000000000 / FRAMERATE));
            }
            
        }

        return 0;
    }

    /* Deinitialize the window and any leftover textures */
    void deinitializeSdl()
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);

        renderer = nullptr;
        window = nullptr;

        IMG_Quit();
        SDL_Quit();
    }
}