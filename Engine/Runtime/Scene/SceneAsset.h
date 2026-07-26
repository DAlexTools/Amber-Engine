#ifndef AMBER_RUNTIME_SCENE_SCENE_ASSET_H
#define AMBER_RUNTIME_SCENE_SCENE_ASSET_H

#include <filesystem>
#include <string>
#include <vector>

namespace AE::Scene
{

enum class ObjectKind
{
    Camera,
    Grid,
    RuntimeWorld,
    AssetInstance,
    Box,
    Circle,
    Empty
};

struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;
};

struct Transform
{
    Vec2 position;
    float rotationDegrees = 0.0f;
    Vec2 scale{1.0f, 1.0f};
};

struct ObjectData
{
    std::string name;
    std::string assetId;
    std::string className = "Object";
    ObjectKind kind = ObjectKind::Empty;
    Transform transform;
    Vec2 size{80.0f, 80.0f};
    bool visible = true;
    bool locked = false;
};

struct Document
{
    std::string name = "Untitled Scene";
    std::vector<ObjectData> objects;
};

const char* ObjectKindName(ObjectKind kind);
bool TryParseObjectKind(const std::string& value, ObjectKind& kind);

bool SaveScene(const Document& document, const std::filesystem::path& path, std::string* error = nullptr);
bool LoadScene(const std::filesystem::path& path, Document& document, std::string* error = nullptr);

}

#endif
