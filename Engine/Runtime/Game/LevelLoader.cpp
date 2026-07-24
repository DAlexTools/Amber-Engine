#include "LevelLoader.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "../Components/TransformComponent.h"
#include "../Components/AnimationComponent.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/CameraFollowComponent.h"
#include "../Components/HealthComponent.h"
#include "../Components/KeyboardControlledComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/ProjectileEmitterComponent.h"
#include "../Components/TextRenderComponent.h"
#include "../EnginePhysicsBridge/EnginePhysicsBridge.h"
#include "../Logging/Logger.h"
#include "../Systems/PhysicsContactResponseSystem.h"
#include "../Classes/Engine.h"

namespace
{
    float ReadFloat(sol::table table, const char* key, float defaultValue)
    {
        return static_cast<float>(table[key].get_or(defaultValue));
    }

    bool ReadBool(sol::table table, const char* key, bool defaultValue)
    {
        return table[key].get_or(defaultValue);
    }

    std::uint32_t ReadCollisionLayerValue(const sol::object& value, std::uint32_t defaultValue)
    {
        if (!value.valid() || value.get_type() == sol::type::lua_nil)
        {
            return defaultValue;
        }

        if (value.is<std::string>())
        {
            return EnginePhysicsCollision::FromName(value.as<std::string>(), defaultValue);
        }

        if (value.is<int>())
        {
            return static_cast<std::uint32_t>(value.as<int>());
        }

        if (value.is<double>())
        {
            return static_cast<std::uint32_t>(value.as<double>());
        }

        return defaultValue;
    }

    std::uint32_t ReadCollisionLayer(sol::table table, const char* key, const char* fallbackKey, std::uint32_t defaultValue)
    {
        sol::object value = table.get<sol::object>(key);
        if (value.valid() && value.get_type() != sol::type::lua_nil)
        {
            return ReadCollisionLayerValue(value, defaultValue);
        }

        sol::object fallbackValue = table.get<sol::object>(fallbackKey);
        return ReadCollisionLayerValue(fallbackValue, defaultValue);
    }

    std::uint32_t DefaultPhysicsCollisionCategory(const std::string& entityTag, const std::string& entityGroup)
    {
        if (entityTag == "player")
        {
            return EnginePhysicsCollision::Player;
        }
        if (entityGroup == "enemies")
        {
            return EnginePhysicsCollision::Enemy;
        }
        if (entityGroup == "obstacles")
        {
            return EnginePhysicsCollision::Obstacle;
        }

        return EnginePhysicsCollision::Player;
    }

    std::uint32_t DefaultPhysicsCollisionMask(const std::string& entityTag, const std::string& entityGroup)
    {
        if (entityTag == "player")
        {
            return EnginePhysicsCollision::EnemyProjectile | EnginePhysicsCollision::Obstacle;
        }
        if (entityGroup == "enemies")
        {
            return EnginePhysicsCollision::PlayerProjectile | EnginePhysicsCollision::Obstacle;
        }
        if (entityGroup == "obstacles")
        {
            return EnginePhysicsCollision::All;
        }

        return EnginePhysicsCollision::All;
    }

    glm::vec2 ReadVec2(sol::table table, const char* key, glm::vec2 defaultValue)
    {
        sol::optional<sol::table> value = table[key];
        if (value == sol::nullopt)
        {
            return defaultValue;
        }

        sol::table vector = value.value();
        return glm::vec2(
            ReadFloat(vector, "x", defaultValue.x),
            ReadFloat(vector, "y", defaultValue.y));
    }

    int ReadTileId(const std::string& tileCode)
    {
        try
        {
            return std::stoi(tileCode);
        }
        catch (...)
        {
            return 0;
        }
    }

