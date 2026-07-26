#include "Editor/Shell/TextureCache.h"

#include <SDL2/SDL_image.h>

namespace AE::Editor
{

TextureCache::~TextureCache()
{
    Clear();
}

void TextureCache::Initialize(SDL_Renderer* value)
{
    renderer = value;
}

void TextureCache::Clear()
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

TexturePreview* TextureCache::GetTexture(const AssetRecord& asset)
{
    if (!renderer || asset.type != AssetType::Texture)
    {
        return nullptr;
    }

    TexturePreview& preview = textures[asset.id];
    if (preview.texture || preview.failed)
    {
        return &preview;
    }

    preview.texture = IMG_LoadTexture(renderer, asset.absolutePath.string().c_str());
    if (!preview.texture)
    {
        preview.failed = true;
        return &preview;
    }

    SDL_SetTextureBlendMode(preview.texture, SDL_BLENDMODE_BLEND);
    SDL_QueryTexture(preview.texture, nullptr, nullptr, &preview.width, &preview.height);
    return &preview;
}

}
