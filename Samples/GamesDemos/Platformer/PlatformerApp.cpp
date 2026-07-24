#include "PlatformerApp.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <sstream>

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
#include "imgui.h"
#endif

#include "Logging/Logger.h"
#include "Physics/Constraint.h"
#include "Physics/Objects/Shape.h"

namespace
{
    constexpr float FixedTimeStep = 1.0f / 60.0f;
    constexpr float Gravity = 1850.0f;
    constexpr float MoveAcceleration = 4200.0f;
    constexpr float GroundFriction = 5000.0f;
    constexpr float MaxRunSpeed = 255.0f;
    constexpr float JumpVelocity = -650.0f;
    constexpr float JumpCutVelocity = -260.0f;
    constexpr float CoyoteTime = 0.11f;
    constexpr float JumpBufferTime = 0.13f;
    constexpr float MaxFallSpeed = 880.0f;
    constexpr float EnemyGravity = 1350.0f;
    constexpr float ProjectileSpeed = 680.0f;
    constexpr float ProjectileCooldown = 0.18f;
    constexpr float PhysicsFixedTimeStep = 1.0f / 120.0f;
    constexpr std::uint32_t PhysicsCategoryTerrain = 0x00000001u;
    constexpr std::uint32_t PhysicsCategoryDynamic = 0x00000002u;
    constexpr std::uint32_t PhysicsCategorySensor = 0x00000004u;

    SDL_Color SkyTop{100, 168, 235, 255};
    SDL_Color SkyBottom{170, 215, 245, 255};
    SDL_Color Ground{72, 124, 55, 255};
    SDL_Color Dirt{126, 86, 51, 255};
    SDL_Color Brick{158, 92, 72, 255};
    SDL_Color PlayerBody{35, 123, 172, 255};
    SDL_Color PlayerAccent{242, 225, 179, 255};
    SDL_Color CoinColor{245, 196, 48, 255};
    SDL_Color EnemyColor{174, 54, 62, 255};
    SDL_Color ProjectileColor{255, 236, 132, 255};
    SDL_Color PhysicsWood{184, 130, 72, 255};
    SDL_Color PhysicsWoodEdge{92, 58, 42, 255};
    SDL_Color PhysicsMetal{110, 130, 148, 255};
    SDL_Color PhysicsMetalEdge{42, 52, 60, 255};
    SDL_Color PhysicsBall{78, 170, 200, 255};
    SDL_Color PhysicsJointColor{42, 72, 80, 180};

    bool IsFullscreenToggleKey(const SDL_KeyboardEvent& keyEvent)
    {
        const SDL_Keycode key = keyEvent.keysym.sym;
        const bool altPressed = (keyEvent.keysym.mod & KMOD_ALT) != 0;
        return key == SDLK_F11 || (altPressed && (key == SDLK_RETURN || key == SDLK_KP_ENTER));
    }

    int RoundToInt(float value)
    {
        return static_cast<int>(std::round(value));
    }

    float ReadFloat(sol::table table, const char* key, float defaultValue)
    {
        return static_cast<float>(table[key].get_or(defaultValue));
    }

    int ReadInt(sol::table table, const char* key, int defaultValue)
    {
        return table[key].get_or(defaultValue);
    }

    SDL_Color ReadColor(sol::table table, const char* key, SDL_Color defaultValue)
    {
        sol::optional<sol::table> colorTable = table[key];
        if (colorTable == sol::nullopt)
        {
            return defaultValue;
        }

        sol::table color = colorTable.value();
        return SDL_Color{
            static_cast<Uint8>(ReadInt(color, "r", defaultValue.r)),
            static_cast<Uint8>(ReadInt(color, "g", defaultValue.g)),
            static_cast<Uint8>(ReadInt(color, "b", defaultValue.b)),
            static_cast<Uint8>(ReadInt(color, "a", defaultValue.a))
        };
    }

    std::filesystem::path FindPlatformerEnemyScript()
    {
        namespace fs = std::filesystem;

        const fs::path relativePath = fs::path("Samples") / "GamesDemos" / "Platformer" /
            "Content" / "Scripts" / "PlatformerEnemies.lua";
        const std::array<fs::path, 10> candidateRoots = {
            fs::current_path(),
            fs::current_path() / "AmberEngine",
            fs::current_path() / "..",
            fs::current_path() / ".." / "..",
            fs::current_path() / ".." / ".." / "..",
            fs::current_path() / ".." / ".." / ".." / "..",
            fs::current_path() / ".." / "AmberEngine",
            fs::current_path() / ".." / ".." / "AmberEngine",
            fs::current_path() / ".." / ".." / ".." / "AmberEngine",
            fs::current_path() / ".." / ".." / ".." / ".." / "AmberEngine"
        };

        for (const fs::path& root : candidateRoots)
        {
            const fs::path candidate = root / relativePath;
            std::error_code error;
            if (fs::exists(candidate, error))
            {
                return fs::weakly_canonical(candidate, error);
            }
        }

        return {};
    }
}

PlatformerApp::PlatformerApp()
{
    AE::Logger::SetConsoleEnabled(false);
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);
    BuildLevel();
    ResetLevel();
}

namespace
{
    double ElapsedMs(Uint64 startCounter, Uint64 endCounter)
    {
        return static_cast<double>(endCounter - startCounter) * 1000.0 /
            static_cast<double>(SDL_GetPerformanceFrequency());
    }
}

int PlatformerApp::Run()
{
#if SMOKE_TEST
    smokeMode = false;
#endif
    if (!Initialize())
    {
        return 1;
    }

    InputState input;
    Uint64 previousCounter = SDL_GetPerformanceCounter();
    float accumulator = 0.0f;
    running = true;

    while (running)
    {
        PollEvents(input);

        const Uint64 currentCounter = SDL_GetPerformanceCounter();
        const float elapsed = static_cast<float>(currentCounter - previousCounter) /
            static_cast<float>(SDL_GetPerformanceFrequency());
        previousCounter = currentCounter;
        accumulator += ClampFloat(elapsed, 0.0f, 0.05f);

        const Uint64 updateStart = SDL_GetPerformanceCounter();
        InputState stepInput = input;
        bool stepped = false;
        fixedStepsThisFrame = 0;
        while (!paused && accumulator >= FixedTimeStep)
        {
            Step(FixedTimeStep, stepInput);
            stepInput.jumpPressed = false;
            stepInput.shootPressed = false;
            accumulator -= FixedTimeStep;
            stepped = true;
            ++fixedStepsThisFrame;
        }
        if (stepped)
        {
            input.jumpPressed = false;
            input.shootPressed = false;
        }
        lastUpdateMs = ElapsedMs(updateStart, SDL_GetPerformanceCounter());

        Render();
    }

    Shutdown();
    return 0;
}

