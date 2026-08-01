#ifndef AMBER_RUNTIME_ASSETS_ASSET_RESOLVER_H
#define AMBER_RUNTIME_ASSETS_ASSET_RESOLVER_H

#include <filesystem>
#include <string>
#include <vector>

namespace AE
{

/**
 * @brief Represents a named asset root directory.
 *
 * Associates a logical root name (e.g. "Project" or "Engine")
 * with its corresponding filesystem path.
 */
struct RuntimeAssetRoot
{
	/** Logical name of the asset root. */
	std::string Name;

	/** Absolute path to the asset root directory. */
	std::filesystem::path Path;
};

/**
 * @brief Configuration used for runtime asset path resolution.
 *
 * Specifies the project and engine locations along with the
 * collection of available asset roots.
 */
struct RuntimeAssetResolverConfig
{
	/** Root directory of the current project. */
	std::filesystem::path ProjectRoot;

	/** Root directory of the engine installation. */
	std::filesystem::path EngineRoot;

	/** Project content directory. */
	std::filesystem::path ContentRoot;

	/** Available asset roots used during asset resolution. */
	std::vector<RuntimeAssetRoot> Roots;
};

/**
 * @brief Resolves the project's content directory.
 *
 * Returns an absolute content directory path based on the specified
 * project root and optional content root override.
 *
 * @param ProjectRoot Root directory of the project.
 * @param ContentRoot Content directory or relative override.
 *
 * @return Absolute path to the project's content directory.
 */
std::filesystem::path ResolveProjectContentRoot(const std::filesystem::path& ProjectRoot, const std::filesystem::path& ContentRoot);

/**
 * @brief Resolves the engine's content directory.
 *
 * Attempts to locate the Engine/Content directory for the
 * specified engine installation.
 *
 * @param EngineRoot Root directory of the engine installation.
 *
 * @return Absolute path to the engine content directory.
 */
std::filesystem::path ResolveEngineContentRoot(const std::filesystem::path& EngineRoot);

/**
 * @brief Builds the list of runtime asset roots.
 *
 * Creates logical asset roots based on the supplied resolver
 * configuration.
 *
 * @param Config Asset resolver configuration.
 *
 * @return Collection of runtime asset roots.
 */
std::vector<RuntimeAssetRoot> BuildRuntimeAssetRoots(const RuntimeAssetResolverConfig& Config);

/**
 * @brief Creates a runtime asset identifier.
 *
 * Combines a root name and a relative asset path into a single
 * runtime asset identifier.
 *
 * @param RootName Logical asset root name.
 * @param RelativePath Asset path relative to the root.
 *
 * @return Runtime asset identifier.
 */
std::string MakeRuntimeAssetId(const std::string& RootName, const std::filesystem::path& RelativePath);

/**
 * @brief Resolves the filesystem path for a runtime asset.
 *
 * Searches for the specified asset using the supplied resolver
 * configuration.
 *
 * @param AssetId Runtime asset identifier.
 * @param Config Asset resolver configuration.
 *
 * @return Absolute asset path if found; otherwise an empty path.
 */
std::filesystem::path ResolveRuntimeAssetPath(const std::string& AssetId, const RuntimeAssetResolverConfig& Config);

/**
 * @brief Resolves the filesystem path for a runtime asset.
 *
 * Searches for the specified asset using the provided runtime
 * asset roots.
 *
 * @param AssetId Runtime asset identifier.
 * @param Roots Collection of runtime asset roots.
 *
 * @return Absolute asset path if found; otherwise an empty path.
 */
std::filesystem::path ResolveRuntimeAssetPath(const std::string& AssetId, const std::vector<RuntimeAssetRoot>& Roots);

} // namespace AE

#endif
