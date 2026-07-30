#ifndef AMBER_EDITOR_SHELL_ACTOR_TYPE_REGISTRY_H
#define AMBER_EDITOR_SHELL_ACTOR_TYPE_REGISTRY_H

#include "SceneDocument.h"

#include <filesystem>
#include <string>
#include <vector>

namespace AE::Editor
{

struct FActorComponentPropertySchema
{
    std::string Name;
    AE::Scene::ComponentPropertyType Type = AE::Scene::ComponentPropertyType::String;
    std::string DefaultValue;
};

struct FActorComponentSchema
{
    std::string Name;
    std::vector<FActorComponentPropertySchema> Properties;
};

struct FActorPreviewColor
{
    uint8 R = 78;
    uint8 G = 150;
    uint8 B = 204;
    uint8 A = 176;
};

struct FActorTypeDefinition
{
    std::string TypeId;
    std::string DisplayName;
    std::string ClassName;
    std::string Category;
    SceneObjectKind Kind = SceneObjectKind::Empty;
    EditorVec2 DefaultSize{80.0f, 80.0f};
    FActorPreviewColor FillColor;
    FActorPreviewColor OutlineColor{104, 184, 238, 230};
    std::vector<FActorComponentSchema> Components;
};

class FActorTypeRegistry
{
public:
    void Clear();
    void RegisterActorType(FActorTypeDefinition ActorType);

    const std::vector<FActorTypeDefinition>& GetActorTypes() const;
    const FActorTypeDefinition* FindByTypeId(const std::string& TypeId) const;
    const FActorTypeDefinition* FindByClassName(const std::string& ClassName) const;

    bool IsManagedComponentName(const std::string& ComponentName) const;
    bool IsComponentExpectedForClass(const std::string& ClassName, const std::string& ComponentName) const;

private:
    std::vector<FActorTypeDefinition> ActorTypes;
};

void RegisterDefaultActorTypes(FActorTypeRegistry& Registry);
bool LoadActorTypesFromFile(const std::filesystem::path& Path, FActorTypeRegistry& Registry, std::string* Error = nullptr);

} // namespace AE::Editor

#endif
