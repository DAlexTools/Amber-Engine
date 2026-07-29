#ifndef AMBER_RUNTIME_GAME_RUNTIME_VIEWER_SDL_H
#define AMBER_RUNTIME_GAME_RUNTIME_VIEWER_SDL_H

#include "Assets/AssetResolver.h"
#include "Project/ProjectDescriptor.h"
#include "Game/RuntimeTextureCacheSDL.h"
#include "Scene/SceneAsset.h"

#include <SDL2/SDL.h>

#include <filesystem>
#include <string>
#include <vector>

namespace AE
{

struct RuntimeSceneRendererConfig
{
    std::filesystem::path projectRoot;
    std::filesystem::path engineRoot;
    std::filesystem::path contentRoot;
    std::vector<RuntimeAssetRoot> assetRoots;
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    float zoom = 1.0f;
    bool showGrid = true;
};

struct RuntimeRenderContextSDL
{
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Rect viewport{0, 0, 0, 0};
    const ProjectDescriptor* project = nullptr;
    const Scene::Document* scene = nullptr;
};

struct RuntimeViewerSDLConfig
{
    std::string windowTitle = "Amber Runtime";
    int width = 1280;
    int height = 720;
    bool startFullscreen = false;
};

class RuntimeSceneRendererSDL
{
public:
    explicit RuntimeSceneRendererSDL(SDL_Renderer* renderer = nullptr);
    ~RuntimeSceneRendererSDL();

    RuntimeSceneRendererSDL(const RuntimeSceneRendererSDL&) = delete;
    RuntimeSceneRendererSDL& operator=(const RuntimeSceneRendererSDL&) = delete;

    void SetRenderer(SDL_Renderer* value);
    void ClearTextureCache();
    void RenderScene(const Scene::Document& scene, const RuntimeSceneRendererConfig& config, const SDL_Rect& viewport);

private:
    SDL_Texture* GetTexture(const std::string& assetId, const RuntimeSceneRendererConfig& config);
    SDL_Rect GetObjectRect(const Scene::ObjectData& object, const SDL_Rect& viewport, const RuntimeSceneRendererConfig& config) const;
    SDL_Point WorldToScreen(const Scene::Vec2& point, const SDL_Rect& viewport, const RuntimeSceneRendererConfig& config) const;

    SDL_Renderer* renderer = nullptr;
    RuntimeTextureCacheSDL textureCache;
};

class RuntimeViewerSDL
{
public:
    RuntimeViewerSDL() = default;
    ~RuntimeViewerSDL();

    RuntimeViewerSDL(const RuntimeViewerSDL&) = delete;
    RuntimeViewerSDL& operator=(const RuntimeViewerSDL&) = delete;

    bool Initialize(const RuntimeViewerSDLConfig& config, std::string* error = nullptr);
    void Shutdown();

    bool PollEvents();
    void BeginFrame();
    void RenderScene(const Scene::Document& scene, const RuntimeSceneRendererConfig& config);
    void Present();

    RuntimeRenderContextSDL MakeRenderContext(const ProjectDescriptor& project, const Scene::Document& scene) const;
    SDL_Window* GetWindow() const;
    SDL_Renderer* GetRenderer() const;
    SDL_Rect GetViewport() const;

private:
    void ToggleFullscreen();

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    RuntimeSceneRendererSDL sceneRenderer;
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    bool sdlInitialized = false;
    bool imageInitialized = false;
};

}

#endif
