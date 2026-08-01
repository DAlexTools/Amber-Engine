#include "Assets/TextureCache.h"

#include <utility>

namespace AE::Editor
{

TextureCache::~TextureCache()
{
	Clear();
}

void TextureCache::Initialize(SDL_Renderer* value)
{
	runtimeTextureCache.SetRenderer(value);
}

void TextureCache::Clear()
{
	runtimeTextureCache.Clear();
	textures.clear();
}

void TextureCache::SetAssetRoots(std::vector<AssetRoot> roots)
{
	runtimeTextureCache.SetAssetRoots(std::move(roots));
	textures.clear();
}

TexturePreview* TextureCache::GetTexture(const AssetRecord& asset)
{
	if (asset.type != AssetType::Texture)
	{
		return nullptr;
	}

	TexturePreview& preview = textures[asset.id];
	if (preview.texture || preview.failed)
	{
		return &preview;
	}

	RuntimeTextureSDL* texture = runtimeTextureCache.GetTexture(asset.id);
	if (!texture || !texture->texture)
	{
		preview.failed = true;
		return &preview;
	}

	preview.texture = texture->texture;
	preview.width = texture->width;
	preview.height = texture->height;
	preview.failed = texture->failed;
	return &preview;
}

} // namespace AE::Editor