#if SMOKE_TEST
bool PlatformerApp::RunSmokeTest()
{
    smokeMode = true;
    ResetLevel();

    const float startX = player.position.x;
    InputState runRight;
    runRight.moveRight = true;

    for (int frame = 0; frame < 140; ++frame)
    {
        runRight.jumpPressed = frame == 8 || frame == 72;
        runRight.jumpHeld = (frame >= 8 && frame < 26) || (frame >= 72 && frame < 90);
        Step(FixedTimeStep, runRight);
    }

    const bool movedRight = player.position.x > startX + 110.0f;
    const bool stayedInWorld = player.position.y < static_cast<float>(LevelRows * TileSize);
    const bool hasGroundState = player.grounded || player.velocity.y >= 0.0f;

    if (!coins.empty())
    {
        player.position.x = coins.front().bounds.x;
        player.position.y = coins.front().bounds.y;
        UpdateCoins();
    }
    const bool coinCollected = !coins.empty() && coins.front().collected;

    if (!enemies.empty())
    {
        player.position.x = enemies.front().position.x;
        player.position.y = enemies.front().position.y;
        UpdateHazards();
    }
    const bool hazardResetsPlayer = std::abs(player.position.x - playerSpawn.x) < 0.5f &&
        std::abs(player.position.y - playerSpawn.y) < 0.5f;

    player.position.x = finish.x;
    player.position.y = finish.y;
    UpdateGoal();
    const bool finishWorks = player.won;

    ResetPlayer();
    for (int frame = 0; frame < 12; ++frame)
    {
        Step(FixedTimeStep, InputState{});
    }
    const float groundedY = player.position.y;
    player.position.y = groundedY - 18.0f;
    player.velocity.y = 340.0f;
    player.grounded = false;
    InputState bufferedJumpInput;
    bufferedJumpInput.jumpPressed = true;
    bufferedJumpInput.jumpHeld = true;
    Step(FixedTimeStep, bufferedJumpInput);
    bufferedJumpInput.jumpPressed = false;
    for (int frame = 0; frame < 9; ++frame)
    {
        Step(FixedTimeStep, bufferedJumpInput);
    }
    const bool bufferedJumpWorks = player.velocity.y < 0.0f && !player.grounded;

    ResetPlayer();
    for (int frame = 0; frame < 12; ++frame)
    {
        Step(FixedTimeStep, InputState{});
    }
    player.position.x += 2.0f * TileSize;
    player.grounded = false;
    InputState coyoteJumpInput;
    coyoteJumpInput.jumpPressed = true;
    coyoteJumpInput.jumpHeld = true;
    Step(FixedTimeStep, coyoteJumpInput);
    const bool coyoteJumpWorks = player.velocity.y < 0.0f && !player.grounded;

    const bool scriptedEnemiesWork = scriptedEnemiesLoaded && enemies.size() >= 4;

    ResetLevel();
    bool shootingWorks = false;
    if (!enemies.empty())
    {
        Enemy& target = enemies.back();
        player.position = AE::Physics::Vector2D(target.position.x - 190.0f, target.position.y);
        player.velocity = AE::Physics::Vector2D::Zero;
        player.facing = 1;

        for (int shot = 0; shot < 5 && target.alive; ++shot)
        {
            InputState shootInput;
            shootInput.shootPressed = true;
            Step(FixedTimeStep, shootInput);
            shootInput.shootPressed = false;
            for (int frame = 0; frame < 34 && target.alive; ++frame)
            {
                Step(FixedTimeStep, shootInput);
            }
        }
        shootingWorks = !target.alive;
    }

    ResetLevel();
    AE::Physics::Body* testBody = nullptr;
    for (const BodyVisual& visual : physicsVisuals)
    {
        if (visual.body && visual.gameplayBody && !visual.body->IsStatic())
        {
            testBody = visual.body;
            break;
        }
    }

    bool physicsBodyReacts = false;
    if (testBody)
    {
        const AE::Physics::Vector2D startPosition = testBody->position;
        player.position = AE::Physics::Vector2D(testBody->position.x - 115.0f, testBody->position.y - player.height * 0.45f);
        player.velocity = AE::Physics::Vector2D::Zero;
        player.facing = 1;

        InputState shootInput;
        shootInput.shootPressed = true;
        Step(FixedTimeStep, shootInput);
        shootInput.shootPressed = false;
        for (int frame = 0; frame < 28; ++frame)
        {
            Step(FixedTimeStep, shootInput);
        }

        physicsBodyReacts = (testBody->position - startPosition).MagnitudeSquared() > 0.5f ||
            testBody->velocity.MagnitudeSquared() > 5.0f;
    }

    const bool physicsSceneWorks = PhysicsBodyCount() > static_cast<int>(solidPlatforms.size()) + 18 &&
        PhysicsConstraintCount() >= 12 &&
        physicsWorld &&
        physicsWorld->GetLastStats().bodyCount > 0;

    const bool passed = movedRight && stayedInWorld && hasGroundState && coinCollected && hazardResetsPlayer && finishWorks &&
        bufferedJumpWorks && coyoteJumpWorks && scriptedEnemiesWork && shootingWorks && physicsSceneWorks &&
        physicsBodyReacts;
    if (!passed)
    {
        std::cerr
            << "Platformer smoke diagnostics:"
            << " movedRight=" << movedRight
            << " stayedInWorld=" << stayedInWorld
            << " hasGroundState=" << hasGroundState
            << " coinCollected=" << coinCollected
            << " hazardResetsPlayer=" << hazardResetsPlayer
            << " finishWorks=" << finishWorks
            << " bufferedJumpWorks=" << bufferedJumpWorks
            << " coyoteJumpWorks=" << coyoteJumpWorks
            << " scriptedEnemiesWork=" << scriptedEnemiesWork
            << " shootingWorks=" << shootingWorks
            << " physicsSceneWorks=" << physicsSceneWorks
            << " physicsBodyReacts=" << physicsBodyReacts
            << " enemies=" << enemies.size()
            << " alive=" << AliveEnemyCount()
            << " bodies=" << PhysicsBodyCount()
            << " constraints=" << PhysicsConstraintCount()
            << std::endl;
    }
    return passed;
}
#endif

