#ifndef AMBER_EDITOR_SHELL_ASSET_REGISTRY_H
#define AMBER_EDITOR_SHELL_ASSET_REGISTRY_H

#include <filesystem>
#include <string>
#include <vector>

namespace AE::Editor
{

enum class AssetType
{
    Unknown,
    Texture,
    Tilemap,
    Script,
    Sound,
    Font,
    Scene
};

struct AssetRecord
{
    std::string id;
    std::string rootName;
    std::filesystem::path absolutePath;
    std::filesystem::path relativePath;
    std::string name;
    AssetType type = AssetType::Unknown;
};

struct AssetRoot
{
    std::string name;
    std::filesystem::path path;
};

class AssetRegistry
{
public:
    void Scan(const std::filesystem::path& root);
    void ScanRoots(const std::vector<AssetRoot>& roots);

    const std::filesystem::path& GetContentRoot() const;
    const std::vector<AssetRoot>& GetRoots() const;
    const std::vector<AssetRecord>& GetAssets() const;
    const AssetRecord* FindAssetById(const std::string& id) const;
    std::vector<const AssetRecord*> GetAssetsInDirectory(const std::filesystem::path& directory) const;

    static AssetType Classify(const std::filesystem::path& path);
    static const char* TypeName(AssetType type);
    static bool CanInstantiate(AssetType type);

private:
    std::filesystem::path contentRoot;
    std::vector<AssetRoot> roots;
    std::vector<AssetRecord> assets;
};

}

#endif
