#ifndef AMBER_RUNTIME_GAME_RUNTIME_TEXTURE_CACHE_SDL_H
#define AMBER_RUNTIME_GAME_RUNTIME_TEXTURE_CACHE_SDL_H

#include "Assets/AssetResolver.h"

#include <SDL2/SDL.h>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace AE
{

struct RuntimeTextureSDL
{
    SDL_Texture* texture = nullptr;
    int width = 0;
    int height = 0;
    bool failed = false;
    std::filesystem::path resolvedPath;
};

class RuntimeTextureCacheSDL
{
public:
    explicit RuntimeTextureCacheSDL(SDL_Renderer* renderer = nullptr);
    ~RuntimeTextureCacheSDL();

    RuntimeTextureCacheSDL(const RuntimeTextureCacheSDL&) = delete;
    RuntimeTextureCacheSDL& operator=(const RuntimeTextureCacheSDL&) = delete;

    void SetRenderer(SDL_Renderer* value);
    void SetResolverConfig(RuntimeAssetResolverConfig config);
    void SetAssetRoots(std::vector<RuntimeAssetRoot> roots);
    void Clear();

    RuntimeTextureSDL* GetTexture(const std::string& assetId);
    RuntimeTextureSDL* GetTextureFromPath(const std::string& cacheId, const std::filesystem::path& path);

private:
    RuntimeTextureSDL* LoadTexture(const std::string& key, const std::filesystem::path& path);

    SDL_Renderer* renderer = nullptr;
    RuntimeAssetResolverConfig resolverConfig;
    std::unordered_map<std::string, RuntimeTextureSDL> textures;
};

}

#endif