bool PlatformerApp::Initialize()
{
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow(
        "Platformer Sample",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WindowWidth,
        WindowHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        Shutdown();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (!renderer)
    {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
    }
    if (!renderer)
    {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        Shutdown();
        return false;
    }

    if (!CreateFrameTexture())
    {
        Shutdown();
        return false;
    }

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
    diagnostics.Initialize(window, renderer, WindowWidth, WindowHeight);
#endif

    ResetLevel();
    return true;
}

void PlatformerApp::Shutdown()
{
#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
    diagnostics.Shutdown();
#endif

    if (frameTexture)
    {
        SDL_DestroyTexture(frameTexture);
        frameTexture = nullptr;
    }
    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}

void PlatformerApp::ToggleFullscreen()
{
    if (!window)
    {
        return;
    }

    fullscreen = !fullscreen;
    SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    if (!fullscreen)
    {
        SDL_SetWindowSize(window, WindowWidth, WindowHeight);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
}

void PlatformerApp::BuildLevel()
{
    levelTiles.assign(LevelRows, std::string(LevelCols, '.'));
    solidPlatforms.clear();

    auto fillTiles = [this](int tileX, int tileY, int width, int height, char value)
    {
        if (value == '#')
        {
            solidPlatforms.push_back(SolidPlatform{tileX, tileY, width, height});
        }

        for (int y = tileY; y < tileY + height; ++y)
        {
            for (int x = tileX; x < tileX + width; ++x)
            {
                if (x >= 0 && x < LevelCols && y >= 0 && y < LevelRows)
                {
                    levelTiles[y][x] = value;
                }
            }
        }
    };

    fillTiles(0, 15, LevelCols, 2, '#');
    fillTiles(14, 14, 6, 1, '#');
    fillTiles(25, 13, 6, 1, '#');
    fillTiles(36, 12, 7, 1, '#');
    fillTiles(50, 11, 6, 1, '#');
    fillTiles(63, 13, 8, 1, '#');
    fillTiles(78, 10, 5, 1, '#');
    fillTiles(58, 14, 3, 1, '#');
    fillTiles(72, 14, 4, 1, '#');

    fillTiles(7, 14, 1, 1, '#');
    fillTiles(8, 13, 1, 2, '#');
    fillTiles(9, 12, 1, 3, '#');
    fillTiles(86, 14, 1, 1, '#');
    fillTiles(87, 13, 1, 2, '#');
    fillTiles(88, 12, 1, 3, '#');

    playerSpawn = AE::Physics::Vector2D(64.0f, 15.0f * TileSize - 42.0f);
    finish = RectF{
        91.0f * TileSize,
        15.0f * TileSize - 96.0f,
        32.0f,
        96.0f
    };

    coins = {
        Coin{RectF{10.5f * TileSize, 11.8f * TileSize, 18.0f, 18.0f}},
        Coin{RectF{27.0f * TileSize, 12.0f * TileSize, 18.0f, 18.0f}},
        Coin{RectF{39.5f * TileSize, 10.8f * TileSize, 18.0f, 18.0f}},
        Coin{RectF{53.0f * TileSize, 9.8f * TileSize, 18.0f, 18.0f}},
        Coin{RectF{67.0f * TileSize, 11.8f * TileSize, 18.0f, 18.0f}},
        Coin{RectF{80.0f * TileSize, 8.8f * TileSize, 18.0f, 18.0f}}
    };

    LoadScriptedEnemies();
}

void PlatformerApp::LoadScriptedEnemies()
{
    enemies.clear();
    scriptedEnemiesLoaded = false;
    enemyScriptPath.clear();

    const std::filesystem::path scriptPath = FindPlatformerEnemyScript();
    if (scriptPath.empty())
    {
        LoadFallbackEnemies();
        return;
    }

    try
    {
        sol::load_result loadedScript = lua.load_file(scriptPath.string());
        if (!loadedScript.valid())
        {
            sol::error error = loadedScript;
            std::cerr << "Failed to load platformer enemy script: " << error.what() << std::endl;
            LoadFallbackEnemies();
            return;
        }

        sol::protected_function script = loadedScript;
        sol::protected_function_result result = script();
        if (!result.valid())
        {
            sol::error error = result;
            std::cerr << "Failed to run platformer enemy script: " << error.what() << std::endl;
            LoadFallbackEnemies();
            return;
        }

        sol::table config = result;
        sol::optional<sol::table> enemyTableList = config["enemies"];
        if (enemyTableList == sol::nullopt)
        {
            LoadFallbackEnemies();
            return;
        }

        for (auto&& enemyEntry : enemyTableList.value())
        {
            const sol::object enemyObject = enemyEntry.second;
            if (!enemyObject.is<sol::table>())
            {
                continue;
            }

            sol::table enemyTable = enemyObject.as<sol::table>();
            Enemy enemy;
            enemy.name = enemyTable["name"].get_or(std::string("scripted_enemy"));
            enemy.spawnPosition = AE::Physics::Vector2D(
                ReadFloat(enemyTable, "x", 19.0f * TileSize),
                ReadFloat(enemyTable, "y", 15.0f * TileSize - enemy.height));
            enemy.position = enemy.spawnPosition;
            enemy.width = ReadFloat(enemyTable, "width", enemy.width);
            enemy.height = ReadFloat(enemyTable, "height", enemy.height);
            enemy.speed = std::abs(ReadFloat(enemyTable, "speed", enemy.speed));
            enemy.direction = ReadFloat(enemyTable, "direction", enemy.direction) < 0.0f ? -1.0f : 1.0f;
            enemy.leftBound = ReadFloat(enemyTable, "left", enemy.spawnPosition.x - 96.0f);
            enemy.rightBound = ReadFloat(enemyTable, "right", enemy.spawnPosition.x + 160.0f);
            enemy.groundY = ReadFloat(enemyTable, "ground_y", enemy.spawnPosition.y);
            enemy.maxHealth = std::max(1, ReadInt(enemyTable, "health", enemy.maxHealth));
            enemy.health = enemy.maxHealth;
            enemy.jumpCooldown = ReadFloat(enemyTable, "jump_cooldown", enemy.jumpCooldown);
            enemy.jumpVelocity = ReadFloat(enemyTable, "jump_velocity", enemy.jumpVelocity);
            enemy.alertRange = ReadFloat(enemyTable, "alert_range", enemy.alertRange);
            enemy.color = ReadColor(enemyTable, "color", enemy.color);
            enemy.velocity = AE::Physics::Vector2D(enemy.speed * enemy.direction, 0.0f);

            sol::object updateObject = enemyTable.get<sol::object>("on_update");
            if (updateObject.valid() && updateObject.is<sol::function>())
            {
                enemy.updateScript = updateObject.as<sol::function>();
            }

            enemies.push_back(enemy);
        }

        if (enemies.empty())
        {
            LoadFallbackEnemies();
            return;
        }

        scriptedEnemiesLoaded = true;
        enemyScriptPath = scriptPath.string();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Failed to read platformer enemy script: " << exception.what() << std::endl;
        LoadFallbackEnemies();
    }
}

void PlatformerApp::LoadFallbackEnemies()
{
    scriptedEnemiesLoaded = false;
    enemies = {
        Enemy{
            "fallback_patrol",
            AE::Physics::Vector2D(19.0f * TileSize, 15.0f * TileSize - 26.0f),
            {},
            AE::Physics::Vector2D(70.0f, 0.0f),
            30.0f,
            26.0f,
            70.0f,
            1.0f,
            18.0f * TileSize,
            24.0f * TileSize,
            15.0f * TileSize - 26.0f,
            0.0f,
            0.0f,
            1.2f,
            -340.0f,
            240.0f,
            2,
            2,
            true,
            EnemyColor
        },
        Enemy{
            "fallback_hopper",
            AE::Physics::Vector2D(45.0f * TileSize, 15.0f * TileSize - 28.0f),
            {},
            AE::Physics::Vector2D(-58.0f, 0.0f),
            30.0f,
            28.0f,
            58.0f,
            -1.0f,
            43.0f * TileSize,
            49.0f * TileSize,
            15.0f * TileSize - 28.0f,
            0.0f,
            0.0f,
            1.1f,
            -360.0f,
            240.0f,
            3,
            3,
            true,
            SDL_Color{90, 132, 210, 255}
        },
        Enemy{
            "fallback_sentry",
            AE::Physics::Vector2D(66.0f * TileSize, 13.0f * TileSize - 28.0f),
            {},
            AE::Physics::Vector2D(54.0f, 0.0f),
            32.0f,
            28.0f,
            54.0f,
            1.0f,
            63.0f * TileSize,
            71.0f * TileSize,
            13.0f * TileSize - 28.0f,
            0.0f,
            0.0f,
            1.2f,
            -340.0f,
            260.0f,
            4,
            4,
            true,
            SDL_Color{151, 83, 188, 255}
        }
    };

    for (Enemy& enemy : enemies)
    {
        enemy.position = enemy.spawnPosition;
    }
}

void PlatformerApp::BuildPhysicsScene()
{
    physicsSceneTime = 0.0f;
    physicsWorld = std::make_unique<AE::Physics::World>(-9.8f);
    physicsWorld->SetSolverIterations(8);
    physicsWorld->SetBroadPhaseEnabled(true);
    physicsWorld->SetBroadPhaseCellSize(48.0f);
    physicsWorld->SetSleepingEnabled(true);
    physicsWorld->SetParallelNarrowPhaseEnabled(true);
    physicsWorld->SetParallelSolverEnabled(true);

    physicsVisuals.clear();
    kinematicBodies.clear();

    for (const SolidPlatform& platform : solidPlatforms)
    {
        const float width = static_cast<float>(platform.width * TileSize);
        const float height = static_cast<float>(platform.height * TileSize);
        const AE::Physics::Vector2D center(
            static_cast<float>(platform.tileX * TileSize) + width * 0.5f,
            static_cast<float>(platform.tileY * TileSize) + height * 0.5f);
        AE::Physics::Body* body = AddPhysicsBox(center, width, height, 0.0f, SDL_Color{72, 124, 55, 80}, SDL_Color{55, 82, 46, 120}, 0.0f, false);
        body->collisionCategory = PhysicsCategoryTerrain;
        body->collisionMask = PhysicsCategoryDynamic | PhysicsCategorySensor;
    }

    for (int row = 0; row < 3; ++row)
    {
        for (int column = 0; column < 4 - row; ++column)
        {
            const float x = 560.0f + static_cast<float>(column) * 34.0f + static_cast<float>(row) * 17.0f;
            const float y = 450.0f - static_cast<float>(row) * 31.0f;
            AE::Physics::Body* crate = AddPhysicsBox(
                AE::Physics::Vector2D(x, y),
                30.0f,
                30.0f,
                1.1f,
                PhysicsWood,
                PhysicsWoodEdge,
                0.04f * static_cast<float>(column - 1),
                true);
            crate->friction = 0.24f;
            crate->restitution = 0.08f;
        }
    }

    for (int index = 0; index < 7; ++index)
    {
        AE::Physics::Body* ball = AddPhysicsCircle(
            AE::Physics::Vector2D(1110.0f + static_cast<float>(index % 4) * 28.0f, 250.0f - static_cast<float>(index / 4) * 32.0f),
            13.0f,
            0.85f,
            index % 2 == 0 ? PhysicsBall : SDL_Color{230, 178, 72, 255},
            PhysicsMetalEdge,
            true);
        ball->friction = 0.05f;
        ball->restitution = 0.32f;
    }

    AE::Physics::Body* bridgeLeftAnchor = AddPhysicsBox(AE::Physics::Vector2D(1440.0f, 270.0f), 46.0f, 24.0f, 0.0f, PhysicsMetal, PhysicsMetalEdge, 0.0f, false);
    AE::Physics::Body* bridgeRightAnchor = AddPhysicsBox(AE::Physics::Vector2D(1810.0f, 270.0f), 46.0f, 24.0f, 0.0f, PhysicsMetal, PhysicsMetalEdge, 0.0f, false);
    std::vector<AE::Physics::Body*> bridgeLinks;
    constexpr int BridgeSegments = 10;
    for (int index = 0; index < BridgeSegments; ++index)
    {
        const float t = static_cast<float>(index) / static_cast<float>(BridgeSegments - 1);
        AE::Physics::Body* link = AddPhysicsBox(
            AE::Physics::Vector2D(1480.0f + t * 290.0f, 304.0f + std::sin(t * 3.14159f) * 12.0f),
            32.0f,
            14.0f,
            0.8f,
            index % 2 == 0 ? SDL_Color{92, 174, 184, 255} : SDL_Color{230, 178, 72, 255},
            PhysicsMetalEdge,
            0.0f,
            true);
        link->friction = 0.16f;
        bridgeLinks.push_back(link);
    }

    if (!bridgeLinks.empty())
    {
        AddPhysicsJoint(bridgeLeftAnchor, bridgeLinks.front());
        for (std::size_t index = 1; index < bridgeLinks.size(); ++index)
        {
            AddPhysicsJoint(bridgeLinks[index - 1], bridgeLinks[index]);
        }
        AddPhysicsJoint(bridgeLinks.back(), bridgeRightAnchor);
    }

    AE::Physics::Body* chainAnchor = AddPhysicsBox(AE::Physics::Vector2D(2220.0f, 150.0f), 48.0f, 22.0f, 0.0f, PhysicsMetal, PhysicsMetalEdge, 0.0f, false);
    AE::Physics::Body* previous = chainAnchor;
    for (int index = 0; index < 7; ++index)
    {
        AE::Physics::Body* link = AddPhysicsBox(
            AE::Physics::Vector2D(2220.0f, 188.0f + static_cast<float>(index) * 28.0f),
            16.0f,
            24.0f,
            0.65f,
            SDL_Color{130, 154, 172, 255},
            PhysicsMetalEdge,
            0.0f,
            true);
        AddPhysicsJoint(previous, link);
        previous = link;
    }
    AE::Physics::Body* load = AddPhysicsCircle(AE::Physics::Vector2D(2220.0f, 405.0f), 22.0f, 3.5f, SDL_Color{205, 74, 75, 255}, PhysicsMetalEdge, true);
    AddPhysicsJoint(previous, load);

    AE::Physics::Body* movingPlatform = AddPhysicsBox(
        AE::Physics::Vector2D(2420.0f, 360.0f),
        154.0f,
        20.0f,
        0.0f,
        SDL_Color{104, 184, 116, 255},
        PhysicsMetalEdge,
        0.0f,
        false);
    kinematicBodies.push_back(KinematicBody{
        movingPlatform,
        movingPlatform->position,
        AE::Physics::Vector2D(1.0f, 0.0f),
        92.0f,
        1.15f,
        0.0f
    });
}

void PlatformerApp::ResetLevel()
{
    for (Coin& coin : coins)
    {
        coin.collected = false;
    }
    for (Enemy& enemy : enemies)
    {
        enemy.position = enemy.spawnPosition;
        enemy.velocity = AE::Physics::Vector2D(enemy.speed * enemy.direction, 0.0f);
        enemy.health = enemy.maxHealth;
        enemy.alive = true;
        enemy.timeAlive = 0.0f;
        enemy.timeSinceJump = 0.0f;
    }
    projectiles.clear();
    shootCooldownTimer = 0.0f;
    BuildPhysicsScene();
    ResetPlayer();
}

void PlatformerApp::ResetPlayer()
{
    player.position = playerSpawn;
    player.velocity = AE::Physics::Vector2D::Zero;
    player.grounded = false;
    player.won = false;
    player.facing = 1;
    coyoteTimer = 0.0f;
    jumpBufferTimer = 0.0f;
}

void PlatformerApp::PollEvents(InputState& input)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
        diagnostics.ProcessEvent(event);
#endif

        if (event.type == SDL_QUIT)
        {
            running = false;
        }
        else if (event.type == SDL_KEYDOWN && !event.key.repeat)
        {
            if (IsFullscreenToggleKey(event.key))
            {
                ToggleFullscreen();
                continue;
            }

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
            if (event.key.keysym.sym == SDLK_F1)
            {
                diagnostics.ShowDiagnostics() = !diagnostics.ShowDiagnostics();
                continue;
            }
            if (event.key.keysym.sym == SDLK_F2)
            {
                diagnostics.ShowControls() = !diagnostics.ShowControls();
                continue;
            }
            if (event.key.keysym.sym == SDLK_F3)
            {
                diagnostics.ShowOutputLog() = !diagnostics.ShowOutputLog();
                continue;
            }
            if (diagnostics.WantsKeyboard())
            {
                continue;
            }
#endif

            switch (event.key.keysym.sym)
            {
                case SDLK_ESCAPE:
                    running = false;
                    break;
                case SDLK_p:
                    paused = !paused;
                    break;
                case SDLK_a:
                case SDLK_LEFT:
                    input.moveLeft = true;
                    break;
                case SDLK_d:
                case SDLK_RIGHT:
                    input.moveRight = true;
                    break;
                case SDLK_w:
                case SDLK_UP:
                case SDLK_SPACE:
                    input.jumpPressed = true;
                    input.jumpHeld = true;
                    break;
                case SDLK_j:
                case SDLK_LCTRL:
                case SDLK_RCTRL:
                    input.shootPressed = true;
                    break;
                case SDLK_r:
                    ResetLevel();
                    break;
                default:
                    break;
            }
        }
        else if (event.type == SDL_KEYUP)
        {
#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
            if (diagnostics.WantsKeyboard())
            {
                continue;
            }
#endif

            switch (event.key.keysym.sym)
            {
                case SDLK_a:
                case SDLK_LEFT:
                    input.moveLeft = false;
                    break;
                case SDLK_d:
                case SDLK_RIGHT:
                    input.moveRight = false;
                    break;
                case SDLK_w:
                case SDLK_UP:
                case SDLK_SPACE:
                    input.jumpHeld = false;
                    break;
                default:
                    break;
            }
        }
    }
}

