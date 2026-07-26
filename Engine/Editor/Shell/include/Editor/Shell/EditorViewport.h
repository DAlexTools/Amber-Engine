#ifndef AMBER_EDITOR_SHELL_EDITOR_VIEWPORT_H
#define AMBER_EDITOR_SHELL_EDITOR_VIEWPORT_H

#include "Editor/Shell/SceneDocument.h"

#include <cstdint>
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

    std::optional<AssetDropRequest> Draw(
        SceneDocument& sceneDocument,
        SelectionService& selection,
        const AssetRegistry& assetRegistry,
        TextureCache& textureCache,
        EditorTool activeTool,
        bool playing,
        bool paused);

    float GetZoom() const;
    void SetZoom(float value);
    void FocusOrigin();
    EditorVec2 GetViewCenter() const;

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
};

}

#endif
