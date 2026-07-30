#include "ActorTypeRegistry.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace
{

std::filesystem::path SourceRoot()
{
    return std::filesystem::path(AMBER_TEST_SOURCE_ROOT);
}

std::filesystem::path PlatformerActorTypesPath()
{
    return SourceRoot() / "Projects" / "Platformer" / "Config" / "ActorTypes.amberactors";
}

TEST(ActorTypeRegistryTests, LoadsProjectActorTypesWithComponentSchemas)
{
    AE::Editor::FActorTypeRegistry Registry;
    AE::Editor::RegisterDefaultActorTypes(Registry);
    std::string Error;
    ASSERT_TRUE(AE::Editor::LoadActorTypesFromFile(PlatformerActorTypesPath(), Registry, &Error)) << Error;

    const AE::Editor::FActorTypeDefinition* PlayerSpawn = Registry.FindByClassName("PlayerSpawnObject");
    ASSERT_NE(PlayerSpawn, nullptr);
    EXPECT_EQ(PlayerSpawn->DisplayName, "Player Spawn");
    EXPECT_EQ(PlayerSpawn->Kind, AE::Editor::SceneObjectKind::Box);
    ASSERT_EQ(PlayerSpawn->Components.size(), 1u);
    EXPECT_EQ(PlayerSpawn->Components.front().Name, "FPlayerSpawnComponent");

    const AE::Editor::FActorTypeDefinition* EnemySpawn = Registry.FindByTypeId("Platformer.EnemySpawn");
    ASSERT_NE(EnemySpawn, nullptr);
    ASSERT_EQ(EnemySpawn->Components.size(), 1u);
    EXPECT_EQ(EnemySpawn->Components.front().Name, "FEnemySpawnComponent");
    EXPECT_EQ(EnemySpawn->Components.front().Properties.size(), 11u);
}

TEST(ActorTypeRegistryTests, TracksManagedComponentsForActorClasses)
{
    AE::Editor::FActorTypeRegistry Registry;
    std::string Error;
    ASSERT_TRUE(AE::Editor::LoadActorTypesFromFile(PlatformerActorTypesPath(), Registry, &Error)) << Error;

    EXPECT_TRUE(Registry.IsManagedComponentName("FEnemySpawnComponent"));
    EXPECT_TRUE(Registry.IsComponentExpectedForClass("EnemySpawnObject", "FEnemySpawnComponent"));
    EXPECT_FALSE(Registry.IsComponentExpectedForClass("CoinObject", "FEnemySpawnComponent"));
}

} // namespace
