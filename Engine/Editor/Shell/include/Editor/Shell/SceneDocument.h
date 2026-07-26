#ifndef AMBER_EDITOR_SHELL_SCENE_DOCUMENT_H
#define AMBER_EDITOR_SHELL_SCENE_DOCUMENT_H

#include <cstdint>
#include <string>
#include <vector>

namespace AE::Editor
{

enum class SceneObjectKind
{
    Camera,
    Grid,
    RuntimeWorld,
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
    SceneObjectKind kind = SceneObjectKind::Empty;
    EditorTransform transform;
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
