#ifndef AMBER_EDITOR_SHELL_TEXTURE_CACHE_H
#define AMBER_EDITOR_SHELL_TEXTURE_CACHE_H

#include "AssetRegistry.h"
#include "Game/RuntimeTextureCacheSDL.h"

#include <SDL2/SDL.h>

#include <map>
#include <vector>

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
    void SetAssetRoots(std::vector<AssetRoot> roots);
    TexturePreview* GetTexture(const AssetRecord& asset);

private:
    RuntimeTextureCacheSDL runtimeTextureCache;
    std::map<std::string, TexturePreview> textures;
};

}

#endif