void PlatformerApp::Step(float dt, const InputState& input)
{
    if (player.won)
    {
        return;
    }

    shootCooldownTimer = std::max(0.0f, shootCooldownTimer - dt);
    if (input.shootPressed)
    {
        TryShoot();
    }

    UpdateEnemies(dt);
    UpdatePlayer(dt, input);
    UpdateProjectiles(dt);
    StepPhysics(dt);
    UpdateCoins();
    UpdateHazards();
    UpdateGoal();
    UpdateCamera();
}

void PlatformerApp::TryShoot()
{
    if (shootCooldownTimer > 0.0f)
    {
        return;
    }

    const float direction = player.facing >= 0 ? 1.0f : -1.0f;
    Projectile projectile;
    projectile.position = AE::Physics::Vector2D(
        player.position.x + player.width * 0.5f + direction * 18.0f,
        player.position.y + player.height * 0.42f);
    projectile.velocity = AE::Physics::Vector2D(ProjectileSpeed * direction, 0.0f);
    projectile.damage = 1;
    projectiles.push_back(projectile);
    shootCooldownTimer = ProjectileCooldown;
}

void PlatformerApp::UpdatePlayer(float dt, const InputState& input)
{
    if (input.jumpPressed)
    {
        jumpBufferTimer = JumpBufferTime;
    }
    else
    {
        jumpBufferTimer = std::max(0.0f, jumpBufferTimer - dt);
    }

    if (player.grounded)
    {
        coyoteTimer = CoyoteTime;
    }
    else
    {
        coyoteTimer = std::max(0.0f, coyoteTimer - dt);
    }

    if (input.moveLeft == input.moveRight)
    {
        player.velocity.x = MoveTowardZero(player.velocity.x, GroundFriction * dt);
    }
    else if (input.moveLeft)
    {
        player.facing = -1;
        player.velocity.x -= MoveAcceleration * dt;
    }
    else if (input.moveRight)
    {
        player.facing = 1;
        player.velocity.x += MoveAcceleration * dt;
    }
    player.velocity.x = ClampFloat(player.velocity.x, -MaxRunSpeed, MaxRunSpeed);

    if (jumpBufferTimer > 0.0f && coyoteTimer > 0.0f)
    {
        player.velocity.y = JumpVelocity;
        player.grounded = false;
        coyoteTimer = 0.0f;
        jumpBufferTimer = 0.0f;
    }
    else if (!input.jumpHeld && !input.jumpPressed && player.velocity.y < JumpCutVelocity)
    {
        player.velocity.y = JumpCutVelocity;
    }

    player.velocity.y = ClampFloat(player.velocity.y + Gravity * dt, -1000.0f, MaxFallSpeed);

    player.position.x += player.velocity.x * dt;
    ResolveTileCollisions(true);

    player.position.y += player.velocity.y * dt;
    player.grounded = false;
    ResolveTileCollisions(false);
    if (player.grounded)
    {
        coyoteTimer = CoyoteTime;
    }

    if (player.position.y > static_cast<float>(LevelRows * TileSize + 120))
    {
        ResetPlayer();
    }
}

