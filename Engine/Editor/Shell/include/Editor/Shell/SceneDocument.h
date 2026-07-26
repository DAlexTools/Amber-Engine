#ifndef AMBER_EDITOR_SHELL_SCENE_DOCUMENT_H
#define AMBER_EDITOR_SHELL_SCENE_DOCUMENT_H

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace AE::Editor
{

enum class SceneObjectKind
{
    Camera,
    Grid,
    RuntimeWorld,
    AssetInstance,
    Empty
};

struct EditorVec2
{
    float x = 0.0f;
    float y = 0.0f;
};

struct EditorTransform
{
    EditorVec2 position;
    float rotationDegrees = 0.0f;
    EditorVec2 scale{1.0f, 1.0f};
};

struct SceneObject
{
    std::uint32_t id = 0;
    std::string name;
    std::string assetId;
    std::string className = "Object";
    SceneObjectKind kind = SceneObjectKind::Empty;
    EditorTransform transform;
    EditorVec2 size{80.0f, 80.0f};
    bool visible = true;
    bool locked = false;
};

class SceneDocument
{
public:
    SceneDocument();

    void NewScene();
    const std::string& GetName() const;
    bool IsDirty() const;
    void SetDirty(bool dirty);

    const std::vector<SceneObject>& GetObjects() const;
    SceneObject* FindObject(std::uint32_t id);
    const SceneObject* FindObject(std::uint32_t id) const;
    SceneObject& AddAssetInstance(std::string name, std::string assetId, EditorTransform transform);
    bool RemoveObject(std::uint32_t id);
    bool IsObjectRemovable(std::uint32_t id) const;
    bool SaveToFile(const std::filesystem::path& path, std::string* error = nullptr);
    bool LoadFromFile(const std::filesystem::path& path, std::string* error = nullptr);

    static const char* KindName(SceneObjectKind kind);

private:
    SceneObject& AddObject(std::string name, SceneObjectKind kind, EditorTransform transform);

    std::string name = "Untitled Scene";
    bool dirty = false;
    std::uint32_t nextObjectId = 1;
    std::vector<SceneObject> objects;
};

}

#endif
