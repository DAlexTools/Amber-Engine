#ifndef AMBER_RUNTIME_ASSETS_ASSET_RESOLVER_H
#define AMBER_RUNTIME_ASSETS_ASSET_RESOLVER_H

#include <filesystem>
#include <string>
#include <vector>

namespace AE
{

struct RuntimeAssetRoot
{
    std::string name;
    std::filesystem::path path;
};

struct RuntimeAssetResolverConfig
{
    std::filesystem::path projectRoot;
    std::filesystem::path engineRoot;
    std::filesystem::path contentRoot;
    std::vector<RuntimeAssetRoot> roots;
};

std::filesystem::path ResolveProjectContentRoot(
    const std::filesystem::path& projectRoot,
    const std::filesystem::path& contentRoot);

std::filesystem::path ResolveEngineContentRoot(const std::filesystem::path& engineRoot);

std::vector<RuntimeAssetRoot> BuildRuntimeAssetRoots(const RuntimeAssetResolverConfig& config);

std::string MakeRuntimeAssetId(const std::string& rootName, const std::filesystem::path& relativePath);

std::filesystem::path ResolveRuntimeAssetPath(
    const std::string& assetId,
    const RuntimeAssetResolverConfig& config);

std::filesystem::path ResolveRuntimeAssetPath(
    const std::string& assetId,
    const std::vector<RuntimeAssetRoot>& roots);

}

#endif
