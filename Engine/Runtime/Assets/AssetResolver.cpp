#include "Assets/AssetResolver.h"
#include <system_error>

namespace AE
{
namespace
{
bool StartsWith(const std::string& Value, const std::string& Prefix)
{
	return Value.size() >= Prefix.size() && Value.compare(0, Prefix.size(), Prefix) == 0;
}

bool FileExists(const std::filesystem::path& Path)
{
	std::error_code Error;
	return std::filesystem::exists(Path, Error) && std::filesystem::is_regular_file(Path, Error);
}

bool DirectoryExists(const std::filesystem::path& Path)
{
	std::error_code Error;
	return std::filesystem::exists(Path, Error) && std::filesystem::is_directory(Path, Error);
}

std::filesystem::path WeakCanonicalIfPossible(const std::filesystem::path& Path)
{
	std::error_code Error;
	const std::filesystem::path Canonical = std::filesystem::weakly_canonical(Path, Error);
	return Canonical.empty() ? Path : Canonical;
}

void AddRootCandidate(std::vector<std::filesystem::path>& Candidates, const RuntimeAssetRoot& Root, const std::string& AssetId)
{
	if (Root.Name.empty() || Root.Path.empty())
	{
		return;
	}

	const std::string Prefix = Root.Name + "/";
	if (StartsWith(AssetId, Prefix))
	{
		Candidates.push_back(Root.Path / AssetId.substr(Prefix.size()));
	}
}
} // namespace

std::filesystem::path ResolveProjectContentRoot(const std::filesystem::path& ProjectRoot, const std::filesystem::path& ContentRoot)
{
	if (ContentRoot.empty())
	{
		return ProjectRoot.empty() ? std::filesystem::path{} : ProjectRoot / "Content";
	}
	if (ContentRoot.is_absolute())
	{
		return ContentRoot;
	}
	return ProjectRoot.empty() ? ContentRoot : ProjectRoot / ContentRoot;
}

std::filesystem::path ResolveEngineContentRoot(const std::filesystem::path& EngineRoot)
{
	if (EngineRoot.empty())
	{
		return {};
	}

	const std::filesystem::path Nested = EngineRoot / "Engine" / "Content";
	if (DirectoryExists(Nested))
	{
		return Nested;
	}

	const std::filesystem::path Direct = EngineRoot / "Content";
	if (DirectoryExists(Direct))
	{
		return Direct;
	}

	return Nested;
}

std::vector<RuntimeAssetRoot> BuildRuntimeAssetRoots(const RuntimeAssetResolverConfig& Config)
{
	std::vector<RuntimeAssetRoot> Roots;
	const std::filesystem::path ProjectContentRoot = ResolveProjectContentRoot(Config.ProjectRoot, Config.ContentRoot);
	if (!ProjectContentRoot.empty())
	{
		Roots.push_back(RuntimeAssetRoot{"Project", ProjectContentRoot});
	}

	const std::filesystem::path EngineContentRoot = ResolveEngineContentRoot(Config.EngineRoot);
	if (!EngineContentRoot.empty())
	{
		Roots.push_back(RuntimeAssetRoot{"Engine", EngineContentRoot});
	}

	return Roots;
}

std::string MakeRuntimeAssetId(const std::string& RootName, const std::filesystem::path& RelativePath)
{
	const std::string Relative = RelativePath.generic_string();
	if (RootName.empty())
	{
		return Relative;
	}
	if (Relative.empty() || Relative == ".")
	{
		return RootName;
	}
	return RootName + "/" + Relative;
}

std::filesystem::path ResolveRuntimeAssetPath(const std::string& AssetId, const RuntimeAssetResolverConfig& Config)
{
	if (AssetId.empty())
	{
		return {};
	}

	RuntimeAssetResolverConfig ResolvedConfig = Config;
	if (ResolvedConfig.Roots.empty())
	{
		ResolvedConfig.Roots = BuildRuntimeAssetRoots(Config);
	}

	const std::filesystem::path AssetPath(AssetId);
	std::vector<std::filesystem::path> Candidates;
	if (AssetPath.is_absolute())
	{
		Candidates.push_back(AssetPath);
	}
	else
	{
		for (const RuntimeAssetRoot& Root : ResolvedConfig.Roots)
		{
			AddRootCandidate(Candidates, Root, AssetId);
		}

		for (const RuntimeAssetRoot& Root : ResolvedConfig.Roots)
		{
			if (!Root.Path.empty())
			{
				Candidates.push_back(Root.Path / AssetPath);
			}
		}

		if (!ResolvedConfig.ProjectRoot.empty())
		{
			Candidates.push_back(ResolvedConfig.ProjectRoot / AssetPath);
		}
		if (!ResolvedConfig.EngineRoot.empty())
		{
			Candidates.push_back(ResolvedConfig.EngineRoot / AssetPath);
		}
	}

	for (const std::filesystem::path& Candidate : Candidates)
	{
		if (FileExists(Candidate))
		{
			return WeakCanonicalIfPossible(Candidate);
		}
	}

	return {};
}

std::filesystem::path ResolveRuntimeAssetPath(const std::string& AssetId, const std::vector<RuntimeAssetRoot>& Roots)
{
	RuntimeAssetResolverConfig Config;
	Config.Roots = Roots;
	return ResolveRuntimeAssetPath(AssetId, Config);
}

} // namespace AE
