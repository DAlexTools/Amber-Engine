#include "Selection/SelectionService.h"

#include <utility>

namespace AE::Editor
{

void SelectionService::Clear()
{
	selection = EditorSelection{};
}

void SelectionService::SelectSceneObject(uint32 objectId)
{
	selection = EditorSelection{};
	selection.type = EditorSelectionType::SceneObject;
	selection.objectId = objectId;
}

void SelectionService::SelectAsset(std::string assetId)
{
	selection = EditorSelection{};
	selection.type = EditorSelectionType::Asset;
	selection.assetId = std::move(assetId);
}

const EditorSelection& SelectionService::GetSelection() const
{
	return selection;
}

bool SelectionService::IsSceneObjectSelected(uint32 objectId) const
{
	return selection.type == EditorSelectionType::SceneObject && selection.objectId == objectId;
}

bool SelectionService::IsAssetSelected(const std::string& assetId) const
{
	return selection.type == EditorSelectionType::Asset && selection.assetId == assetId;
}

} // namespace AE::Editor
