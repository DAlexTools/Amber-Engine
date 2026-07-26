#ifndef AMBER_EDITOR_SHELL_EDITOR_VIEWPORT_H
#define AMBER_EDITOR_SHELL_EDITOR_VIEWPORT_H

#include "Editor/Shell/SceneDocument.h"

namespace AE::Editor
{

class SelectionService;

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

    void Draw(SceneDocument& sceneDocument, SelectionService& selection, bool playing, bool paused);

    float GetZoom() const;
    void SetZoom(float value);
    void FocusOrigin();

private:
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    float zoom = 1.0f;
    bool showGrid = true;

    ObjectBounds GetObjectBounds(const SceneObject& object) const;
};

}

#endif