void PlatformerApp::UpdateEnemies(float dt)
{
    for (Enemy& enemy : enemies)
    {
        if (!enemy.alive)
        {
            continue;
        }

        enemy.timeAlive += dt;
        enemy.timeSinceJump += dt;
        const bool enemyOnGround = std::abs(enemy.position.y - enemy.groundY) < 0.5f && enemy.velocity.y >= 0.0f;

        if (enemy.updateScript.valid())
        {
            sol::table enemyState = lua.create_table();
            enemyState["name"] = enemy.name;
            enemyState["x"] = enemy.position.x;
            enemyState["y"] = enemy.position.y;
            enemyState["velocity_x"] = enemy.velocity.x;
            enemyState["velocity_y"] = enemy.velocity.y;
            enemyState["speed"] = enemy.speed;
            enemyState["direction"] = enemy.direction;
            enemyState["left"] = enemy.leftBound;
            enemyState["right"] = enemy.rightBound;
            enemyState["health"] = enemy.health;
            enemyState["time"] = enemy.timeAlive;
            enemyState["time_since_jump"] = enemy.timeSinceJump;
            enemyState["jump_cooldown"] = enemy.jumpCooldown;
            enemyState["jump_velocity"] = enemy.jumpVelocity;
            enemyState["alert_range"] = enemy.alertRange;
            enemyState["on_ground"] = enemyOnGround;

            sol::table playerState = lua.create_table();
            playerState["x"] = player.position.x;
            playerState["y"] = player.position.y;
            playerState["velocity_x"] = player.velocity.x;
            playerState["velocity_y"] = player.velocity.y;

            sol::protected_function update = enemy.updateScript;
            sol::protected_function_result result = update(enemyState, playerState, dt);
            if (result.valid())
            {
                enemy.velocity.x = ReadFloat(enemyState, "velocity_x", enemy.velocity.x);
                enemy.velocity.y = ReadFloat(enemyState, "velocity_y", enemy.velocity.y);
                enemy.direction = ReadFloat(enemyState, "direction", enemy.direction) < 0.0f ? -1.0f : 1.0f;
                enemy.timeSinceJump = ReadFloat(enemyState, "time_since_jump", enemy.timeSinceJump);
            }
            else
            {
                enemy.velocity.x = enemy.speed * enemy.direction;
            }
        }
        else
        {
            enemy.velocity.x = enemy.speed * enemy.direction;
        }

        enemy.velocity.y = ClampFloat(enemy.velocity.y + EnemyGravity * dt, -620.0f, MaxFallSpeed);
        enemy.position.x += enemy.velocity.x * dt;
        enemy.position.y += enemy.velocity.y * dt;

        if (enemy.position.x < enemy.leftBound)
        {
            enemy.position.x = enemy.leftBound;
            enemy.direction = 1.0f;
            enemy.velocity.x = enemy.speed;
        }
        else if (enemy.position.x + enemy.width > enemy.rightBound)
        {
            enemy.position.x = enemy.rightBound - enemy.width;
            enemy.direction = -1.0f;
            enemy.velocity.x = -enemy.speed;
        }

        if (enemy.position.y > enemy.groundY)
        {
            enemy.position.y = enemy.groundY;
            enemy.velocity.y = 0.0f;
        }
    }
}

void PlatformerApp::UpdateProjectiles(float dt)
{
    for (Projectile& projectile : projectiles)
    {
        if (!projectile.active)
        {
            continue;
        }

        projectile.position += projectile.velocity * dt;
        projectile.timeToLive -= dt;
        if (projectile.timeToLive <= 0.0f)
        {
            projectile.active = false;
            continue;
        }

        const RectF projectileBounds = ProjectileRect(projectile);
        const int tileX = static_cast<int>(std::floor((projectileBounds.x + projectileBounds.w * 0.5f) / TileSize));
        const int tileY = static_cast<int>(std::floor((projectileBounds.y + projectileBounds.h * 0.5f) / TileSize));
        if (IsSolidTile(tileX, tileY))
        {
            projectile.active = false;
            continue;
        }

        for (Enemy& enemy : enemies)
        {
            if (!enemy.alive || !Intersects(projectileBounds, EnemyRect(enemy)))
            {
                continue;
            }

            enemy.health -= projectile.damage;
            enemy.velocity.x += projectile.velocity.x * 0.12f;
            enemy.velocity.y = std::min(enemy.velocity.y, -120.0f);
            if (enemy.health <= 0)
            {
                enemy.alive = false;
            }
            projectile.active = false;
            break;
        }

        if (projectile.active)
        {
            ApplyProjectilePhysicsHit(projectile);
        }
    }

    projectiles.erase(
        std::remove_if(
            projectiles.begin(),
            projectiles.end(),
            [](const Projectile& projectile)
            {
                return !projectile.active;
            }),
        projectiles.end());
}

void PlatformerApp::ApplyProjectilePhysicsHit(Projectile& projectile)
{
    if (!physicsWorld)
    {
        return;
    }

    const RectF projectileBounds = ProjectileRect(projectile);
    for (BodyVisual& visual : physicsVisuals)
    {
        AE::Physics::Body* body = visual.body;
        if (!body || body->IsStatic() || !visual.gameplayBody || !Intersects(projectileBounds, BodyBounds(*body)))
        {
            continue;
        }

        const float direction = projectile.velocity.x >= 0.0f ? 1.0f : -1.0f;
        body->ApplyImpulseLinear(AE::Physics::Vector2D(180.0f * direction, -36.0f));
        body->angularVelocity += 2.5f * direction;
        physicsWorld->WakeBody(*body);
        projectile.active = false;
        return;
    }
}

void PlatformerApp::StepPhysics(float dt)
{
    if (!physicsWorld)
    {
        return;
    }

    physicsSceneTime += dt;
    const int physicsSteps = std::max(1, static_cast<int>(std::ceil(dt / PhysicsFixedTimeStep)));
    const float physicsDt = dt / static_cast<float>(physicsSteps);
    for (int step = 0; step < physicsSteps; ++step)
    {
        UpdateKinematicPhysicsBodies(physicsDt);
        physicsWorld->Update(physicsDt);
    }

    ResolvePlayerPhysicsContacts();
}

void PlatformerApp::UpdateKinematicPhysicsBodies(float dt)
{
    if (!physicsWorld)
    {
        return;
    }

    bool moved = false;
    for (KinematicBody& kinematic : kinematicBodies)
    {
        if (!kinematic.body)
        {
            continue;
        }

        const float offset = std::sin(physicsSceneTime * kinematic.speed + kinematic.phase) * kinematic.amplitude;
        const AE::Physics::Vector2D previousPosition = kinematic.body->position;
        kinematic.body->position = kinematic.basePosition + kinematic.axis * offset;
        kinematic.body->velocity = (kinematic.body->position - previousPosition) * (1.0f / std::max(0.0001f, dt));
        kinematic.body->shape->UpdateVertices(kinematic.body->rotation, kinematic.body->position);
        moved = moved || (kinematic.body->position - previousPosition).MagnitudeSquared() > 0.0001f;
    }

    if (moved)
    {
        physicsWorld->WakeAllBodies();
    }
}

void PlatformerApp::ResolvePlayerPhysicsContacts()
{
    if (!physicsWorld)
    {
        return;
    }

    RectF playerBounds = PlayerRect();
    for (BodyVisual& visual : physicsVisuals)
    {
        AE::Physics::Body* body = visual.body;
        if (!body || body->IsStatic() || !visual.gameplayBody)
        {
            continue;
        }

        const RectF bodyBounds = BodyBounds(*body);
        if (!Intersects(playerBounds, bodyBounds))
        {
            continue;
        }

        const float playerBottom = playerBounds.y + playerBounds.h;
        const float bodyTop = bodyBounds.y;
        const float overlapFromTop = playerBottom - bodyTop;
        const bool landingOnBody = player.velocity.y >= 0.0f && playerBounds.y < bodyTop && overlapFromTop >= 0.0f && overlapFromTop < 18.0f;

        if (landingOnBody)
        {
            player.position.y = bodyTop - player.height;
            player.velocity.y = 0.0f;
            player.grounded = true;
            coyoteTimer = CoyoteTime;
            body->ApplyImpulseLinear(AE::Physics::Vector2D(player.velocity.x * 0.08f, 0.0f));
        }
        else
        {
            const float playerCenterX = playerBounds.x + playerBounds.w * 0.5f;
            const float bodyCenterX = bodyBounds.x + bodyBounds.w * 0.5f;
            const float pushDirection = playerCenterX < bodyCenterX ? 1.0f : -1.0f;
            body->ApplyImpulseLinear(AE::Physics::Vector2D(pushDirection * 95.0f, -18.0f));
            body->angularVelocity += pushDirection * 1.8f;
            player.position.x -= pushDirection * 2.0f;
        }

        physicsWorld->WakeBody(*body);
        playerBounds = PlayerRect();
    }
}

void PlatformerApp::UpdateCoins()
{
    const RectF playerBounds = PlayerRect();
    for (Coin& coin : coins)
    {
        if (!coin.collected && Intersects(playerBounds, coin.bounds))
        {
            coin.collected = true;
        }
    }
}

