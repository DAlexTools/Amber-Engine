#include "Game/RuntimeViewerSDL.h"

#include "Logging/Logger.h"

#include <SDL2/SDL_image.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace AE
{
namespace
{
    void SetError(std::string* error, const std::string& message)
    {
        if (error)
        {
            *error = message;
        }
    }

    SDL_Color ShapeFillColor(const Scene::ObjectData& object)
    {
        if (object.className == "PlayerSpawnObject")
        {
            return SDL_Color{74, 178, 116, 120};
        }
        if (object.className == "GoalObject")
        {
            return SDL_Color{228, 83, 86, 120};
        }
        if (object.className == "CoinObject")
        {
            return SDL_Color{232, 186, 68, 150};
        }
        if (object.className == "SolidPlatformObject")
        {
            return SDL_Color{95, 142, 78, 140};
        }

        return object.kind == Scene::ObjectKind::Circle ?
            SDL_Color{225, 142, 72, 120} :
            SDL_Color{78, 150, 204, 110};
    }

    SDL_Color ShapeOutlineColor(const Scene::ObjectData& object)
    {
        if (object.className == "PlayerSpawnObject")
        {
            return SDL_Color{104, 224, 152, 235};
        }
        if (object.className == "GoalObject")
        {
            return SDL_Color{255, 116, 118, 235};
        }
        if (object.className == "CoinObject")
        {
            return SDL_Color{255, 214, 86, 245};
        }
        if (object.className == "SolidPlatformObject")
        {
            return SDL_Color{128, 184, 96, 235};
        }

        return SDL_Color{104, 184, 238, 230};
    }

    void SetDrawColor(SDL_Renderer* renderer, SDL_Color color)
    {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    }

    void DrawRectOutline(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color color)
    {
        SetDrawColor(renderer, color);
        SDL_RenderDrawRect(renderer, &rect);
        if (rect.w > 2 && rect.h > 2)
        {
            SDL_Rect inner{rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2};
            SDL_RenderDrawRect(renderer, &inner);
        }
    }

    void DrawFilledCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius, SDL_Color fill, SDL_Color outline)
    {
        if (radius <= 0)
        {
            return;
        }

        SetDrawColor(renderer, fill);
        for (int y = -radius; y <= radius; ++y)
        {
            const int x = static_cast<int>(std::sqrt(static_cast<double>(radius * radius - y * y)));
            SDL_RenderDrawLine(renderer, centerX - x, centerY + y, centerX + x, centerY + y);
        }

        SetDrawColor(renderer, outline);
        int x = radius - 1;
        int y = 0;
        int dx = 1;
        int dy = 1;
        int err = dx - (radius << 1);

        while (x >= y)
        {
            SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
            SDL_RenderDrawPoint(renderer, centerX + y, centerY + x);
            SDL_RenderDrawPoint(renderer, centerX - y, centerY + x);
            SDL_RenderDrawPoint(renderer, centerX - x, centerY + y);
            SDL_RenderDrawPoint(renderer, centerX - x, centerY - y);
            SDL_RenderDrawPoint(renderer, centerX - y, centerY - x);
            SDL_RenderDrawPoint(renderer, centerX + y, centerY - x);
            SDL_RenderDrawPoint(renderer, centerX + x, centerY - y);

            if (err <= 0)
            {
                ++y;
                err += dy;
                dy += 2;
            }
            if (err > 0)
            {
                --x;
                dx += 2;
                err += dx - (radius << 1);
            }
        }
    }

    bool SceneHasVisibleGrid(const Scene::Document& scene)
    {
        for (const Scene::ObjectData& object : scene.objects)
        {
            if (object.visible && object.kind == Scene::ObjectKind::Grid)
            {
                return true;
            }
        }
        return false;
    }
}

RuntimeSceneRendererSDL::RuntimeSceneRendererSDL(SDL_Renderer* value)
    : renderer(value)
    , textureCache(value)
{
}

RuntimeSceneRendererSDL::~RuntimeSceneRendererSDL()
{
    ClearTextureCache();
}

void RuntimeSceneRendererSDL::SetRenderer(SDL_Renderer* value)
{
    if (renderer != value)
    {
        ClearTextureCache();
        renderer = value;
        textureCache.SetRenderer(value);
    }
}

void RuntimeSceneRendererSDL::ClearTextureCache()
{
    textureCache.Clear();
}

