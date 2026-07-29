#include "Game/RuntimeTextureCacheSDL.h"

#include "Logging/Logger.h"

#include <SDL2/SDL_image.h>

#include <utility>

namespace AE
{
namespace
{
    bool SameRoots(const std::vector<RuntimeAssetRoot>& left, const std::vector<RuntimeAssetRoot>& right)
    {
        if (left.size() != right.size())
        {
            return false;
        }

        for (std::size_t index = 0; index < left.size(); ++index)
        {
            if (left[index].name != right[index].name || left[index].path != right[index].path)
            {
                return false;
            }
        }

        return true;
    }

    bool SameConfig(const RuntimeAssetResolverConfig& left, const RuntimeAssetResolverConfig& right)
    {
        return left.projectRoot == right.projectRoot &&
            left.engineRoot == right.engineRoot &&
            left.contentRoot == right.contentRoot &&
            SameRoots(left.roots, right.roots);
    }
}

RuntimeTextureCacheSDL::RuntimeTextureCacheSDL(SDL_Renderer* value)
    : renderer(value)
{
}

RuntimeTextureCacheSDL::~RuntimeTextureCacheSDL()
{
    Clear();
}

void RuntimeTextureCacheSDL::SetRenderer(SDL_Renderer* value)
{
    if (renderer != value)
    {
        Clear();
        renderer = value;
    }
}

void RuntimeTextureCacheSDL::SetResolverConfig(RuntimeAssetResolverConfig config)
{
    if (!SameConfig(resolverConfig, config))
    {
        Clear();
        resolverConfig = std::move(config);
    }
}

void RuntimeTextureCacheSDL::SetAssetRoots(std::vector<RuntimeAssetRoot> roots)
{
    RuntimeAssetResolverConfig config = resolverConfig;
    config.roots = std::move(roots);
    SetResolverConfig(std::move(config));
}

void RuntimeTextureCacheSDL::Clear()
{
    for (auto& entry : textures)
    {
        if (entry.second.texture)
        {
            SDL_DestroyTexture(entry.second.texture);
            entry.second.texture = nullptr;
        }
    }

    textures.clear();
}

RuntimeTextureSDL* RuntimeTextureCacheSDL::GetTexture(const std::string& assetId)
{
    const auto cached = textures.find(assetId);
    if (cached != textures.end())
    {
        return &cached->second;
    }

    const std::filesystem::path path = ResolveRuntimeAssetPath(assetId, resolverConfig);
    if (path.empty())
    {
        AE::Logger::Warn("Runtime texture cache could not resolve texture asset: " + assetId);
        RuntimeTextureSDL texture;
        texture.failed = true;
        auto [inserted, _] = textures.emplace(assetId, texture);
        return &inserted->second;
    }

    return LoadTexture(assetId, path);
}

RuntimeTextureSDL* RuntimeTextureCacheSDL::GetTextureFromPath(
    const std::string& cacheId,
    const std::filesystem::path& path)
{
    const std::string key = cacheId.empty() ? path.generic_string() : cacheId;
    const auto cached = textures.find(key);
    if (cached != textures.end())
    {
        return &cached->second;
    }

    return LoadTexture(key, path);
}

RuntimeTextureSDL* RuntimeTextureCacheSDL::LoadTexture(const std::string& key, const std::filesystem::path& path)
{
    RuntimeTextureSDL record;
    record.resolvedPath = path;

    if (!renderer || path.empty())
    {
        record.failed = true;
        auto [inserted, _] = textures.emplace(key, record);
        return &inserted->second;
    }

    record.texture = IMG_LoadTexture(renderer, path.string().c_str());
    if (!record.texture)
    {
        record.failed = true;
        AE::Logger::Warn("Runtime texture cache could not load texture '" + path.string() + "': " + IMG_GetError());
        auto [inserted, _] = textures.emplace(key, record);
        return &inserted->second;
    }

    SDL_SetTextureBlendMode(record.texture, SDL_BLENDMODE_BLEND);
    SDL_QueryTexture(record.texture, nullptr, nullptr, &record.width, &record.height);

    auto [inserted, _] = textures.emplace(key, record);
    return &inserted->second;
}

}