void PlatformerApp::UpdateHazards()
{
    const RectF playerBounds = PlayerRect();
    for (const Enemy& enemy : enemies)
    {
        if (!enemy.alive)
        {
            continue;
        }
        if (Intersects(playerBounds, EnemyRect(enemy)))
        {
            ResetPlayer();
            return;
        }
    }
}

void PlatformerApp::UpdateGoal()
{
    if (Intersects(PlayerRect(), finish))
    {
        player.won = true;
        player.velocity = AE::Physics::Vector2D::Zero;
    }
}

void PlatformerApp::ResolveTileCollisions(bool horizontal)
{
    RectF playerBounds = PlayerRect();
    const int minTileX = ClampInt(static_cast<int>(std::floor(playerBounds.x / TileSize)) - 1, 0, LevelCols - 1);
    const int maxTileX = ClampInt(static_cast<int>(std::floor((playerBounds.x + playerBounds.w) / TileSize)) + 1, 0, LevelCols - 1);
    const int minTileY = ClampInt(static_cast<int>(std::floor(playerBounds.y / TileSize)) - 1, 0, LevelRows - 1);
    const int maxTileY = ClampInt(static_cast<int>(std::floor((playerBounds.y + playerBounds.h) / TileSize)) + 1, 0, LevelRows - 1);

    for (int tileY = minTileY; tileY <= maxTileY; ++tileY)
    {
        for (int tileX = minTileX; tileX <= maxTileX; ++tileX)
        {
            if (!IsSolidTile(tileX, tileY))
            {
                continue;
            }

            const RectF tileBounds{
                static_cast<float>(tileX * TileSize),
                static_cast<float>(tileY * TileSize),
                static_cast<float>(TileSize),
                static_cast<float>(TileSize)
            };

            if (!Intersects(playerBounds, tileBounds))
            {
                continue;
            }

            if (horizontal)
            {
                if (player.velocity.x > 0.0f)
                {
                    player.position.x = tileBounds.x - player.width;
                }
                else if (player.velocity.x < 0.0f)
                {
                    player.position.x = tileBounds.x + tileBounds.w;
                }
                player.velocity.x = 0.0f;
            }
            else
            {
                if (player.velocity.y > 0.0f)
                {
                    player.position.y = tileBounds.y - player.height;
                    player.grounded = true;
                }
                else if (player.velocity.y < 0.0f)
                {
                    player.position.y = tileBounds.y + tileBounds.h;
                }
                player.velocity.y = 0.0f;
            }

            playerBounds = PlayerRect();
        }
    }
}

void PlatformerApp::UpdateCamera()
{
    const float targetCameraX = player.position.x + player.width * 0.5f - static_cast<float>(WindowWidth) * 0.45f;
    const float maxCameraX = static_cast<float>(LevelCols * TileSize - WindowWidth);
    cameraX = ClampFloat(targetCameraX, 0.0f, std::max(0.0f, maxCameraX));
}

bool PlatformerApp::IsSolidTile(int tileX, int tileY) const
{
    if (tileX < 0 || tileX >= LevelCols || tileY < 0 || tileY >= LevelRows)
    {
        return false;
    }
    return levelTiles[tileY][tileX] == '#';
}

PlatformerApp::RectF PlatformerApp::PlayerRect() const
{
    return RectF{player.position.x, player.position.y, player.width, player.height};
}

PlatformerApp::RectF PlatformerApp::EnemyRect(const Enemy& enemy) const
{
    return RectF{enemy.position.x, enemy.position.y, enemy.width, enemy.height};
}

PlatformerApp::RectF PlatformerApp::ProjectileRect(const Projectile& projectile) const
{
    return RectF{projectile.position.x, projectile.position.y, projectile.width, projectile.height};
}

PlatformerApp::RectF PlatformerApp::BodyBounds(const AE::Physics::Body& body) const
{
    if (!body.shape)
    {
        return RectF{body.position.x, body.position.y, 0.0f, 0.0f};
    }

    if (body.shape->GetType() == AE::Physics::CIRCLE)
    {
        const AE::Physics::CircleShape* circle = static_cast<const AE::Physics::CircleShape*>(body.shape);
        return RectF{
            body.position.x - circle->radius,
            body.position.y - circle->radius,
            circle->radius * 2.0f,
            circle->radius * 2.0f
        };
    }

    const AE::Physics::PolygonShape* polygon = static_cast<const AE::Physics::PolygonShape*>(body.shape);
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    for (const AE::Physics::Vector2D& vertex : polygon->worldVertices)
    {
        minX = std::min(minX, vertex.x);
        minY = std::min(minY, vertex.y);
        maxX = std::max(maxX, vertex.x);
        maxY = std::max(maxY, vertex.y);
    }

    return RectF{minX, minY, maxX - minX, maxY - minY};
}

int PlatformerApp::AliveEnemyCount() const
{
    int aliveEnemies = 0;
    for (const Enemy& enemy : enemies)
    {
        if (enemy.alive)
        {
            ++aliveEnemies;
        }
    }
    return aliveEnemies;
}

int PlatformerApp::PhysicsBodyCount() const
{
    return physicsWorld ? static_cast<int>(physicsWorld->GetBodies().size()) : 0;
}

int PlatformerApp::PhysicsConstraintCount() const
{
    return physicsWorld ? static_cast<int>(physicsWorld->GetConstraints().size()) : 0;
}

bool PlatformerApp::Intersects(const RectF& first, const RectF& second)
{
    return first.x < second.x + second.w &&
        first.x + first.w > second.x &&
        first.y < second.y + second.h &&
        first.y + first.h > second.y;
}

int PlatformerApp::ClampInt(int value, int minValue, int maxValue)
{
    return std::max(minValue, std::min(maxValue, value));
}

float PlatformerApp::ClampFloat(float value, float minValue, float maxValue)
{
    return std::max(minValue, std::min(maxValue, value));
}

float PlatformerApp::MoveTowardZero(float value, float amount)
{
    if (std::abs(value) <= amount)
    {
        return 0.0f;
    }
    return value > 0.0f ? value - amount : value + amount;
}

AE::Physics::Body* PlatformerApp::AddPhysicsBox(
    AE::Physics::Vector2D position,
    float width,
    float height,
    float mass,
    SDL_Color fill,
    SDL_Color edge,
    float rotation,
    bool gameplayBody)
{
    AE::Physics::BoxShape shape(width, height);
    AE::Physics::Body* body = new AE::Physics::Body(shape, position.x, position.y, mass);
    body->rotation = rotation;
    body->collisionCategory = mass == 0.0f ? PhysicsCategoryTerrain : PhysicsCategoryDynamic;
    body->collisionMask = PhysicsCategoryTerrain | PhysicsCategoryDynamic | PhysicsCategorySensor;
    body->shape->UpdateVertices(body->rotation, body->position);
    if (physicsWorld)
    {
        physicsWorld->AddBody(body);
    }
    physicsVisuals.push_back(BodyVisual{body, fill, edge, gameplayBody});
    return body;
}

AE::Physics::Body* PlatformerApp::AddPhysicsCircle(
    AE::Physics::Vector2D position,
    float radius,
    float mass,
    SDL_Color fill,
    SDL_Color edge,
    bool gameplayBody)
{
    AE::Physics::CircleShape shape(radius);
    AE::Physics::Body* body = new AE::Physics::Body(shape, position.x, position.y, mass);
    body->collisionCategory = mass == 0.0f ? PhysicsCategoryTerrain : PhysicsCategoryDynamic;
    body->collisionMask = PhysicsCategoryTerrain | PhysicsCategoryDynamic | PhysicsCategorySensor;
    if (physicsWorld)
    {
        physicsWorld->AddBody(body);
    }
    physicsVisuals.push_back(BodyVisual{body, fill, edge, gameplayBody});
    return body;
}

void PlatformerApp::AddPhysicsJoint(AE::Physics::Body* first, AE::Physics::Body* second)
{
    if (!physicsWorld || !first || !second)
    {
        return;
    }

    physicsWorld->AddConstraint(new AE::Physics::JointConstraint(first, second, (first->position + second->position) * 0.5f));
}

void PlatformerApp::Render()
{
    if (!renderer || !frameTexture)
    {
        return;
    }

    const Uint64 renderStart = SDL_GetPerformanceCounter();

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
    diagnostics.BeginFrame();
#endif

    BeginFrameTexture();
    DrawBackground();
    DrawLevel();
    DrawPhysics();
    DrawCoins();
    DrawFinish();
    DrawProjectiles();
    DrawEnemies();
    DrawPlayer();
    DrawHud();
    RenderDiagnostics();

    lastRenderMs = ElapsedMs(renderStart, SDL_GetPerformanceCounter());
    PresentFrameTexture();
}

