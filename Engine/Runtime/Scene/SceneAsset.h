#ifndef AMBER_RUNTIME_SCENE_SCENE_ASSET_H
#define AMBER_RUNTIME_SCENE_SCENE_ASSET_H

#include "Core/Platform/PlatformTypes.h"

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

enum class ComponentPropertyType
{
	Bool,
	Int,
	Float,
	String
};

struct ComponentPropertyData
{
	std::string name;
	ComponentPropertyType type = ComponentPropertyType::String;
	std::string value;
};

struct ComponentData
{
	std::string name;
	std::vector<ComponentPropertyData> properties;
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
	std::vector<ComponentData> components;
};

struct Document
{
	std::string name = "Untitled Scene";
	std::vector<ObjectData> objects;
};

const char* ObjectKindName(ObjectKind kind);
bool TryParseObjectKind(const std::string& value, ObjectKind& kind);
const char* ComponentPropertyTypeName(ComponentPropertyType type);
bool TryParseComponentPropertyType(const std::string& value, ComponentPropertyType& type);

ComponentData* FindComponent(ObjectData& Object, const std::string& ComponentName);
const ComponentData* FindComponent(const ObjectData& Object, const std::string& ComponentName);
ComponentPropertyData* FindComponentProperty(ObjectData& Object, const std::string& ComponentName, const std::string& PropertyName);
const ComponentPropertyData* FindComponentProperty(const ObjectData& Object, const std::string& ComponentName, const std::string& PropertyName);

bool GetComponentPropertyBool(const ObjectData& Object, const std::string& ComponentName, const std::string& PropertyName, bool DefaultValue);
int32 GetComponentPropertyInt(const ObjectData& Object, const std::string& ComponentName, const std::string& PropertyName, int32 DefaultValue);
float GetComponentPropertyFloat(const ObjectData& Object, const std::string& ComponentName, const std::string& PropertyName, float DefaultValue);
std::string GetComponentPropertyString(const ObjectData& Object, const std::string& ComponentName, const std::string& PropertyName, std::string DefaultValue);

bool SaveScene(const Document& document, const std::filesystem::path& path, std::string* error = nullptr);
bool LoadScene(const std::filesystem::path& path, Document& document, std::string* error = nullptr);

} // namespace AE::Scene

#endif
