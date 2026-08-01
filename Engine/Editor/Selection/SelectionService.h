#ifndef AMBER_EDITOR_SELECTION_SERVICE_H
#define AMBER_EDITOR_SELECTION_SERVICE_H

#include "Core/Platform/PlatformTypes.h"
#include <string>

namespace AE::Editor
{

enum class EditorSelectionType
{
	None,
	SceneObject,
	Asset
};

struct EditorSelection
{
	EditorSelectionType type = EditorSelectionType::None;
	uint32 objectId = 0;
	std::string assetId;
};

class SelectionService
{
public:
	void Clear();
	void SelectSceneObject(uint32 objectId);
	void SelectAsset(std::string assetId);

	const EditorSelection& GetSelection() const;
	bool IsSceneObjectSelected(uint32 objectId) const;
	bool IsAssetSelected(const std::string& assetId) const;

private:
	EditorSelection selection;
};

} // namespace AE::Editor

#endif
