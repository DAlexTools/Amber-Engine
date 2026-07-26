#include "Editor/Shell/AssetRegistry.h"

#include <algorithm>
#include <cctype>
#include <system_error>
#include <utility>

namespace AE::Editor
{
namespace
{
    std::string ToLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value;
    }

    bool HasSuffix(const std::string& value, const std::string& suffix)
    {
        return value.size() >= suffix.size() &&
            value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
}

void AssetRegistry::Scan(const std::filesystem::path& root)
{
    ScanRoots(std::vector<AssetRoot>{AssetRoot{"Project", root}});
}

void AssetRegistry::ScanRoots(const std::vector<AssetRoot>& scanRoots)
{
    assets.clear();
    roots.clear();
    contentRoot.clear();

    for (const AssetRoot& inputRoot : scanRoots)
    {
        std::error_code error;
        std::filesystem::path canonicalRoot = std::filesystem::weakly_canonical(inputRoot.path, error);
        if (canonicalRoot.empty())
        {
            canonicalRoot = inputRoot.path;
        }

        if (!std::filesystem::exists(canonicalRoot, error) || !std::filesystem::is_directory(canonicalRoot, error))
        {
            continue;
        }

        AssetRoot rootRecord{inputRoot.name, canonicalRoot};
        roots.push_back(rootRecord);
        if (contentRoot.empty())
        {
            contentRoot = canonicalRoot;
        }

        for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(canonicalRoot, error))
        {
            if (error)
            {
                break;
            }
            if (!entry.is_regular_file(error))
            {
                continue;
            }

            const std::filesystem::path absolutePath = entry.path();
            const std::filesystem::path relativePath = std::filesystem::relative(absolutePath, canonicalRoot, error);
            if (error || relativePath.empty())
            {
                continue;
            }

            AssetRecord asset;
            asset.rootName = rootRecord.name;
            asset.absolutePath = absolutePath;
            asset.relativePath = relativePath;
            asset.id = rootRecord.name.empty() ?
                relativePath.generic_string() :
                rootRecord.name + "/" + relativePath.generic_string();
            asset.name = absolutePath.filename().string();
            asset.type = Classify(absolutePath);
            assets.push_back(std::move(asset));
        }
    }

    std::sort(assets.begin(), assets.end(), [](const AssetRecord& left, const AssetRecord& right) {
        return left.id < right.id;
    });
}

const std::filesystem::path& AssetRegistry::GetContentRoot() const
{
    return contentRoot;
}

const std::vector<AssetRoot>& AssetRegistry::GetRoots() const
{
    return roots;
}

const std::vector<AssetRecord>& AssetRegistry::GetAssets() const
{
    return assets;
}

const AssetRecord* AssetRegistry::FindAssetById(const std::string& id) const
{
    for (const AssetRecord& asset : assets)
    {
        if (asset.id == id)
        {
            return &asset;
        }
    }

    return nullptr;
}

std::vector<const AssetRecord*> AssetRegistry::GetAssetsInDirectory(const std::filesystem::path& directory) const
{
    std::vector<const AssetRecord*> result;
    std::error_code error;
    const std::filesystem::path canonicalDirectory = std::filesystem::weakly_canonical(directory, error);
    const std::filesystem::path compareDirectory = canonicalDirectory.empty() ? directory : canonicalDirectory;

    for (const AssetRecord& asset : assets)
    {
        const std::filesystem::path parent = asset.absolutePath.parent_path();
        const std::filesystem::path canonicalParent = std::filesystem::weakly_canonical(parent, error);
        const std::filesystem::path compareParent = canonicalParent.empty() ? parent : canonicalParent;
        if (compareParent == compareDirectory)
        {
            result.push_back(&asset);
        }
    }

    return result;
}

AssetType AssetRegistry::Classify(const std::filesystem::path& path)
{
    const std::string extension = ToLower(path.extension().string());
    const std::string filename = ToLower(path.filename().string());

    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
        extension == ".bmp" || extension == ".tga")
    {
        return AssetType::Texture;
    }
    if (extension == ".map" || extension == ".tmx" || extension == ".tsx" || extension == ".json")
    {
        return AssetType::Tilemap;
    }
    if (extension == ".lua")
    {
        return AssetType::Script;
    }
    if (extension == ".wav" || extension == ".ogg" || extension == ".mp3")
    {
        return AssetType::Sound;
    }
    if (extension == ".ttf" || extension == ".otf")
    {
        return AssetType::Font;
    }
    if (extension == ".scene" || HasSuffix(filename, ".amber.scene"))
    {
        return AssetType::Scene;
    }

    return AssetType::Unknown;
}

const char* AssetRegistry::TypeName(AssetType type)
{
    switch (type)
    {
        case AssetType::Texture:
            return "Texture";
        case AssetType::Tilemap:
            return "Tilemap";
        case AssetType::Script:
            return "Script";
        case AssetType::Sound:
            return "Sound";
        case AssetType::Font:
            return "Font";
        case AssetType::Scene:
            return "Scene";
        default:
            return "Unknown";
    }
}

bool AssetRegistry::CanInstantiate(AssetType type)
{
    return type == AssetType::Texture || type == AssetType::Tilemap;
}

}
