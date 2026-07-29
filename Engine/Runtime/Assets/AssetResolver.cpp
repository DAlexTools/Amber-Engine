#include "Assets/AssetResolver.h"

#include <system_error>

namespace AE
{
namespace
{
    bool StartsWith(const std::string& value, const std::string& prefix)
    {
        return value.size() >= prefix.size() &&
            value.compare(0, prefix.size(), prefix) == 0;
    }

    bool FileExists(const std::filesystem::path& path)
    {
        std::error_code error;
        return std::filesystem::exists(path, error) && std::filesystem::is_regular_file(path, error);
    }

    bool DirectoryExists(const std::filesystem::path& path)
    {
        std::error_code error;
        return std::filesystem::exists(path, error) && std::filesystem::is_directory(path, error);
    }

    std::filesystem::path WeakCanonicalIfPossible(const std::filesystem::path& path)
    {
        std::error_code error;
        const std::filesystem::path canonical = std::filesystem::weakly_canonical(path, error);
        return canonical.empty() ? path : canonical;
    }

    void AddRootCandidate(
        std::vector<std::filesystem::path>& candidates,
        const RuntimeAssetRoot& root,
        const std::string& assetId)
    {
        if (root.name.empty() || root.path.empty())
        {
            return;
        }

        const std::string prefix = root.name + "/";
        if (StartsWith(assetId, prefix))
        {
            candidates.push_back(root.path / assetId.substr(prefix.size()));
        }
    }
}

std::filesystem::path ResolveProjectContentRoot(
    const std::filesystem::path& projectRoot,
    const std::filesystem::path& contentRoot)
{
    if (contentRoot.empty())
    {
        return projectRoot.empty() ? std::filesystem::path{} : projectRoot / "Content";
    }
    if (contentRoot.is_absolute())
    {
        return contentRoot;
    }
    return projectRoot.empty() ? contentRoot : projectRoot / contentRoot;
}

std::filesystem::path ResolveEngineContentRoot(const std::filesystem::path& engineRoot)
{
    if (engineRoot.empty())
    {
        return {};
    }

    const std::filesystem::path nested = engineRoot / "Engine" / "Content";
    if (DirectoryExists(nested))
    {
        return nested;
    }

    const std::filesystem::path direct = engineRoot / "Content";
    if (DirectoryExists(direct))
    {
        return direct;
    }

    return nested;
}

std::vector<RuntimeAssetRoot> BuildRuntimeAssetRoots(const RuntimeAssetResolverConfig& config)
{
    std::vector<RuntimeAssetRoot> roots;

    const std::filesystem::path projectContentRoot = ResolveProjectContentRoot(config.projectRoot, config.contentRoot);
    if (!projectContentRoot.empty())
    {
        roots.push_back(RuntimeAssetRoot{"Project", projectContentRoot});
    }

    const std::filesystem::path engineContentRoot = ResolveEngineContentRoot(config.engineRoot);
    if (!engineContentRoot.empty())
    {
        roots.push_back(RuntimeAssetRoot{"Engine", engineContentRoot});
    }

    return roots;
}

std::string MakeRuntimeAssetId(const std::string& rootName, const std::filesystem::path& relativePath)
{
    const std::string relative = relativePath.generic_string();
    if (rootName.empty())
    {
        return relative;
    }
    if (relative.empty() || relative == ".")
    {
        return rootName;
    }
    return rootName + "/" + relative;
}

std::filesystem::path ResolveRuntimeAssetPath(
    const std::string& assetId,
    const RuntimeAssetResolverConfig& config)
{
    if (assetId.empty())
    {
        return {};
    }

    RuntimeAssetResolverConfig resolvedConfig = config;
    if (resolvedConfig.roots.empty())
    {
        resolvedConfig.roots = BuildRuntimeAssetRoots(config);
    }

    const std::filesystem::path assetPath(assetId);
    std::vector<std::filesystem::path> candidates;
    if (assetPath.is_absolute())
    {
        candidates.push_back(assetPath);
    }
    else
    {
        for (const RuntimeAssetRoot& root : resolvedConfig.roots)
        {
            AddRootCandidate(candidates, root, assetId);
        }

        for (const RuntimeAssetRoot& root : resolvedConfig.roots)
        {
            if (!root.path.empty())
            {
                candidates.push_back(root.path / assetPath);
            }
        }

        if (!resolvedConfig.projectRoot.empty())
        {
            candidates.push_back(resolvedConfig.projectRoot / assetPath);
        }
        if (!resolvedConfig.engineRoot.empty())
        {
            candidates.push_back(resolvedConfig.engineRoot / assetPath);
        }
    }

    for (const std::filesystem::path& candidate : candidates)
    {
        if (FileExists(candidate))
        {
            return WeakCanonicalIfPossible(candidate);
        }
    }

    return {};
}

std::filesystem::path ResolveRuntimeAssetPath(
    const std::string& assetId,
    const std::vector<RuntimeAssetRoot>& roots)
{
    RuntimeAssetResolverConfig config;
    config.roots = roots;
    return ResolveRuntimeAssetPath(assetId, config);
}

}
