#ifndef AMBER_EDITOR_SHELL_SCENE_DOCUMENT_H
#define AMBER_EDITOR_SHELL_SCENE_DOCUMENT_H

#include "Core/Platform/PlatformTypes.h"
#include "Scene/SceneAsset.h"

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
	Box,
	Circle,
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
	uint32 id = 0;
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
	SceneObject* FindObject(uint32 id);
	const SceneObject* FindObject(uint32 id) const;
	SceneObject& AddAssetInstance(std::string name, std::string assetId, EditorTransform transform);
	SceneObject& AddBoxObject(std::string name, EditorTransform transform, EditorVec2 size = EditorVec2{96.0f, 64.0f});
	SceneObject& AddCircleObject(std::string name, EditorTransform transform, EditorVec2 size = EditorVec2{64.0f, 64.0f});
	bool RemoveObject(uint32 id);
	bool IsObjectRemovable(uint32 id) const;
	bool SaveToFile(const std::filesystem::path& path, std::string* error = nullptr);
	bool LoadFromFile(const std::filesystem::path& path, std::string* error = nullptr);
	AE::Scene::Document ToRuntimeDocument() const;

	static const char* KindName(SceneObjectKind kind);

private:
	SceneObject& AddObject(std::string name, SceneObjectKind kind, EditorTransform transform);

	std::string name = "Untitled Scene";
	bool dirty = false;
	uint32 nextObjectId = 1;
	std::vector<SceneObject> objects;
};

} // namespace AE::Editor

#endif
