#include "Editor/Shell/SceneDocument.h"

#include <utility>

namespace AE::Editor
{

SceneDocument::SceneDocument()
{
    NewScene();
}

void SceneDocument::NewScene()
{
    name = "Untitled Scene";
    dirty = false;
    nextObjectId = 1;
    objects.clear();

    AddObject("Editor Camera", SceneObjectKind::Camera, EditorTransform{EditorVec2{0.0f, 0.0f}, 0.0f, EditorVec2{1.0f, 1.0f}});
    AddObject("Grid", SceneObjectKind::Grid, EditorTransform{EditorVec2{0.0f, 0.0f}, 0.0f, EditorVec2{1.0f, 1.0f}});
    AddObject("Runtime World", SceneObjectKind::RuntimeWorld, EditorTransform{EditorVec2{180.0f, 96.0f}, 0.0f, EditorVec2{1.0f, 1.0f}});
}

const std::string& SceneDocument::GetName() const
{
    return name;
}

bool SceneDocument::IsDirty() const
{
    return dirty;
}

void SceneDocument::SetDirty(bool value)
{
    dirty = value;
}

const std::vector<SceneObject>& SceneDocument::GetObjects() const
{
    return objects;
}

SceneObject* SceneDocument::FindObject(std::uint32_t id)
{
    for (SceneObject& object : objects)
    {
        if (object.id == id)
        {
            return &object;
        }
    }

    return nullptr;
}

const SceneObject* SceneDocument::FindObject(std::uint32_t id) const
{
    for (const SceneObject& object : objects)
    {
        if (object.id == id)
        {
            return &object;
        }
    }

    return nullptr;
}

const char* SceneDocument::KindName(SceneObjectKind kind)
{
    switch (kind)
    {
        case SceneObjectKind::Camera:
            return "Camera";
        case SceneObjectKind::Grid:
            return "Grid";
        case SceneObjectKind::RuntimeWorld:
            return "Runtime World";
        default:
            return "Empty";
    }
}

SceneObject& SceneDocument::AddObject(std::string objectName, SceneObjectKind kind, EditorTransform transform)
{
    SceneObject object;
    object.id = nextObjectId++;
    object.name = std::move(objectName);
    object.kind = kind;
    object.transform = transform;
    objects.push_back(std::move(object));
    return objects.back();
}

}
