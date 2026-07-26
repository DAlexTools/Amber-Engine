#include "Editor/Shell/SelectionService.h"

#include <utility>

namespace AE::Editor
{

void SelectionService::Clear()
{
    selection = EditorSelection{};
}

void SelectionService::SelectSceneObject(std::uint32_t objectId)
{
    selection = EditorSelection{};
    selection.type = EditorSelectionType::SceneObject;
    selection.objectId = objectId;
}

void SelectionService::SelectAsset(std::string assetPath)
{
    selection = EditorSelection{};
    selection.type = EditorSelectionType::Asset;
    selection.assetPath = std::move(assetPath);
}

const EditorSelection& SelectionService::GetSelection() const
{
    return selection;
}

bool SelectionService::IsSceneObjectSelected(std::uint32_t objectId) const
{
    return selection.type == EditorSelectionType::SceneObject && selection.objectId == objectId;
}

bool SelectionService::IsAssetSelected(const std::string& assetPath) const
{
    return selection.type == EditorSelectionType::Asset && selection.assetPath == assetPath;
}

}
