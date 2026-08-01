#ifndef PHYSICS_DEMO_SAMPLE_PATHS_H
#define PHYSICS_DEMO_SAMPLE_PATHS_H

#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

inline std::string NormalizePhysicsDemoAssetName(const std::string& assetPath)
{
	std::string normalized = assetPath;
	for (char& character : normalized)
	{
		if (character == '\\')
		{
			character = '/';
		}
	}

	const std::string parentContentPrefix = "../Content/";
	if (normalized.rfind(parentContentPrefix, 0) == 0)
	{
		return normalized.substr(parentContentPrefix.size());
	}

	const std::string contentPrefix = "Content/";
	if (normalized.rfind(contentPrefix, 0) == 0)
	{
		return normalized.substr(contentPrefix.size());
	}

	const std::string legacyParentAssetsPrefix = "../assets/";
	if (normalized.rfind(legacyParentAssetsPrefix, 0) == 0)
	{
		return normalized.substr(legacyParentAssetsPrefix.size());
	}

	const std::string legacyAssetsPrefix = "assets/";
	if (normalized.rfind(legacyAssetsPrefix, 0) == 0)
	{
		return normalized.substr(legacyAssetsPrefix.size());
	}

	return normalized;
}

inline std::string ResolvePhysicsDemoAssetPath(const std::string& assetPath)
{
	namespace fs = std::filesystem;

	std::error_code errorCode;
	if (fs::exists(assetPath, errorCode))
	{
		return assetPath;
	}

	const fs::path relativeAssetPath = NormalizePhysicsDemoAssetName(assetPath);
	const fs::path currentPath = fs::current_path(errorCode);
	if (errorCode)
	{
		return assetPath;
	}

	const std::vector<fs::path> assetRoots = {
		currentPath / "Samples" / "PhysicsDemos" / "Content",
		currentPath / "Content",
		currentPath / ".." / "Content",
		currentPath / ".." / ".." / "Samples" / "PhysicsDemos" / "Content",
		currentPath / ".." / ".." / ".." / "Samples" / "PhysicsDemos" / "Content"};

	for (const fs::path& assetRoot : assetRoots)
	{
		const fs::path candidate = assetRoot / relativeAssetPath;
		if (fs::exists(candidate, errorCode))
		{
			return candidate.string();
		}
	}

	return assetPath;
}

#endif