    std::vector<int> ReadTileIds(sol::table table, const char* key)
    {
        std::vector<int> tileIds;
        sol::optional<sol::table> values = table[key];
        if (values == sol::nullopt)
        {
            return tileIds;
        }

        for (auto&& tileEntry : values.value())
        {
            const sol::object value = tileEntry.second;
            if (value.is<int>())
            {
                tileIds.push_back(value.as<int>());
            }
            else if (value.is<double>())
            {
                tileIds.push_back(static_cast<int>(value.as<double>()));
            }
            else if (value.is<std::string>())
            {
                tileIds.push_back(ReadTileId(value.as<std::string>()));
            }
        }

        return tileIds;
    }

    std::vector<int> ReadSolidTileIds(sol::table table)
    {
        std::vector<int> tileIds = ReadTileIds(table, "solid_tiles");
        if (tileIds.empty())
        {
            tileIds = ReadTileIds(table, "solid_tile_ids");
        }
        if (tileIds.empty())
        {
            tileIds = ReadTileIds(table, "tiles");
        }
        return tileIds;
    }

    bool ContainsTileId(const std::vector<int>& tileIds, int tileId)
    {
        return std::find(tileIds.begin(), tileIds.end(), tileId) != tileIds.end();
    }

    PhysicsContactAction ReadContactAction(sol::table table)
    {
        std::string actionName = EnginePhysicsCollision::NormalizeLayerName(
            table["action"].get_or(std::string("bounce")));

        if (actionName != "bounce")
        {
            AE::Logger::Warn("Unknown physics contact action '" + actionName + "', using bounce.");
        }

        return PhysicsContactAction::Bounce;
    }

    PhysicsContactResponder ReadContactResponder(sol::table table)
    {
        const std::string responder = EnginePhysicsCollision::NormalizeLayerName(
            table["responder"].get_or(std::string("first")));

        if (responder == "second" || responder == "b")
        {
            return PhysicsContactResponder::Second;
        }

        if (responder == "both")
        {
            return PhysicsContactResponder::Both;
        }

        return PhysicsContactResponder::First;
    }

    void LoadPhysicsContactPolicies(sol::table level, const std::unique_ptr<Registry>& registry)
    {
        if (!registry->HasSystem<PhysicsContactResponseSystem>())
        {
            return;
        }

        sol::optional<sol::table> physicsConfig = level["physics"];
        if (physicsConfig == sol::nullopt)
        {
            return;
        }

        sol::optional<sol::table> policyConfigs = physicsConfig.value()["contact_policies"];
        if (policyConfigs == sol::nullopt)
        {
            return;
        }

        auto& responseSystem = registry->GetSystem<PhysicsContactResponseSystem>();
        responseSystem.ClearPolicies();

        int policiesLoaded = 0;
        sol::table policies = policyConfigs.value();
        int i = 0;
        while (true)
        {
            sol::optional<sol::table> policyConfig = policies[i];
            if (policyConfig == sol::nullopt)
            {
                break;
            }

            sol::table policyTable = policyConfig.value();
            PhysicsContactPolicy policy;
            policy.firstLayerMask = ReadCollisionLayer(
                policyTable,
                "first",
                "a",
                EnginePhysicsCollision::All);
            policy.secondLayerMask = ReadCollisionLayer(
                policyTable,
                "second",
                "b",
                EnginePhysicsCollision::All);
            policy.action = ReadContactAction(policyTable);
            policy.responder = ReadContactResponder(policyTable);
            policy.bidirectional = ReadBool(policyTable, "bidirectional", true);
            policy.name = policyTable["name"].get_or(std::string("contact_policy_") + std::to_string(i));

            responseSystem.AddPolicy(policy);
            ++policiesLoaded;
            ++i;
        }

        AE::Logger::Log("Loaded " + std::to_string(policiesLoaded) + " physics contact response policies.");
    }

