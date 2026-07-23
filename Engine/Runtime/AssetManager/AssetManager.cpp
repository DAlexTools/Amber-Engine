#include "AssetManager.h"
#include "SDL2/SDL_image.h"
#include "../Logging/Logger.h"

/**
 * @brief Constructs an AssetManager object and logs its creation.
 * 
 * Initializes the asset manager and provides a log message indicating the constructor has been called.
 */
AssetManager::AssetManager()
{
    AE::Logger::Log("Asset Manager constructor called");
}

/**
 * @brief Destructs the AssetManager object and clears all loaded assets.
 * 
 * Ensures that all textures are properly released from memory before the object is destroyed.
 * Logs the destruction of the AssetManager.
 */
AssetManager::~AssetManager()
{
    ClearAssets();
    AE::Logger::Log("Asset Manager destructor called");
}

/**
 * @brief Clears all loaded textures and releases associated memory.
 * 
 * Iterates through the `textures` map, destroys each SDL_Texture using `SDL_DestroyTexture`,
 * and clears the map to remove all entries.
 */
void AssetManager::ClearAssets()
{
    for (auto texture : textures)
    {
        SDL_DestroyTexture(texture.second); // Release the SDL_Texture from memory.
    }
    textures.clear();   // Clear the map of all stored textures.

    for (auto font : fonts)
    {
        TTF_CloseFont(font.second);
    }
    fonts.clear();      // Clear the map of all stored fonts

}

/**
 * @brief Adds a new texture to the asset manager.
 * 
 * Loads an image from the given file path, converts it to an SDL_Texture, and associates it with a unique asset ID.
 * If the image file cannot be loaded or the texture creation fails, an error is logged.
 * 
 * @param renderer The SDL_Renderer used to create the texture.
 * @param assetID The unique identifier to associate with the texture.
 * @param filePath The file path of the image to load as a texture.
 */
void AssetManager::AddTexture(SDL_Renderer* renderer, const std::string &assetID, const std::string &filePath)
{
    // Load the image file into an SDL_Surface.
    SDL_Surface* surface = IMG_Load(filePath.c_str());
    if (!surface) 
    {
        AE::Logger::Err("Error loading image file: " + filePath);
        return; // Exit the function if the surface cannot be loaded.
    }

    // Convert the SDL_Surface to an SDL_Texture.
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture)
    {
        AE::Logger::Err("Error creating texture from surface: " + filePath);
        SDL_FreeSurface(surface); // Free the surface before returning.
        return;
    }

    // Free the surface after creating the texture.
    SDL_FreeSurface(surface);

    // Add the texture to the map with its associated asset ID.
    textures.emplace(assetID, texture);
    AE::Logger::Log("New texture added to the asset manager with ID = " + assetID);
}

/**
 * @brief Retrieves a texture by its unique asset ID.
 * 
 * Searches the `textures` map for a texture associated with the given asset ID.
 * Returns the texture if found; if the asset ID is not found, this function may exhibit undefined behavior.
 * 
 * @param assetID The unique identifier for the texture.
 * @return SDL_Texture* Pointer to the texture associated with the asset ID.
 */
SDL_Texture* AssetManager::GetTexture(const std::string &assetID) 
{
    return textures[assetID]; // Access the texture from the map by its asset ID.
}

void AssetManager::AddFont(const std::string &assetID, const std::string &filePath, int fontSize)
{
    fonts.emplace(assetID, TTF_OpenFont(filePath.c_str(), fontSize));
}

TTF_Font* AssetManager::GetFont(const std::string &assetID)
{
    return fonts[assetID];  
}
