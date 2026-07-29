#ifndef AMBER_EDITOR_SHELL_EDITOR_VIEWPORT_H
#define AMBER_EDITOR_SHELL_EDITOR_VIEWPORT_H

#include "Game/RuntimeViewerSDL.h"
#include "Project/ProjectDescriptor.h"
#include "SceneDocument.h"

#include <SDL2/SDL.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace AE::Editor
{

class AssetRegistry;
class SelectionService;
class TextureCache;

enum class EditorTool
{
    Select,
    Move,
    Rotate,
    Scale
};

class EditorViewport
{
public:
    ~EditorViewport();

    struct ObjectBounds
    {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
    };

    struct AssetDropRequest
    {
        std::string assetId;
        EditorVec2 worldPosition;
    };

    enum class ViewportMode
    {
        EditPreview,
        PlayOutput
    };

    using RuntimeRenderCallback = std::function<void(RuntimeRenderContextSDL&)>;

    std::optional<AssetDropRequest> Draw(
        SceneDocument& sceneDocument,
        SelectionService& selection,
        const AssetRegistry& assetRegistry,
        TextureCache& textureCache,
        EditorTool activeTool,
        SDL_Window* window,
        SDL_Renderer* renderer,
        const ProjectDescriptor* activeProject,
        ViewportMode mode,
        bool paused,
        const RuntimeRenderCallback& runtimeRenderCallback = {});

    float GetZoom() const;
    void SetZoom(float value);
    void FocusOrigin();
    EditorVec2 GetViewCenter() const;
    void ReleaseRenderResources();

private:
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    float zoom = 1.0f;
    bool showGrid = true;
    bool panning = false;
    EditorVec2 panStartMouseScreen;
    EditorVec2 panStartCamera;

    enum class GizmoAxis
    {
        None,
        X,
        Y,
        XY
    };

    GizmoAxis activeGizmoAxis = GizmoAxis::None;
    std::uint32_t activeGizmoObjectId = 0;
    EditorVec2 dragStartMouseWorld;
    EditorVec2 dragStartObjectPosition;

    ObjectBounds GetObjectBounds(const SceneObject& object) const;
    bool EnsureRuntimePreviewTexture(SDL_Renderer* renderer, int width, int height);
    void DestroyRuntimePreviewTexture();

    RuntimeSceneRendererSDL runtimeSceneRenderer;
    SDL_Texture* runtimePreviewTexture = nullptr;
    int runtimePreviewWidth = 0;
    int runtimePreviewHeight = 0;
};

}

#endif
