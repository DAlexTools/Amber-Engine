#ifndef AMBER_EDITOR_SHELL_SELECTION_SERVICE_H
#define AMBER_EDITOR_SHELL_SELECTION_SERVICE_H

#include <cstdint>
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
    std::uint32_t objectId = 0;
    std::string assetPath;
};

class SelectionService
{
public:
    void Clear();
    void SelectSceneObject(std::uint32_t objectId);
    void SelectAsset(std::string assetPath);

    const EditorSelection& GetSelection() const;
    bool IsSceneObjectSelected(std::uint32_t objectId) const;
    bool IsAssetSelected(const std::string& assetPath) const;

private:
    EditorSelection selection;
};

}

#endif