    void CreateStaticTilePhysicsBody(
        const std::unique_ptr<Registry>& registry,
        PhysicsWorldSystem& physicsWorldSystem,
        float x,
        float y,
        float width,
        float height,
        std::uint32_t collisionCategory,
        std::uint32_t collisionMask,
        bool isSensor)
    {
        Entity tileCollision = registry->CreateEntity();
        tileCollision.Group("obstacles");
        tileCollision.AddComponent<TransformComponent>(glm::vec2(x, y), glm::vec2(1.0f, 1.0f), 0.0);
        tileCollision.AddComponent<BoxCollisionComponent>(
            static_cast<int>(width),
            static_cast<int>(height));

        PhysicsBodyDefinition bodyDefinition;
        bodyDefinition.mass = 0.0f;
        bodyDefinition.width = width;
        bodyDefinition.height = height;
        bodyDefinition.pullPositionFromPhysics = false;
        bodyDefinition.pullRotationFromPhysics = false;
        bodyDefinition.collisionCategory = collisionCategory;
        bodyDefinition.collisionMask = collisionMask;
        bodyDefinition.isSensor = isSensor;

        tileCollision.AddComponent<PhysicsBodyComponent>(
            PhysicsBodyFactory::Create(
                physicsWorldSystem,
                tileCollision.GetComponent<TransformComponent>(),
                bodyDefinition));
    }
}


LevelLoader::LevelLoader()
{
    AE::Logger::Log("Level Loader constructor");
}

LevelLoader::~LevelLoader()
{
    AE::Logger::Log("Level Loader destructor");
}

