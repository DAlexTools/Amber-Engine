#ifndef LEVELLOADER_H
#define LEVELLOADER_H

#include "../EntityComponentSystem/ECS.h"
#include <SDL2/SDL.h>
#include "../AssetManager/AssetManager.h"
#include <memory>
#include <sol/sol.hpp>

class LevelLoader
{
public:
	LevelLoader();
	~LevelLoader();
	void LoadLevel(sol::state& lua, const std::unique_ptr<Registry>& registry, const std::unique_ptr<AssetManager>& assetStore, SDL_Renderer* renderer, int levelNumber);
};

#endif