void RuntimeSceneRendererSDL::RenderScene(
    const Scene::Document& scene,
    const RuntimeSceneRendererConfig& config,
    const SDL_Rect& viewport)
{
    if (!renderer)
    {
        return;
    }

    SetDrawColor(renderer, SDL_Color{16, 18, 20, 255});
    SDL_RenderFillRect(renderer, &viewport);

    if (config.showGrid && SceneHasVisibleGrid(scene))
    {
        const float gridStep = std::max(4.0f, 32.0f * config.zoom);
        const float centerX = viewport.x + viewport.w * 0.5f;
        const float centerY = viewport.y + viewport.h * 0.5f;
        const float originX = centerX - config.cameraX * config.zoom;
        const float originY = centerY - config.cameraY * config.zoom;

        SetDrawColor(renderer, SDL_Color{58, 65, 72, 120});
        for (float x = std::fmod(originX - viewport.x, gridStep); x < viewport.w; x += gridStep)
        {
            const int lineX = static_cast<int>(std::round(viewport.x + x));
            SDL_RenderDrawLine(renderer, lineX, viewport.y, lineX, viewport.y + viewport.h);
        }
        for (float y = std::fmod(originY - viewport.y, gridStep); y < viewport.h; y += gridStep)
        {
            const int lineY = static_cast<int>(std::round(viewport.y + y));
            SDL_RenderDrawLine(renderer, viewport.x, lineY, viewport.x + viewport.w, lineY);
        }

        SetDrawColor(renderer, SDL_Color{132, 74, 74, 170});
        const int axisY = static_cast<int>(std::round(originY));
        SDL_RenderDrawLine(renderer, viewport.x, axisY, viewport.x + viewport.w, axisY);
        SetDrawColor(renderer, SDL_Color{75, 126, 86, 170});
        const int axisX = static_cast<int>(std::round(originX));
        SDL_RenderDrawLine(renderer, axisX, viewport.y, axisX, viewport.y + viewport.h);
    }

    for (const Scene::ObjectData& object : scene.objects)
    {
        if (!object.visible ||
            object.kind == Scene::ObjectKind::Grid ||
            object.kind == Scene::ObjectKind::Camera)
        {
            continue;
        }

        const SDL_Rect rect = GetObjectRect(object, viewport, config);
        if (rect.w <= 0 || rect.h <= 0)
        {
            continue;
        }

        if (object.kind == Scene::ObjectKind::AssetInstance && !object.assetId.empty())
        {
            SDL_Texture* texture = GetTexture(object.assetId, config);
            if (texture)
            {
                const SDL_Point center{rect.w / 2, rect.h / 2};
                SDL_RenderCopyEx(renderer, texture, nullptr, &rect, object.transform.rotationDegrees, &center, SDL_FLIP_NONE);
                continue;
            }
        }

        if (object.kind == Scene::ObjectKind::Circle)
        {
            DrawFilledCircle(
                renderer,
                rect.x + rect.w / 2,
                rect.y + rect.h / 2,
                std::max(1, std::min(rect.w, rect.h) / 2),
                ShapeFillColor(object),
                ShapeOutlineColor(object));
        }
        else
        {
            SDL_Color fill = ShapeFillColor(object);
            SDL_Color outline = ShapeOutlineColor(object);
            SetDrawColor(renderer, fill);
            SDL_RenderFillRect(renderer, &rect);
            DrawRectOutline(renderer, rect, outline);
        }
    }
}

SDL_Texture* RuntimeSceneRendererSDL::GetTexture(const std::string& assetId, const RuntimeSceneRendererConfig& config)
{
    RuntimeAssetResolverConfig resolverConfig;
    resolverConfig.projectRoot = config.projectRoot;
    resolverConfig.engineRoot = config.engineRoot;
    resolverConfig.contentRoot = config.contentRoot;
    resolverConfig.roots = config.assetRoots;
    textureCache.SetResolverConfig(std::move(resolverConfig));

    RuntimeTextureSDL* texture = textureCache.GetTexture(assetId);
    return texture ? texture->texture : nullptr;
}

