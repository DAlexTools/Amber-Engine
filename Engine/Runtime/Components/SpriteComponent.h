#ifndef SPRITECOMPONENT_H
#define SPRITECOMPONENT_H

#include <string>
#include <SDL2/SDL.h>

struct SpriteComponent
{
    int width;
    int height;
    std::string assetID;
    int zIndex;
    SDL_RendererFlip flip;
    bool isFixed;
    
    SDL_Rect srcRect;

    SpriteComponent(std::string assetID = "", int width = 0, int height = 0,int zIndex = 0,bool isFixed = false, int srcRectX = 0, int srcRectY = 0)
    {
        this->assetID = assetID;
        this->width = width;
        this->height = height;
        this->zIndex = zIndex;
        this->flip = SDL_FLIP_NONE;
        this->isFixed = isFixed;
        this->srcRect = {srcRectX, srcRectY, width, height};
    }
};

#endif