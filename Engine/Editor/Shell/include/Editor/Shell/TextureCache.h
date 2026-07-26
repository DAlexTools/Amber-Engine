#ifndef AMBER_EDITOR_SHELL_TEXTURE_CACHE_H
#define AMBER_EDITOR_SHELL_TEXTURE_CACHE_H

#include "Editor/Shell/AssetRegistry.h"

#include <SDL2/SDL.h>

#include <map>
#include <string>

namespace AE::Editor
{

struct TexturePreview
{
    SDL_Texture* texture = nullptr;
    int width = 0;
    int height = 0;
    bool failed = false;
};

class TextureCache
{
public:
    ~TextureCache();

    void Initialize(SDL_Renderer* renderer);
    void Clear();
    TexturePreview* GetTexture(const AssetRecord& asset);

private:
    SDL_Renderer* renderer = nullptr;
    std::map<std::string, TexturePreview> textures;
};

}

#endif
