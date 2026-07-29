#ifndef BODY_TEXTURE_STORE_H
#define BODY_TEXTURE_STORE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <unordered_map>

#include "../Renderer/SDL/Graphics.h"
#include "SamplePaths.h"
#include "Body.h"

class BodyTextureStore
{
public:
    ~BodyTextureStore()
    {
        Clear();
    }

    void Set(const Body* body, const char* textureFileName)
    {
        if (!body)
        {
            return;
        }

        const std::string resolvedTextureFileName = ResolvePhysicsDemoAssetPath(textureFileName);
        SDL_Surface* surface = IMG_Load(resolvedTextureFileName.c_str());
        if (!surface)
        {
            return;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(Graphics::Renderer, surface);
        SDL_FreeSurface(surface);

        if (!texture)
        {
            return;
        }

        auto existingTexture = textures.find(body);
        if (existingTexture != textures.end())
        {
            SDL_DestroyTexture(existingTexture->second);
        }

        textures[body] = texture;
    }

    SDL_Texture* Get(const Body* body) const
    {
        auto texture = textures.find(body);
        return texture == textures.end() ? nullptr : texture->second;
    }

    void Remove(const Body* body)
    {
        auto texture = textures.find(body);
        if (texture == textures.end())
        {
            return;
        }

        SDL_DestroyTexture(texture->second);
        textures.erase(texture);
    }

    void Clear()
    {
        for (auto& texture : textures)
        {
            SDL_DestroyTexture(texture.second);
        }
        textures.clear();
    }

private:
    std::unordered_map<const Body*, SDL_Texture*> textures;
};

#endif