void LevelLoader::LoadLevel(sol::state& lua, const std::unique_ptr<Registry>& registry, const std::unique_ptr<AssetManager>& assetStore, SDL_Renderer* renderer, int levelNumber) {
    const std::string levelScriptPath = "./Content/scripts/Level" + std::to_string(levelNumber) + ".lua";

    // This checks the syntax of our script, but it does not execute the script
    sol::load_result script = lua.load_file(levelScriptPath);
    if (!script.valid()) {
        sol::error err = script;
        std::string errorMessage = err.what();
        AE::Logger::Err("Error loading the lua script: " + errorMessage);
        return;
    }

    // Executes the script using the Sol state
    lua.script_file(levelScriptPath);

    // Read the big table for the current level
    sol::table level = lua["Level"];
    LoadPhysicsContactPolicies(level, registry);

    ////////////////////////////////////////////////////////////////////////////
    // Read the level assets
    ////////////////////////////////////////////////////////////////////////////
    sol::table assets = level["assets"];

    int i = 0;
    while (true) {
        sol::optional<sol::table> hasAsset = assets[i];
        if (hasAsset == sol::nullopt) {
            break;
        }
        sol::table asset = assets[i];
        std::string assetType = asset["type"];
        std::string assetId = asset["id"];
        if (assetType == "texture") {
            assetStore->AddTexture(renderer, assetId, asset["file"]);
            AE::Logger::Log("A new texture asset was added to the asset store, id: " + assetId);
        }
        if (assetType == "font") {
            assetStore->AddFont(assetId, asset["file"], asset["font_size"]);
            AE::Logger::Log("A new font asset was added to the asset store, id: " + assetId);
        }
        i++;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Read the level tilemap information
    ////////////////////////////////////////////////////////////////////////////
    sol::table map = level["tilemap"];
    std::string mapFilePath = map["map_file"];
    std::string mapTextureAssetId = map["texture_asset_id"];
    int mapNumRows = map["num_rows"];
    int mapNumCols = map["num_cols"];
    int tileSize = map["tile_size"];
    double mapScale = map["scale"];
    sol::optional<sol::table> tilePhysicsBody = map["physics_body"];
    if (tilePhysicsBody == sol::nullopt) {
        sol::optional<sol::table> tileCollision = map["collision"];
        tilePhysicsBody = tileCollision;
    }

    std::vector<std::vector<int>> tileIds(
        mapNumRows,
        std::vector<int>(mapNumCols, 0));
    std::fstream mapFile;
    mapFile.open(mapFilePath);
    for (int y = 0; y < mapNumRows; y++) {
        std::string mapLine;
        std::getline(mapFile, mapLine);
        std::stringstream lineStream(mapLine);

        for (int x = 0; x < mapNumCols; x++) {
            std::string tileCode;
            if (!std::getline(lineStream, tileCode, ',')) {
                tileCode = "00";
            }

            const int tileId = ReadTileId(tileCode);
            tileIds[y][x] = tileId;
            int srcRectY = (tileId / 10) * tileSize;
            int srcRectX = (tileId % 10) * tileSize;

            Entity tile = registry->CreateEntity();
            tile.AddComponent<TransformComponent>(glm::vec2(x * (mapScale * tileSize), y * (mapScale * tileSize)), glm::vec2(mapScale, mapScale), 0.0);
            tile.AddComponent<SpriteComponent>(mapTextureAssetId, tileSize, tileSize, 0, false, srcRectX, srcRectY);
        }
    }
    mapFile.close();
    AE::Engine::MapWidth = mapNumCols * tileSize * mapScale;
    AE::Engine::MapHeight = mapNumRows * tileSize * mapScale;

    if (tilePhysicsBody != sol::nullopt && ReadBool(tilePhysicsBody.value(), "enabled", true)) {
        if (!registry->HasSystem<PhysicsWorldSystem>()) {
            AE::Logger::Warn("tilemap physics_body skipped: PhysicsWorldSystem is not registered");
        }
        else {
            sol::table tilePhysicsConfig = tilePhysicsBody.value();
            const std::vector<int> solidTileIds = ReadSolidTileIds(tilePhysicsConfig);

            if (solidTileIds.empty()) {
                AE::Logger::Warn("tilemap physics_body skipped: no solid_tiles were configured");
            }
            else {
                auto& physicsWorldSystem = registry->GetSystem<PhysicsWorldSystem>();
                const float tileWorldSize = static_cast<float>(tileSize * mapScale);
                const std::uint32_t collisionCategory = ReadCollisionLayer(
                    tilePhysicsConfig,
                    "collision_category",
                    "category",
                    EnginePhysicsCollision::Obstacle);
                const std::uint32_t collisionMask = ReadCollisionLayer(
                    tilePhysicsConfig,
                    "collision_mask",
                    "mask",
                    EnginePhysicsCollision::All);
                const bool isSensor = ReadBool(
                    tilePhysicsConfig,
                    "sensor",
                    ReadBool(tilePhysicsConfig, "is_sensor", false));
                int staticTileBodiesCreated = 0;

                for (int y = 0; y < mapNumRows; ++y) {
                    int runStart = -1;

                    for (int x = 0; x <= mapNumCols; ++x) {
                        const bool isSolidTile =
                            x < mapNumCols &&
                            ContainsTileId(solidTileIds, tileIds[y][x]);

                        if (isSolidTile && runStart < 0) {
                            runStart = x;
                        }

                        if ((!isSolidTile || x == mapNumCols) && runStart >= 0) {
                            const int runLength = x - runStart;
                            CreateStaticTilePhysicsBody(
                                registry,
                                physicsWorldSystem,
                                runStart * tileWorldSize,
                                y * tileWorldSize,
                                runLength * tileWorldSize,
                                tileWorldSize,
                                collisionCategory,
                                collisionMask,
                                isSensor);
                            ++staticTileBodiesCreated;
                            runStart = -1;
                        }
                    }
                }

                AE::Logger::Log("Created " + std::to_string(staticTileBodiesCreated) + " static tilemap physics bodies.");
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Read the level entities and their components
    ////////////////////////////////////////////////////////////////////////////
    sol::table entities = level["entities"];
    i = 0;
    while (true) {
        sol::optional<sol::table> hasEntity = entities[i];
        if (hasEntity == sol::nullopt) {
            break;
        }

        sol::table entity = entities[i];

        Entity newEntity = registry->CreateEntity();

        // Tag
        sol::optional<std::string> tag = entity["tag"];
        if (tag != sol::nullopt) {
            newEntity.Tag(entity["tag"]);
        }
        const std::string entityTag = tag != sol::nullopt ? tag.value() : std::string("");

        // Group
        sol::optional<std::string> group = entity["group"];
        if (group != sol::nullopt) {
            newEntity.Group(entity["group"]);
        }
        const std::string entityGroup = group != sol::nullopt ? group.value() : std::string("");

        // Components
        sol::optional<sol::table> hasComponents = entity["components"];
        if (hasComponents != sol::nullopt) {
            // Transform
            sol::optional<sol::table> transform = entity["components"]["transform"];
            if (transform != sol::nullopt) {
                newEntity.AddComponent<TransformComponent>(
                    glm::vec2(
                        entity["components"]["transform"]["position"]["x"],
                        entity["components"]["transform"]["position"]["y"]
                    ),
                    glm::vec2(
                        entity["components"]["transform"]["scale"]["x"].get_or(1.0),
                        entity["components"]["transform"]["scale"]["y"].get_or(1.0)
                    ),
                    entity["components"]["transform"]["rotation"].get_or(0.0)
                );
            }

            // RigidBody
            sol::optional<sol::table> rigidbody = entity["components"]["rigidbody"];
            if (rigidbody != sol::nullopt) {
                newEntity.AddComponent<RigidBodyComponent>(
                    glm::vec2(
                        entity["components"]["rigidbody"]["velocity"]["x"].get_or(0.0),
                        entity["components"]["rigidbody"]["velocity"]["y"].get_or(0.0)
                    )
                );
            }

            // Sprite
            sol::optional<sol::table> sprite = entity["components"]["sprite"];
            if (sprite != sol::nullopt) {
                newEntity.AddComponent<SpriteComponent>(
                    entity["components"]["sprite"]["texture_asset_id"],
                    entity["components"]["sprite"]["width"],
                    entity["components"]["sprite"]["height"],
                    entity["components"]["sprite"]["z_index"].get_or(1),
                    entity["components"]["sprite"]["fixed"].get_or(false),
                    entity["components"]["sprite"]["src_rect_x"].get_or(0),
                    entity["components"]["sprite"]["src_rect_y"].get_or(0)
                );
            }

            // Animation
            sol::optional<sol::table> animation = entity["components"]["animation"];
            if (animation != sol::nullopt) {
                newEntity.AddComponent<AnimationComponent>(
                    entity["components"]["animation"]["num_frames"].get_or(1),
                    entity["components"]["animation"]["speed_rate"].get_or(1)
                );
            }

            // BoxCollider
            sol::optional<sol::table> collider = entity["components"]["boxcollider"];
            if (collider != sol::nullopt) {
                newEntity.AddComponent<BoxCollisionComponent>(
                    entity["components"]["boxcollider"]["width"],
                    entity["components"]["boxcollider"]["height"],
                    glm::vec2(
                        entity["components"]["boxcollider"]["offset"]["x"].get_or(0),
                        entity["components"]["boxcollider"]["offset"]["y"].get_or(0)
                    )
                );
            }

            // PhysicsBody
            sol::optional<sol::table> physicsBody = entity["components"]["physics_body"];
            const bool hasPhysicsBodyConfig = physicsBody != sol::nullopt;
            const bool shouldCreateDefaultEnemyPhysicsBody =
                !hasPhysicsBodyConfig &&
                collider != sol::nullopt &&
                entityGroup == "enemies";
            const bool shouldCreateDefaultPlayerPhysicsBody =
                !hasPhysicsBodyConfig &&
                collider != sol::nullopt &&
                entityTag == "player";
            const bool shouldCreateDefaultObstaclePhysicsBody =
                !hasPhysicsBodyConfig &&
                collider != sol::nullopt &&
                entityGroup == "obstacles";
            const bool shouldCreateDefaultPhysicsBody =
                shouldCreateDefaultEnemyPhysicsBody ||
                shouldCreateDefaultPlayerPhysicsBody ||
                shouldCreateDefaultObstaclePhysicsBody;

            if (hasPhysicsBodyConfig || shouldCreateDefaultPhysicsBody) {
                if (!newEntity.HasComponent<TransformComponent>()) {
                    AE::Logger::Warn("physics_body skipped for entity id " + std::to_string(newEntity.GetID()) + ": missing transform component");
                }
                else if (!registry->HasSystem<PhysicsWorldSystem>()) {
                    AE::Logger::Warn("physics_body skipped for entity id " + std::to_string(newEntity.GetID()) + ": PhysicsWorldSystem is not registered");
                }
                else {
                    PhysicsBodyDefinition bodyDefinition;

                    float defaultWidth = 1.0f;
                    float defaultHeight = 1.0f;
                    glm::vec2 defaultOffset = glm::vec2(0.0f, 0.0f);
                    if (collider != sol::nullopt) {
                        sol::table colliderConfig = collider.value();
                        defaultWidth = ReadFloat(colliderConfig, "width", defaultWidth);
                        defaultHeight = ReadFloat(colliderConfig, "height", defaultHeight);
                        defaultOffset = ReadVec2(colliderConfig, "offset", defaultOffset);
                    }

                    bodyDefinition.width = defaultWidth;
                    bodyDefinition.height = defaultHeight;
                    bodyDefinition.offset = defaultOffset;
                    bodyDefinition.mass = newEntity.HasComponent<RigidBodyComponent>() ? 1.0f : 0.0f;
                    bodyDefinition.collisionCategory = DefaultPhysicsCollisionCategory(entityTag, entityGroup);
                    bodyDefinition.collisionMask = DefaultPhysicsCollisionMask(entityTag, entityGroup);
                    bodyDefinition.isSensor = shouldCreateDefaultEnemyPhysicsBody;

                    if (shouldCreateDefaultPlayerPhysicsBody) {
                        bodyDefinition.mass = newEntity.HasComponent<RigidBodyComponent>() ? 1.0f : 0.0f;
                        bodyDefinition.pullPositionFromPhysics = true;
                        bodyDefinition.pullRotationFromPhysics = true;
                        bodyDefinition.isSensor = false;
                    }

                    if (shouldCreateDefaultObstaclePhysicsBody) {
                        bodyDefinition.mass = 0.0f;
                        bodyDefinition.pullPositionFromPhysics = false;
                        bodyDefinition.pullRotationFromPhysics = false;
                        bodyDefinition.isSensor = false;
                    }

                    if (hasPhysicsBodyConfig) {
                        sol::table physicsBodyConfig = physicsBody.value();
                        std::string shapeName = physicsBodyConfig["shape"].get_or(std::string(""));
                        if (shapeName.empty()) {
                            shapeName = physicsBodyConfig["type"].get_or(std::string("box"));
                        }

                        bodyDefinition.shapeType = PhysicsBodyFactory::ShapeTypeFromString(shapeName);
                        bodyDefinition.mass = ReadBool(physicsBodyConfig, "static", false)
                            ? 0.0f
                            : ReadFloat(physicsBodyConfig, "mass", 1.0f);
                        bodyDefinition.width = ReadFloat(physicsBodyConfig, "width", defaultWidth);
                        bodyDefinition.height = ReadFloat(physicsBodyConfig, "height", defaultHeight);
                        bodyDefinition.radius = ReadFloat(physicsBodyConfig, "radius", 0.5f);
                        bodyDefinition.offset = ReadVec2(physicsBodyConfig, "offset", defaultOffset);
                        bodyDefinition.velocity = ReadVec2(physicsBodyConfig, "velocity", glm::vec2(0.0f, 0.0f));
                        bodyDefinition.angularVelocity = ReadFloat(physicsBodyConfig, "angular_velocity", 0.0f);
                        bodyDefinition.pullPositionFromPhysics = ReadBool(physicsBodyConfig, "pull_position", true);
                        bodyDefinition.pullRotationFromPhysics = ReadBool(physicsBodyConfig, "pull_rotation", true);
                        bodyDefinition.collisionCategory = ReadCollisionLayer(
                            physicsBodyConfig,
                            "collision_category",
                            "category",
                            bodyDefinition.collisionCategory);
                        bodyDefinition.collisionMask = ReadCollisionLayer(
                            physicsBodyConfig,
                            "collision_mask",
                            "mask",
                            bodyDefinition.collisionMask);
                        bodyDefinition.isSensor = ReadBool(
                            physicsBodyConfig,
                            "sensor",
                            ReadBool(physicsBodyConfig, "is_sensor", false));
                    }

                    auto& physicsWorldSystem = registry->GetSystem<PhysicsWorldSystem>();
                    auto& transformComponent = newEntity.GetComponent<TransformComponent>();
                    PhysicsBodyComponent physicsBodyComponent = PhysicsBodyFactory::Create(
                        physicsWorldSystem,
                        transformComponent,
                        bodyDefinition);

                    newEntity.AddComponent<PhysicsBodyComponent>(physicsBodyComponent);
                }
            }

            // Health
            sol::optional<sol::table> health = entity["components"]["health"];
            if (health != sol::nullopt) {
                newEntity.AddComponent<HealthComponent>(
                    static_cast<int>(entity["components"]["health"]["health_percentage"].get_or(100))
                );
            }

            // ProjectileEmitter
            sol::optional<sol::table> projectileEmitter = entity["components"]["projectile_emitter"];
            if (projectileEmitter != sol::nullopt) {
                newEntity.AddComponent<ProjectileEmitterComponent>(
                    glm::vec2(
                        entity["components"]["projectile_emitter"]["projectile_velocity"]["x"],
                        entity["components"]["projectile_emitter"]["projectile_velocity"]["y"]
                    ),
                    static_cast<int>(entity["components"]["projectile_emitter"]["repeat_frequency"].get_or(1)) * 1000,
                    static_cast<int>(entity["components"]["projectile_emitter"]["projectile_duration"].get_or(10)) * 1000,
                    static_cast<int>(entity["components"]["projectile_emitter"]["hit_percentage_damage"].get_or(10)),
                    entity["components"]["projectile_emitter"]["friendly"].get_or(false)
                );
            }

            // CameraFollow
            sol::optional<sol::table> cameraFollow = entity["components"]["camera_follow"];
            if (cameraFollow != sol::nullopt) {
                newEntity.AddComponent<CameraFollowComponent>();
            }

            // KeyboardControlled
            sol::optional<sol::table> keyboardControlled = entity["components"]["keyboard_controller"];
            if (keyboardControlled != sol::nullopt) {
                newEntity.AddComponent<KeyboardControlledComponent>(
                    glm::vec2(
                        entity["components"]["keyboard_controller"]["up_velocity"]["x"],
                        entity["components"]["keyboard_controller"]["up_velocity"]["y"]
                    ),
                    glm::vec2(
                        entity["components"]["keyboard_controller"]["right_velocity"]["x"],
                        entity["components"]["keyboard_controller"]["right_velocity"]["y"]
                    ),
                    glm::vec2(
                        entity["components"]["keyboard_controller"]["down_velocity"]["x"],
                        entity["components"]["keyboard_controller"]["down_velocity"]["y"]
                    ),
                    glm::vec2(
                        entity["components"]["keyboard_controller"]["left_velocity"]["x"],
                        entity["components"]["keyboard_controller"]["left_velocity"]["y"]
                    )
                );
            }
        }
        i++;
    }
}