bool PlatformerApp::CreateFrameTexture()
{
    frameTexture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        WindowWidth,
        WindowHeight);
    if (!frameTexture)
    {
        std::cerr << "SDL_CreateTexture frame target failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetTextureBlendMode(frameTexture, SDL_BLENDMODE_NONE);
#if SDL_VERSION_ATLEAST(2, 0, 12)
    SDL_SetTextureScaleMode(frameTexture, SDL_ScaleModeLinear);
#endif
    return true;
}

void PlatformerApp::BeginFrameTexture()
{
    SDL_SetRenderTarget(renderer, frameTexture);
    SDL_RenderSetLogicalSize(renderer, 0, 0);
    SDL_RenderSetViewport(renderer, nullptr);
    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
}

void PlatformerApp::PresentFrameTexture()
{
    SDL_SetRenderTarget(renderer, nullptr);
    SDL_RenderSetLogicalSize(renderer, 0, 0);
    SDL_RenderSetViewport(renderer, nullptr);
    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    const SDL_Rect destination = CalculateFrameViewport();
    SDL_RenderCopy(renderer, frameTexture, nullptr, &destination);
    SDL_RenderPresent(renderer);
}

SDL_Rect PlatformerApp::CalculateFrameViewport() const
{
    int outputWidth = WindowWidth;
    int outputHeight = WindowHeight;
    SDL_GetRendererOutputSize(renderer, &outputWidth, &outputHeight);

    const float scale = std::min(
        static_cast<float>(outputWidth) / static_cast<float>(WindowWidth),
        static_cast<float>(outputHeight) / static_cast<float>(WindowHeight));
    const int width = static_cast<int>(std::round(static_cast<float>(WindowWidth) * scale));
    const int height = static_cast<int>(std::round(static_cast<float>(WindowHeight) * scale));
    return SDL_Rect{(outputWidth - width) / 2, (outputHeight - height) / 2, width, height};
}

void PlatformerApp::RenderDiagnostics()
{
#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
    int collectedCoins = 0;
    for (const Coin& coin : coins)
    {
        if (coin.collected)
        {
            ++collectedCoins;
        }
    }

    AE::Editor::SampleDiagnosticsData data;
    data.sampleName = "PlatformerApp";
    data.paused = paused;
    data.fixedSteps = fixedStepsThisFrame;
    data.updateMs = lastUpdateMs;
    data.renderMs = lastRenderMs;
    data.statusText =
        "Player x " + std::to_string(static_cast<int>(player.position.x)) +
        " y " + std::to_string(static_cast<int>(player.position.y)) +
        " | coins " + std::to_string(collectedCoins) + "/" + std::to_string(coins.size()) +
        " | enemies " + std::to_string(AliveEnemyCount()) + "/" + std::to_string(enemies.size()) +
        " | bodies " + std::to_string(PhysicsBodyCount());

    diagnostics.Draw(data, [this, collectedCoins]()
    {
        if (ImGui::Button(paused ? "Resume" : "Pause"))
        {
            paused = !paused;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset"))
        {
            ResetLevel();
        }
        ImGui::SameLine();
        if (ImGui::Button(fullscreen ? "Windowed" : "Fullscreen"))
        {
            ToggleFullscreen();
        }

        ImGui::Checkbox("Output Log", &diagnostics.ShowOutputLog());
        ImGui::Separator();
        ImGui::Text("Coins: %d / %zu", collectedCoins, coins.size());
        ImGui::Text("Enemies: %d / %zu", AliveEnemyCount(), enemies.size());
        ImGui::Text("Projectiles: %zu", projectiles.size());
        ImGui::Text("Scripted enemies: %s", scriptedEnemiesLoaded ? "yes" : "no");
        ImGui::Text("Physics bodies: %d", PhysicsBodyCount());
        ImGui::Text("Physics constraints: %d", PhysicsConstraintCount());
        if (physicsWorld)
        {
            const AE::Physics::WorldStats& stats = physicsWorld->GetLastStats();
            ImGui::Text("Physics contacts: %zu", stats.contactCount);
            ImGui::Text("Physics step: %.3f ms", stats.totalStepMs);
        }
        ImGui::Text("Grounded: %s", player.grounded ? "yes" : "no");
        ImGui::Text("Won: %s", player.won ? "yes" : "no");
        ImGui::Text("Camera X: %.1f", cameraX);
    });
    diagnostics.Render();
#endif
}

void PlatformerApp::DrawBackground() const
{
    DrawScreenRect(0, 0, WindowWidth, WindowHeight / 2, SkyTop);
    DrawScreenRect(0, WindowHeight / 2, WindowWidth, WindowHeight / 2, SkyBottom);

    const int hillOffset = static_cast<int>(cameraX * 0.25f) % 260;
    for (int x = -260 - hillOffset; x < WindowWidth + 260; x += 260)
    {
        DrawScreenRect(x + 10, 370, 220, 120, SDL_Color{87, 166, 100, 255});
        DrawScreenRect(x + 70, 330, 160, 160, SDL_Color{99, 185, 112, 255});
    }

    const int cloudOffset = static_cast<int>(cameraX * 0.12f) % 340;
    for (int x = -340 - cloudOffset; x < WindowWidth + 340; x += 340)
    {
        DrawScreenRect(x + 50, 70, 110, 24, SDL_Color{245, 248, 252, 255});
        DrawScreenRect(x + 80, 48, 66, 34, SDL_Color{245, 248, 252, 255});
        DrawScreenRect(x + 150, 82, 72, 20, SDL_Color{245, 248, 252, 255});
    }
}

void PlatformerApp::DrawLevel() const
{
    const int firstTileX = ClampInt(static_cast<int>(cameraX / TileSize) - 1, 0, LevelCols - 1);
    const int lastTileX = ClampInt(static_cast<int>((cameraX + WindowWidth) / TileSize) + 1, 0, LevelCols - 1);

    for (int y = 0; y < LevelRows; ++y)
    {
        for (int x = firstTileX; x <= lastTileX; ++x)
        {
            if (!IsSolidTile(x, y))
            {
                continue;
            }

            const RectF tile{
                static_cast<float>(x * TileSize),
                static_cast<float>(y * TileSize),
                static_cast<float>(TileSize),
                static_cast<float>(TileSize)
            };
            DrawRect(tile, y >= 15 ? Dirt : Brick);

            const int screenX = static_cast<int>(tile.x - cameraX);
            const int screenY = static_cast<int>(tile.y);
            DrawScreenRect(screenX, screenY, TileSize, 4, Ground);
            DrawScreenRect(screenX, screenY, 2, TileSize, SDL_Color{205, 137, 96, 255});
        }
    }
}

void PlatformerApp::DrawCoins() const
{
    for (const Coin& coin : coins)
    {
        if (coin.collected)
        {
            continue;
        }

        DrawRect(coin.bounds, CoinColor);
        DrawRect(RectF{coin.bounds.x + 6.0f, coin.bounds.y + 3.0f, 4.0f, 12.0f}, SDL_Color{255, 236, 132, 255});
    }
}

void PlatformerApp::DrawEnemies() const
{
    for (const Enemy& enemy : enemies)
    {
        if (!enemy.alive)
        {
            continue;
        }

        const RectF bounds = EnemyRect(enemy);
        DrawRect(bounds, enemy.color);
        DrawRect(RectF{bounds.x + 6.0f, bounds.y + 6.0f, 5.0f, 5.0f}, SDL_Color{255, 245, 220, 255});
        DrawRect(RectF{bounds.x + bounds.w - 11.0f, bounds.y + 6.0f, 5.0f, 5.0f}, SDL_Color{255, 245, 220, 255});
        DrawRect(RectF{bounds.x, bounds.y + bounds.h - 5.0f, bounds.w, 5.0f}, SDL_Color{110, 36, 50, 255});

        const float healthRatio = ClampFloat(
            static_cast<float>(enemy.health) / static_cast<float>(std::max(1, enemy.maxHealth)),
            0.0f,
            1.0f);
        DrawRect(RectF{bounds.x, bounds.y - 7.0f, bounds.w, 3.0f}, SDL_Color{70, 45, 48, 220});
        DrawRect(RectF{bounds.x, bounds.y - 7.0f, bounds.w * healthRatio, 3.0f}, SDL_Color{245, 196, 48, 255});
    }
}

void PlatformerApp::DrawProjectiles() const
{
    for (const Projectile& projectile : projectiles)
    {
        DrawRect(ProjectileRect(projectile), ProjectileColor);
        DrawRect(
            RectF{projectile.position.x - (projectile.velocity.x > 0.0f ? 7.0f : -7.0f), projectile.position.y + 2.0f, 8.0f, 2.0f},
            SDL_Color{255, 255, 245, 170});
    }
}

void PlatformerApp::DrawPhysics() const
{
    if (!physicsWorld)
    {
        return;
    }

    for (const AE::Physics::Constraint* constraint : physicsWorld->GetConstraints())
    {
        if (constraint && constraint->a && constraint->b)
        {
            DrawWorldLine(constraint->a->position, constraint->b->position, PhysicsJointColor);
        }
    }

    for (const BodyVisual& visual : physicsVisuals)
    {
        if (!visual.body || !visual.body->shape)
        {
            continue;
        }

        if (visual.body->shape->GetType() == AE::Physics::CIRCLE)
        {
            const AE::Physics::CircleShape* circle = static_cast<const AE::Physics::CircleShape*>(visual.body->shape);
            DrawFilledCircle(
                RoundToInt(visual.body->position.x - cameraX),
                RoundToInt(visual.body->position.y),
                RoundToInt(circle->radius),
                visual.fill);
            DrawFilledCircle(
                RoundToInt(visual.body->position.x - cameraX),
                RoundToInt(visual.body->position.y),
                2,
                visual.edge);
        }
        else
        {
            const AE::Physics::PolygonShape* polygon = static_cast<const AE::Physics::PolygonShape*>(visual.body->shape);
            std::vector<AE::Physics::Vector2D> screenVertices;
            screenVertices.reserve(polygon->worldVertices.size());
            for (const AE::Physics::Vector2D& vertex : polygon->worldVertices)
            {
                screenVertices.emplace_back(vertex.x - cameraX, vertex.y);
            }
            DrawFilledPolygon(screenVertices, visual.fill);
            DrawPolyline(screenVertices, visual.edge, true);
        }
    }

    for (const AE::Physics::Contact& contact : physicsWorld->GetContacts())
    {
        DrawWorldLine(contact.start, contact.end, SDL_Color{255, 245, 145, 160});
        DrawFilledCircle(RoundToInt(contact.start.x - cameraX), RoundToInt(contact.start.y), 2, SDL_Color{255, 245, 145, 190});
    }
}

void PlatformerApp::DrawPlayer() const
{
    const RectF bounds = PlayerRect();
    DrawRect(bounds, PlayerBody);
    DrawRect(RectF{bounds.x + 5.0f, bounds.y + 6.0f, bounds.w - 10.0f, 10.0f}, PlayerAccent);
    DrawRect(RectF{bounds.x + 7.0f, bounds.y + 18.0f, 6.0f, 6.0f}, SDL_Color{20, 35, 48, 255});
    DrawRect(RectF{bounds.x + bounds.w - 13.0f, bounds.y + 18.0f, 6.0f, 6.0f}, SDL_Color{20, 35, 48, 255});
    DrawRect(RectF{bounds.x + 6.0f, bounds.y + bounds.h - 7.0f, 8.0f, 7.0f}, SDL_Color{24, 70, 105, 255});
    DrawRect(RectF{bounds.x + bounds.w - 14.0f, bounds.y + bounds.h - 7.0f, 8.0f, 7.0f}, SDL_Color{24, 70, 105, 255});
}

void PlatformerApp::DrawFinish() const
{
    DrawRect(RectF{finish.x + 4.0f, finish.y, 5.0f, finish.h}, SDL_Color{48, 62, 74, 255});
    DrawRect(RectF{finish.x + 9.0f, finish.y + 6.0f, 34.0f, 22.0f}, player.won ? SDL_Color{245, 196, 48, 255} : SDL_Color{220, 68, 78, 255});
}

void PlatformerApp::DrawHud() const
{
    DrawScreenRect(18, 18, 244, 42, SDL_Color{24, 40, 56, 220});

    int collected = 0;
    for (const Coin& coin : coins)
    {
        if (coin.collected)
        {
            ++collected;
        }
    }

    for (int index = 0; index < static_cast<int>(coins.size()); ++index)
    {
        const SDL_Color color = index < collected ? CoinColor : SDL_Color{83, 99, 114, 255};
        DrawScreenRect(34 + index * 24, 30, 14, 14, color);
    }

    const int aliveEnemies = AliveEnemyCount();
    for (int index = 0; index < static_cast<int>(enemies.size()); ++index)
    {
        const SDL_Color color = index < aliveEnemies ? EnemyColor : SDL_Color{83, 99, 114, 255};
        DrawScreenRect(190 + index * 14, 30, 9, 14, color);
    }

    const int cooldownWidth = static_cast<int>(42.0f * (1.0f - ClampFloat(shootCooldownTimer / ProjectileCooldown, 0.0f, 1.0f)));
    DrawScreenRect(34, 52, 42, 4, SDL_Color{83, 99, 114, 210});
    DrawScreenRect(34, 52, cooldownWidth, 4, ProjectileColor);

    if (player.won)
    {
        DrawScreenRect(WindowWidth / 2 - 92, 72, 184, 38, SDL_Color{24, 40, 56, 230});
        DrawScreenRect(WindowWidth / 2 - 66, 85, 132, 12, SDL_Color{245, 196, 48, 255});
    }
}

void PlatformerApp::DrawRect(const RectF& rect, SDL_Color color) const
{
    DrawScreenRect(
        static_cast<int>(std::round(rect.x - cameraX)),
        static_cast<int>(std::round(rect.y)),
        static_cast<int>(std::round(rect.w)),
        static_cast<int>(std::round(rect.h)),
        color);
}

void PlatformerApp::DrawWorldLine(const AE::Physics::Vector2D& from, const AE::Physics::Vector2D& to, SDL_Color color) const
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(
        renderer,
        RoundToInt(from.x - cameraX),
        RoundToInt(from.y),
        RoundToInt(to.x - cameraX),
        RoundToInt(to.y));
}

