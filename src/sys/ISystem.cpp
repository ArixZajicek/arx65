#include "sys/ISystem.h"

namespace arx65::sys
{
    void ISystem::init(){}
    void ISystem::deinit(){}
    void ISystem::tick(double delta){}
    void ISystem::drawGraphics(SDL_Renderer *renderer, double delta){}
    void ISystem::keyPressEvent(SDL_Keysym k){}
    void ISystem::keyReleaseEvent(SDL_Keysym k){}
    void ISystem::keyTypeEvent(char *text){}
}