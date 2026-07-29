#include "SceneDocument.h"

#include "Scene/SceneAsset.h"

#include <algorithm>
#include <utility>

namespace AE::Editor
{
namespace
{
    AE::Scene::ObjectKind ToRuntimeKind(SceneObjectKind kind)
    {
        switch (kind)
        {
            case SceneObjectKind::Camera:
                return AE::Scene::ObjectKind::Camera;
            case SceneObjectKind::Grid:
                return AE::Scene::ObjectKind::Grid;
            case SceneObjectKind::RuntimeWorld:
                return AE::Scene::ObjectKind::RuntimeWorld;
            case SceneObjectKind::AssetInstance:
                return AE::Scene::ObjectKind::AssetInstance;
            case SceneObjectKind::Box:
                return AE::Scene::ObjectKind::Box;
            case SceneObjectKind::Circle:
                return AE::Scene::ObjectKind::Circle;
            default:
                return AE::Scene::ObjectKind::Empty;
        }
    }

    SceneObjectKind FromRuntimeKind(AE::Scene::ObjectKind kind)
    {
        switch (kind)
        {
            case AE::Scene::ObjectKind::Camera:
                return SceneObjectKind::Camera;
            case AE::Scene::ObjectKind::Grid:
                return SceneObjectKind::Grid;
            case AE::Scene::ObjectKind::RuntimeWorld:
                return SceneObjectKind::RuntimeWorld;
            case AE::Scene::ObjectKind::AssetInstance:
                return SceneObjectKind::AssetInstance;
            case AE::Scene::ObjectKind::Box:
                return SceneObjectKind::Box;
            case AE::Scene::ObjectKind::Circle:
                return SceneObjectKind::Circle;
            default:
                return SceneObjectKind::Empty;
        }
    }
}

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

SceneObject& SceneDocument::AddAssetInstance(std::string objectName, std::string assetId, EditorTransform transform)
{
    SceneObject& object = AddObject(std::move(objectName), SceneObjectKind::AssetInstance, transform);
    object.assetId = std::move(assetId);
    object.className = "SpriteObject";
    dirty = true;
    return object;
}

SceneObject& SceneDocument::AddBoxObject(std::string objectName, EditorTransform transform, EditorVec2 size)
{
    SceneObject& object = AddObject(std::move(objectName), SceneObjectKind::Box, transform);
    object.className = "BoxObject";
    object.size = size;
    dirty = true;
    return object;
}

SceneObject& SceneDocument::AddCircleObject(std::string objectName, EditorTransform transform, EditorVec2 size)
{
    SceneObject& object = AddObject(std::move(objectName), SceneObjectKind::Circle, transform);
    object.className = "CircleObject";
    object.size = size;
    dirty = true;
    return object;
}

bool SceneDocument::RemoveObject(std::uint32_t id)
{
    const auto it = std::find_if(objects.begin(), objects.end(), [id](const SceneObject& object) {
        return object.id == id;
    });

    if (it == objects.end() || !IsObjectRemovable(id))
    {
        return false;
    }

    objects.erase(it);
    dirty = true;
    return true;
}

bool SceneDocument::IsObjectRemovable(std::uint32_t id) const
{
    const SceneObject* object = FindObject(id);
    if (!object || object->locked)
    {
        return false;
    }

    return object->kind == SceneObjectKind::AssetInstance ||
        object->kind == SceneObjectKind::Box ||
        object->kind == SceneObjectKind::Circle ||
        object->kind == SceneObjectKind::Empty;
}

bool SceneDocument::SaveToFile(const std::filesystem::path& path, std::string* error)
{
    AE::Scene::Document document = ToRuntimeDocument();

    if (!AE::Scene::SaveScene(document, path, error))
    {
        return false;
    }

    dirty = false;
    return true;
}

AE::Scene::Document SceneDocument::ToRuntimeDocument() const
{
    AE::Scene::Document document;
    document.name = name;
    document.objects.reserve(objects.size());

    for (const SceneObject& object : objects)
    {
        AE::Scene::ObjectData sceneObject;
        sceneObject.name = object.name;
        sceneObject.assetId = object.assetId;
        sceneObject.className = object.className.empty() ?
            (object.kind == SceneObjectKind::AssetInstance ? "SpriteObject" :
                object.kind == SceneObjectKind::Box ? "BoxObject" :
                object.kind == SceneObjectKind::Circle ? "CircleObject" :
                "Object") :
            object.className;
        sceneObject.kind = ToRuntimeKind(object.kind);
        sceneObject.transform.position = AE::Scene::Vec2{object.transform.position.x, object.transform.position.y};
        sceneObject.transform.rotationDegrees = object.transform.rotationDegrees;
        sceneObject.transform.scale = AE::Scene::Vec2{object.transform.scale.x, object.transform.scale.y};
        sceneObject.size = AE::Scene::Vec2{object.size.x, object.size.y};
        sceneObject.visible = object.visible;
        sceneObject.locked = object.locked;
        document.objects.push_back(std::move(sceneObject));
    }

    return document;
}

bool SceneDocument::LoadFromFile(const std::filesystem::path& path, std::string* error)
{
    AE::Scene::Document document;
    if (!AE::Scene::LoadScene(path, document, error))
    {
        return false;
    }

    name = document.name.empty() ? "Untitled Scene" : std::move(document.name);
    dirty = false;
    nextObjectId = 1;
    objects.clear();

    bool hasCamera = false;
    bool hasGrid = false;
    bool hasRuntimeWorld = false;

    for (const AE::Scene::ObjectData& sceneObject : document.objects)
    {
        EditorTransform transform;
        transform.position = EditorVec2{sceneObject.transform.position.x, sceneObject.transform.position.y};
        transform.rotationDegrees = sceneObject.transform.rotationDegrees;
        transform.scale = EditorVec2{sceneObject.transform.scale.x, sceneObject.transform.scale.y};

        const SceneObjectKind kind = FromRuntimeKind(sceneObject.kind);
        SceneObject& object = AddObject(sceneObject.name, kind, transform);
        object.assetId = sceneObject.assetId;
        object.className = sceneObject.className.empty() ?
            (kind == SceneObjectKind::AssetInstance ? "SpriteObject" :
                kind == SceneObjectKind::Box ? "BoxObject" :
                kind == SceneObjectKind::Circle ? "CircleObject" :
                "Object") :
            sceneObject.className;
        object.size = EditorVec2{sceneObject.size.x, sceneObject.size.y};
        object.visible = sceneObject.visible;
        object.locked = sceneObject.locked;

        hasCamera = hasCamera || kind == SceneObjectKind::Camera;
        hasGrid = hasGrid || kind == SceneObjectKind::Grid;
        hasRuntimeWorld = hasRuntimeWorld || kind == SceneObjectKind::RuntimeWorld;
    }

    if (!hasCamera)
    {
        AddObject("Editor Camera", SceneObjectKind::Camera, EditorTransform{EditorVec2{0.0f, 0.0f}, 0.0f, EditorVec2{1.0f, 1.0f}});
    }
    if (!hasGrid)
    {
        AddObject("Grid", SceneObjectKind::Grid, EditorTransform{EditorVec2{0.0f, 0.0f}, 0.0f, EditorVec2{1.0f, 1.0f}});
    }
    if (!hasRuntimeWorld)
    {
        AddObject("Runtime World", SceneObjectKind::RuntimeWorld, EditorTransform{EditorVec2{180.0f, 96.0f}, 0.0f, EditorVec2{1.0f, 1.0f}});
    }

    dirty = false;
    return true;
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
        case SceneObjectKind::AssetInstance:
            return "Asset Instance";
        case SceneObjectKind::Box:
            return "Box";
        case SceneObjectKind::Circle:
            return "Circle";
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