SDL_Rect RuntimeSceneRendererSDL::GetObjectRect(
    const Scene::ObjectData& object,
    const SDL_Rect& viewport,
    const RuntimeSceneRendererConfig& config) const
{
    const Scene::Vec2 center = object.transform.position;
    const SDL_Point screen = WorldToScreen(center, viewport, config);
    const int width = std::max(1, static_cast<int>(std::round(object.size.x * object.transform.scale.x * config.zoom)));
    const int height = std::max(1, static_cast<int>(std::round(object.size.y * object.transform.scale.y * config.zoom)));
    return SDL_Rect{screen.x - width / 2, screen.y - height / 2, width, height};
}

SDL_Point RuntimeSceneRendererSDL::WorldToScreen(
    const Scene::Vec2& point,
    const SDL_Rect& viewport,
    const RuntimeSceneRendererConfig& config) const
{
    const float centerX = viewport.x + viewport.w * 0.5f;
    const float centerY = viewport.y + viewport.h * 0.5f;
    return SDL_Point{
        static_cast<int>(std::round(centerX + (point.x - config.cameraX) * config.zoom)),
        static_cast<int>(std::round(centerY + (point.y - config.cameraY) * config.zoom))
    };
}

RuntimeViewerSDL::~RuntimeViewerSDL()
{
    Shutdown();
}

bool RuntimeViewerSDL::Initialize(const RuntimeViewerSDLConfig& config, std::string* error)
{
    width = std::max(320, config.width);
    height = std::max(240, config.height);
    fullscreen = config.startFullscreen;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        SetError(error, std::string("SDL_Init failed: ") + SDL_GetError());
        return false;
    }
    sdlInitialized = true;

    const int imageFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    const int initializedImageFlags = IMG_Init(imageFlags);
    imageInitialized = initializedImageFlags != 0;
    if ((initializedImageFlags & IMG_INIT_PNG) == 0)
    {
        AE::Logger::Warn(std::string("SDL_image PNG loader is unavailable: ") + IMG_GetError());
    }

    window = SDL_CreateWindow(
        config.windowTitle.empty() ? "Amber Runtime" : config.windowTitle.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SetError(error, std::string("SDL_CreateWindow failed: ") + SDL_GetError());
        Shutdown();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer)
    {
        SetError(error, std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
        Shutdown();
        return false;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    sceneRenderer.SetRenderer(renderer);

    if (fullscreen)
    {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }

    return true;
}

void RuntimeViewerSDL::Shutdown()
{
    sceneRenderer.ClearTextureCache();

    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    if (imageInitialized)
    {
        IMG_Quit();
        imageInitialized = false;
    }
    if (sdlInitialized)
    {
        SDL_Quit();
        sdlInitialized = false;
    }
}

bool RuntimeViewerSDL::PollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            return false;
        }
        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                return false;
            }
            if (event.key.keysym.sym == SDLK_F11)
            {
                ToggleFullscreen();
            }
        }
    }
    return true;
}

void RuntimeViewerSDL::BeginFrame()
{
    if (!renderer)
    {
        return;
    }

    SetDrawColor(renderer, SDL_Color{16, 18, 20, 255});
    SDL_RenderClear(renderer);
}

void RuntimeViewerSDL::RenderScene(const Scene::Document& scene, const RuntimeSceneRendererConfig& config)
{
    sceneRenderer.RenderScene(scene, config, GetViewport());
}

void RuntimeViewerSDL::Present()
{
    if (renderer)
    {
        SDL_RenderPresent(renderer);
    }
}

RuntimeRenderContextSDL RuntimeViewerSDL::MakeRenderContext(
    const ProjectDescriptor& project,
    const Scene::Document& scene) const
{
    return RuntimeRenderContextSDL{window, renderer, GetViewport(), &project, &scene};
}

SDL_Window* RuntimeViewerSDL::GetWindow() const
{
    return window;
}

SDL_Renderer* RuntimeViewerSDL::GetRenderer() const
{
    return renderer;
}

SDL_Rect RuntimeViewerSDL::GetViewport() const
{
    int outputWidth = width;
    int outputHeight = height;
    if (renderer)
    {
        SDL_GetRendererOutputSize(renderer, &outputWidth, &outputHeight);
    }
    return SDL_Rect{0, 0, std::max(1, outputWidth), std::max(1, outputHeight)};
}

void RuntimeViewerSDL::ToggleFullscreen()
{
    if (!window)
    {
        return;
    }

    fullscreen = !fullscreen;
    SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

}
