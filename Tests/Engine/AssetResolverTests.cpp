#include "Assets/AssetResolver.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

namespace
{
    class TempAssetTree
    {
    public:
        TempAssetTree()
        {
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            root = std::filesystem::temp_directory_path() / ("AmberAssetResolverTests_" + std::to_string(stamp));
            std::filesystem::create_directories(root);
        }

        ~TempAssetTree()
        {
            std::error_code error;
            std::filesystem::remove_all(root, error);
        }

        std::filesystem::path CreateFile(const std::filesystem::path& relativePath)
        {
            const std::filesystem::path path = root / relativePath;
            std::filesystem::create_directories(path.parent_path());
            std::ofstream file(path);
            file << "asset";
            return path;
        }

        std::filesystem::path Canonical(const std::filesystem::path& path) const
        {
            std::error_code error;
            const std::filesystem::path canonical = std::filesystem::weakly_canonical(path, error);
            return canonical.empty() ? path : canonical;
        }

        std::filesystem::path root;
    };
}

TEST(AssetResolverTests, BuildsStableAssetIdsFromRootAndRelativePath)
{
    EXPECT_EQ(AE::MakeRuntimeAssetId("Project", std::filesystem::path("images") / "player.png"), "Project/images/player.png");
    EXPECT_EQ(AE::MakeRuntimeAssetId("", std::filesystem::path("images") / "player.png"), "images/player.png");
}

TEST(AssetResolverTests, ResolvesProjectEngineRelativeAndAbsoluteAssets)
{
    TempAssetTree tree;
    const std::filesystem::path projectRoot = tree.root / "Project";
    const std::filesystem::path engineRoot = tree.root / "AmberEngine";
    const std::filesystem::path projectTexture = tree.CreateFile("Project/Content/images/player.png");
    const std::filesystem::path engineTexture = tree.CreateFile("AmberEngine/Engine/Content/icons/default.png");
    const std::filesystem::path looseAsset = tree.CreateFile("Project/Content/loose.txt");

    AE::RuntimeAssetResolverConfig config;
    config.ProjectRoot = projectRoot;
    config.EngineRoot = engineRoot;
    config.ContentRoot = "Content";

    EXPECT_EQ(AE::ResolveRuntimeAssetPath("Project/images/player.png", config), tree.Canonical(projectTexture));
    EXPECT_EQ(AE::ResolveRuntimeAssetPath("Engine/icons/default.png", config), tree.Canonical(engineTexture));
    EXPECT_EQ(AE::ResolveRuntimeAssetPath("loose.txt", config), tree.Canonical(looseAsset));
    EXPECT_EQ(AE::ResolveRuntimeAssetPath(projectTexture.string(), config), tree.Canonical(projectTexture));
}

TEST(AssetResolverTests, UsesExplicitRootsWhenProvided)
{
    TempAssetTree tree;
    const std::filesystem::path firstTexture = tree.CreateFile("First/shared.png");
    const std::filesystem::path secondTexture = tree.CreateFile("Second/shared.png");

    AE::RuntimeAssetResolverConfig config;
    config.Roots = {
        AE::RuntimeAssetRoot{"Project", tree.root / "First"},
        AE::RuntimeAssetRoot{"Engine", tree.root / "Second"}
    };

    EXPECT_EQ(AE::ResolveRuntimeAssetPath("Project/shared.png", config), tree.Canonical(firstTexture));
    EXPECT_EQ(AE::ResolveRuntimeAssetPath("Engine/shared.png", config), tree.Canonical(secondTexture));
}
