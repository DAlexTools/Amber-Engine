#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <map>
#include <string>
#include <SDL2/SDL.h>
#include "SDL2/SDL_ttf.h"

/**
 * @class AssetManager
 * @brief Manages game assets such as textures to optimize resource handling and reuse.
 * 
 * This class provides methods to load, retrieve, and clear assets (textures) in the game.
 * It helps avoid redundant asset loading by storing textures with unique identifiers.
 */
class AssetManager
{
private:
    /**
     * @brief Stores loaded textures mapped to their unique asset IDs.
     * 
     * The key is a string representing the asset ID, and the value is a pointer to the corresponding SDL_Texture.
     */
    std::map<std::string, SDL_Texture*> textures;


    std::map<std::string, TTF_Font*> fonts; 

public:
    /**
     * @brief Constructs a new AssetManager object.
     * 
     * Initializes the asset manager with no loaded assets.
     */
    AssetManager();

    /**
     * @brief Destructs the AssetManager object.
     * 
     * Clears all loaded assets and releases memory associated with textures.
     */
    ~AssetManager();

    /**
     * @brief Clears all loaded assets from memory.
     * 
     * Releases memory associated with all loaded textures and clears the internal asset map.
     */
    void ClearAssets();

    /**
     * @brief Adds a texture to the asset manager.
     * 
     * Loads a texture from the specified file path and associates it with a unique asset ID.
     * 
     * @param renderer The SDL_Renderer to use for creating the texture.
     * @param assetID The unique identifier for the texture.
     * @param filePath The path to the image file to load as a texture.
     */
    void AddTexture(SDL_Renderer* renderer, const std::string& assetID, const std::string& filePath);

    /**
     * @brief Retrieves a texture by its unique asset ID.
     * 
     * Returns a pointer to the texture associated with the specified asset ID.
     * 
     * @param assetID The unique identifier for the texture.
     * @return SDL_Texture* Pointer to the requested texture, or nullptr if not found.
     */
    SDL_Texture* GetTexture(const std::string& assetID);  


    void AddFont(const std::string& assetID, const std::string& filePath, int fontSize);  
    TTF_Font* GetFont(const std::string& assetID);
};



#endif