void PlatformerApp::DrawFilledCircle(int centerX, int centerY, int radius, SDL_Color color) const
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int y = -radius; y <= radius; ++y)
    {
        const int span = static_cast<int>(std::sqrt(static_cast<float>(radius * radius - y * y)));
        SDL_RenderDrawLine(renderer, centerX - span, centerY + y, centerX + span, centerY + y);
    }
}

void PlatformerApp::DrawFilledPolygon(const std::vector<AE::Physics::Vector2D>& vertices, SDL_Color color) const
{
    if (vertices.size() < 3)
    {
        return;
    }

    float minY = vertices.front().y;
    float maxY = vertices.front().y;
    for (const AE::Physics::Vector2D& vertex : vertices)
    {
        minY = std::min(minY, vertex.y);
        maxY = std::max(maxY, vertex.y);
    }

    const int startY = std::max(0, static_cast<int>(std::ceil(minY)));
    const int endY = std::min(WindowHeight - 1, static_cast<int>(std::floor(maxY)));

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    std::vector<float> intersections;
    intersections.reserve(vertices.size());
    for (int y = startY; y <= endY; ++y)
    {
        intersections.clear();
        const float scanY = static_cast<float>(y) + 0.5f;

        for (std::size_t i = 0; i < vertices.size(); ++i)
        {
            const AE::Physics::Vector2D& a = vertices[i];
            const AE::Physics::Vector2D& b = vertices[(i + 1) % vertices.size()];
            if ((a.y <= scanY && b.y > scanY) || (b.y <= scanY && a.y > scanY))
            {
                const float t = (scanY - a.y) / (b.y - a.y);
                intersections.push_back(a.x + t * (b.x - a.x));
            }
        }

        std::sort(intersections.begin(), intersections.end());
        for (std::size_t i = 0; i + 1 < intersections.size(); i += 2)
        {
            const int x0 = std::max(0, static_cast<int>(std::ceil(intersections[i])));
            const int x1 = std::min(WindowWidth - 1, static_cast<int>(std::floor(intersections[i + 1])));
            SDL_RenderDrawLine(renderer, x0, y, x1, y);
        }
    }
}

void PlatformerApp::DrawPolyline(const std::vector<AE::Physics::Vector2D>& vertices, SDL_Color color, bool closed) const
{
    if (vertices.size() < 2)
    {
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (std::size_t i = 0; i + 1 < vertices.size(); ++i)
    {
        SDL_RenderDrawLine(
            renderer,
            RoundToInt(vertices[i].x),
            RoundToInt(vertices[i].y),
            RoundToInt(vertices[i + 1].x),
            RoundToInt(vertices[i + 1].y));
    }
    if (closed)
    {
        SDL_RenderDrawLine(
            renderer,
            RoundToInt(vertices.back().x),
            RoundToInt(vertices.back().y),
            RoundToInt(vertices.front().x),
            RoundToInt(vertices.front().y));
    }
}

void PlatformerApp::DrawScreenRect(int x, int y, int w, int h, SDL_Color color) const
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect{x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
}
