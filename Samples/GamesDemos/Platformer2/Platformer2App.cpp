#include "Platformer2App.h"

#include <SDL2/SDL_image.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef AMBER_ENABLE_PLATFORMER2_EDITOR
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_sdl.h"
#endif

namespace
{
    constexpr float FixedTimeStep = 1.0f / 60.0f;
    constexpr float Gravity = 1850.0f;
    constexpr float MoveAcceleration = 3800.0f;
    constexpr float GroundFriction = 4700.0f;
    constexpr float MaxRunSpeed = 245.0f;
    constexpr float JumpVelocity = -640.0f;
    constexpr float JumpCutVelocity = -250.0f;
    constexpr float MaxFallSpeed = 860.0f;
    constexpr float LadderSpeed = 155.0f;
    constexpr float CoyoteTime = 0.10f;
    constexpr float JumpBufferTime = 0.12f;
    constexpr float ProjectileSpeed = 620.0f;
    constexpr float ProjectileCooldown = 0.20f;
    constexpr int MaxAirJumps = 1;

    constexpr int TileGroundLeft = 160;
    constexpr int TileGroundMiddle = 161;
    constexpr int TileGroundRight = 162;
    constexpr int TileGroundFill = 216;
    constexpr int TileLadder = 80;
    constexpr int TileSpike = 183;
    constexpr int TileCoin = 20;
    constexpr int TileDoorTopLeft = 56;
    constexpr int TileDoorTopRight = 57;
    constexpr int TileDoorBottomLeft = 76;
    constexpr int TileDoorBottomRight = 77;
    constexpr int TileLiftLeft = 123;
    constexpr int TileLiftMiddle = 124;
    constexpr int TileLiftRight = 126;
    constexpr int TilePlayerIdle = 240;
    constexpr int TilePlayerWalkStart = 240;
    constexpr int TilePlayerJump = 246;
    constexpr int TilePlayerClimbStart = 260;
    constexpr int TileEnemyWalkStart = 320;
    constexpr int TileProjectile = 0;
    constexpr int TileHeart = 40;

    SDL_Color BackgroundTop{32, 35, 36, 255};
    SDL_Color BackgroundBottom{18, 20, 22, 255};
    SDL_Color ProjectileColor{255, 255, 255, 255};
    SDL_Color HudBack{0, 0, 0, 120};

    int RoundToInt(float value)
    {
        return static_cast<int>(std::round(value));
    }

    std::filesystem::path FindPlatformer2Asset(const std::filesystem::path& relativeContentPath)
    {
        namespace fs = std::filesystem;

        const fs::path relativePath = fs::path("Samples") / "GamesDemos" / "Platformer2" /
            "Content" / relativeContentPath;
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

    std::filesystem::path FindPlatformer2ContentRoot()
    {
        namespace fs = std::filesystem;

        const fs::path relativePath = fs::path("Samples") / "GamesDemos" / "Platformer2" / "Content";
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
            if (fs::exists(candidate, error) && fs::is_directory(candidate, error))
            {
                return fs::weakly_canonical(candidate, error);
            }
        }

        return {};
    }

    std::filesystem::path Platformer2MapPath()
    {
        namespace fs = std::filesystem;

        const fs::path existingMap = FindPlatformer2Asset(fs::path("Maps") / "Platformer2Level.txt");
        if (!existingMap.empty())
        {
            return existingMap;
        }

        const fs::path contentRoot = FindPlatformer2ContentRoot();
        if (!contentRoot.empty())
        {
            return contentRoot / "Maps" / "Platformer2Level.txt";
        }

        return fs::current_path() / "Samples" / "GamesDemos" / "Platformer2" / "Content" / "Maps" / "Platformer2Level.txt";
    }

    bool IsFullscreenToggleKey(const SDL_KeyboardEvent& keyEvent)
    {
        const SDL_Keycode key = keyEvent.keysym.sym;
        const bool altPressed = (keyEvent.keysym.mod & KMOD_ALT) != 0;
        return key == SDLK_F11 || (altPressed && (key == SDLK_RETURN || key == SDLK_KP_ENTER));
    }
}

Platformer2App::Platformer2App()
{
    ResetGame();
}

int Platformer2App::Run()
{
    if (!Initialize())
    {
        return 1;
    }

    running = true;
    InputState input;
    Uint64 previousCounter = SDL_GetPerformanceCounter();
    float accumulator = 0.0f;

    while (running)
    {
        const Uint64 currentCounter = SDL_GetPerformanceCounter();
        const float dt = static_cast<float>(currentCounter - previousCounter) /
            static_cast<float>(SDL_GetPerformanceFrequency());
        previousCounter = currentCounter;
        accumulator = std::min(accumulator + dt, 0.25f);

        PollEvents(input);
        InputState stepInput = input;
        bool stepped = false;
        while (!paused && accumulator >= FixedTimeStep)
        {
            Step(FixedTimeStep, stepInput);
            stepInput.jumpPressed = false;
            stepInput.shootPressed = false;
            accumulator -= FixedTimeStep;
            stepped = true;
        }
        if (stepped)
        {
            input.jumpPressed = false;
            input.shootPressed = false;
            pendingJumpPressed = false;
            pendingShootPressed = false;
        }

        Render();
    }

    Shutdown();
    return 0;
}

#if SMOKE_TEST
bool Platformer2App::RunSmokeTest()
{
    forceDefaultMap = true;
    ResetGame();

    const float startX = player.position.x;
    InputState runRight;
    runRight.moveRight = true;
    for (int frame = 0; frame < 50; ++frame)
    {
        runRight.jumpPressed = frame == 12;
        runRight.jumpHeld = frame >= 12 && frame < 32;
        Step(FixedTimeStep, runRight);
        runRight.jumpPressed = false;
    }
    const bool movementWorks = player.position.x > startX + 75.0f && player.position.y < LevelRows * WorldTileSize;

    ResetGame();
    player.position = Vec2{15.0f * WorldTileSize + 5.0f, 17.0f * WorldTileSize};
    player.velocity = Vec2{};
    const float ladderStartY = player.position.y;
    InputState climb;
    climb.climbUp = true;
    for (int frame = 0; frame < 38; ++frame)
    {
        Step(FixedTimeStep, climb);
    }
    const bool ladderWorks = player.position.y < ladderStartY - 45.0f;

    ResetGame();
    player.position = Vec2{16.0f * WorldTileSize + 4.0f, 18.0f * WorldTileSize + 2.0f};
    UpdatePickupsAndHazards();
    const bool spikesReset = std::abs(player.position.x - playerSpawn.x) < 0.5f &&
        std::abs(player.position.y - playerSpawn.y) < 0.5f;

    ResetGame();
    bool liftWorks = false;
    if (!lifts.empty())
    {
        const Lift& lift = lifts.front();
        player.position = Vec2{lift.position.x + 28.0f, lift.position.y - player.height};
        player.velocity = Vec2{};
        const float liftStartY = player.position.y;
        for (int frame = 0; frame < 120; ++frame)
        {
            Step(FixedTimeStep, InputState{});
        }
        liftWorks = std::abs(player.position.y - liftStartY) > 12.0f || player.grounded;
    }

    ResetGame();
    bool projectileWorks = false;
    if (!enemies.empty())
    {
        Enemy& target = enemies.front();
        player.position = Vec2{target.position.x - 130.0f, target.position.y};
        player.velocity = Vec2{};
        player.facing = 1;

        InputState shoot;
        for (int shot = 0; shot < 3 && target.alive; ++shot)
        {
            shoot.shootPressed = true;
            Step(FixedTimeStep, shoot);
            shoot.shootPressed = false;
            for (int frame = 0; frame < 24 && target.alive; ++frame)
            {
                Step(FixedTimeStep, shoot);
            }
        }
        projectileWorks = !target.alive || target.health < 2;
    }

    ResetGame();
    player.position = Vec2{finish.x, finish.y};
    UpdatePickupsAndHazards();
    const bool goalWorks = player.won;

    const bool passed = movementWorks && ladderWorks && spikesReset && liftWorks && projectileWorks &&
        goalWorks && enemies.size() >= 4 && lifts.size() >= 3;
    if (!passed)
    {
        std::cerr
            << "Platformer2 smoke diagnostics:"
            << " movementWorks=" << movementWorks
            << " ladderWorks=" << ladderWorks
            << " spikesReset=" << spikesReset
            << " liftWorks=" << liftWorks
            << " projectileWorks=" << projectileWorks
            << " goalWorks=" << goalWorks
            << " enemies=" << enemies.size()
            << " lifts=" << lifts.size()
            << std::endl;
    }

    forceDefaultMap = false;
    return passed;
}
#endif

bool Platformer2App::Initialize()
{
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0)
    {
        std::cerr << "IMG_Init failed: " << IMG_GetError() << std::endl;
        Shutdown();
        return false;
    }
    imageSystemInitialized = true;

    window = SDL_CreateWindow(
        "Platformer2 Sample",
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

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer)
    {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        Shutdown();
        return false;
    }

    SDL_RenderSetLogicalSize(renderer, WindowWidth, WindowHeight);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
#ifdef AMBER_ENABLE_PLATFORMER2_EDITOR
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForD3D(window);
    ImGuiSDL::Initialize(renderer, WindowWidth, WindowHeight);
    imguiReady = true;
#endif
    if (!LoadContent())
    {
        Shutdown();
        return false;
    }

    ResetGame();
    return true;
}

bool Platformer2App::LoadContent()
{
    const std::filesystem::path tilemapPath = FindPlatformer2Asset(
        std::filesystem::path("Tilemap") / "monochrome_tilemap_transparent_packed.png");
    if (tilemapPath.empty())
    {
        std::cerr << "Platformer2 tilemap was not found." << std::endl;
        return false;
    }

    SDL_Surface* surface = IMG_Load(tilemapPath.string().c_str());
    if (!surface)
    {
        std::cerr << "Failed to load Platformer2 tilemap: " << IMG_GetError() << std::endl;
        return false;
    }

    tilemapTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!tilemapTexture)
    {
        std::cerr << "Failed to create Platformer2 tilemap texture: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetTextureBlendMode(tilemapTexture, SDL_BLENDMODE_BLEND);
    return true;
}

void Platformer2App::Shutdown()
{
    if (tilemapTexture)
    {
        SDL_DestroyTexture(tilemapTexture);
        tilemapTexture = nullptr;
    }
#ifdef AMBER_ENABLE_PLATFORMER2_EDITOR
    if (imguiReady)
    {
        ImGuiSDL::Deinitialize();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        imguiReady = false;
    }
#endif
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
    if (imageSystemInitialized)
    {
        IMG_Quit();
        imageSystemInitialized = false;
    }
    SDL_Quit();
}

void Platformer2App::ToggleFullscreen()
{
    if (!window)
    {
        return;
    }

    fullscreen = !fullscreen;
    SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

void Platformer2App::ToggleEditorMode()
{
#ifdef AMBER_ENABLE_PLATFORMER2_EDITOR
    editorMode = !editorMode;
    if (editorMode)
    {
        editorWasPausedBeforeOpen = paused;
        paused = true;
        editorStatus = "Editor opened";
    }
    else
    {
        paused = editorWasPausedBeforeOpen;
        editorSelection = EditorSelection{};
        editorStatus = "Editor closed";
    }
#endif
}

void Platformer2App::ResetGame()
{
    BuildLevel();
    player.coins = 0;
    worldTime = 0.0f;
    coyoteTimer = 0.0f;
    jumpBufferTimer = 0.0f;
    shootCooldownTimer = 0.0f;
    pendingJumpPressed = false;
    pendingShootPressed = false;
    projectiles.clear();
    ResetPlayer();
    player.won = false;
    cameraX = 0.0f;
    cameraY = 0.0f;
}

void Platformer2App::ResetPlayer()
{
    player.position = playerSpawn;
    player.velocity = Vec2{};
    player.grounded = false;
    player.onLadder = false;
    player.airJumpsRemaining = MaxAirJumps;
    coyoteTimer = 0.0f;
    jumpBufferTimer = 0.0f;
}

void Platformer2App::BuildLevel()
{
    if (!forceDefaultMap && LoadMap())
    {
        return;
    }

    BuildDefaultLevel();
}

void Platformer2App::BuildDefaultLevel()
{
    for (auto& row : level)
    {
        for (TileCell& cell : row)
        {
            cell = TileCell{};
        }
    }

    enemies.clear();
    lifts.clear();
    finish = RectF{114.0f * WorldTileSize, 15.0f * WorldTileSize, 2.0f * WorldTileSize, 2.0f * WorldTileSize};

    FillPlatform(0, LevelCols, 19);
    FillRectTiles(0, 20, LevelCols, 1, TileKind::Solid, TileGroundFill);

    FillPlatform(4, 16, 16);
    AddCoinLine(7, 12, 15);
    AddLadder(15, 13, 18);
    AddSpikes(16, 20, 18);

    FillPlatform(20, 32, 13);
    AddSpikes(23, 27, 12);
    AddCoinLine(22, 28, 11);
    AddLadder(34, 9, 18);

    FillPlatform(10, 19, 10);
    AddCoinLine(12, 18, 8);

    FillPlatform(32, 45, 10);
    AddSpikes(36, 40, 9);
    AddCoinLine(34, 42, 8);

    AddLift(47.0f * WorldTileSize, 17.0f * WorldTileSize, Vec2{0.0f, -1.0f}, 145.0f, 1.25f, 0.0f);
    FillPlatform(55, 68, 15);
    AddSpikes(58, 62, 14);
    AddCoinLine(56, 67, 13);
    AddLadder(61, 11, 18);

    FillPlatform(63, 76, 11);
    AddCoinLine(66, 74, 9);
    AddLift(79.0f * WorldTileSize, 13.0f * WorldTileSize, Vec2{1.0f, 0.0f}, 165.0f, 0.9f, 1.2f);

    FillPlatform(92, 103, 13);
    AddCoinLine(94, 101, 11);
    AddLadder(97, 14, 18);
    AddSpikes(102, 106, 18);

    AddLift(104.0f * WorldTileSize, 16.0f * WorldTileSize, Vec2{0.0f, -1.0f}, 96.0f, 1.55f, 2.3f);
    FillPlatform(108, LevelCols, 17);
    FillRectTiles(114, 15, 2, 2, TileKind::Goal, TileDoorTopLeft);
    SetTile(114, 15, TileKind::Goal, TileDoorTopLeft);
    SetTile(115, 15, TileKind::Goal, TileDoorTopRight);
    SetTile(114, 16, TileKind::Goal, TileDoorBottomLeft);
    SetTile(115, 16, TileKind::Goal, TileDoorBottomRight);

    for (int x = 2; x < LevelCols - 3; x += 13)
    {
        SetTile(x, 5 + (x % 3), TileKind::Decor, 14 + (x % 2));
    }

    playerSpawn = Vec2{2.0f * WorldTileSize, 19.0f * WorldTileSize - player.height};

    AddEnemy(12, 16, 4, 16, 62.0f);
    AddEnemy(29, 13, 20, 32, 86.0f);
    AddEnemy(42, 10, 32, 45, 70.0f);
    AddEnemy(67, 15, 55, 68, 92.0f);
    AddEnemy(100, 13, 92, 103, 78.0f);
}

bool Platformer2App::LoadMap()
{
    const std::filesystem::path mapPath = Platformer2MapPath();
    std::ifstream input(mapPath);
    if (!input)
    {
        return false;
    }

    for (auto& row : level)
    {
        for (TileCell& cell : row)
        {
            cell = TileCell{};
        }
    }
    enemies.clear();
    lifts.clear();
    projectiles.clear();
    finish = RectF{114.0f * WorldTileSize, 15.0f * WorldTileSize, 2.0f * WorldTileSize, 2.0f * WorldTileSize};
    playerSpawn = Vec2{2.0f * WorldTileSize, 19.0f * WorldTileSize - player.height};

    std::string line;
    while (std::getline(input, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::istringstream stream(line);
        std::string token;
        stream >> token;
        if (token == "tile")
        {
            int x = 0;
            int y = 0;
            std::string kindToken;
            int visual = -1;
            stream >> x >> y >> kindToken >> visual;

            TileKind kind = TileKind::Empty;
            if (kindToken == "Solid") kind = TileKind::Solid;
            else if (kindToken == "Ladder") kind = TileKind::Ladder;
            else if (kindToken == "Spike") kind = TileKind::Spike;
            else if (kindToken == "Coin") kind = TileKind::Coin;
            else if (kindToken == "Goal") kind = TileKind::Goal;
            else if (kindToken == "Decor") kind = TileKind::Decor;

            SetTile(x, y, kind, visual);
        }
        else if (token == "enemy")
        {
            Enemy enemy;
            stream >> enemy.spawnPosition.x >> enemy.spawnPosition.y >> enemy.leftBound >> enemy.rightBound >>
                enemy.speed >> enemy.health;
            enemy.position = enemy.spawnPosition;
            enemy.velocity = Vec2{};
            enemies.push_back(enemy);
        }
        else if (token == "lift")
        {
            Lift lift;
            stream >> lift.basePosition.x >> lift.basePosition.y >> lift.axis.x >> lift.axis.y >>
                lift.width >> lift.height >> lift.amplitude >> lift.speed >> lift.phase;
            lift.previousPosition = lift.basePosition;
            lift.position = lift.basePosition;
            lifts.push_back(lift);
        }
        else if (token == "player")
        {
            stream >> playerSpawn.x >> playerSpawn.y;
        }
        else if (token == "finish")
        {
            stream >> finish.x >> finish.y >> finish.w >> finish.h;
        }
    }

    editorStatus = "Loaded " + mapPath.string();
    return true;
}

bool Platformer2App::SaveMap() const
{
    const std::filesystem::path mapPath = Platformer2MapPath();
    std::error_code error;
    std::filesystem::create_directories(mapPath.parent_path(), error);

    std::ofstream output(mapPath);
    if (!output)
    {
        return false;
    }

    output << "# Platformer2 map\n";
    output << "version 1\n";
    output << "player " << playerSpawn.x << ' ' << playerSpawn.y << '\n';
    output << "finish " << finish.x << ' ' << finish.y << ' ' << finish.w << ' ' << finish.h << '\n';

    for (int y = 0; y < LevelRows; ++y)
    {
        for (int x = 0; x < LevelCols; ++x)
        {
            const TileCell& cell = level[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
            if (cell.kind == TileKind::Empty)
            {
                continue;
            }
            output << "tile " << x << ' ' << y << ' ' << TileKindName(cell.kind) << ' ' << cell.visual << '\n';
        }
    }

    for (const Enemy& enemy : enemies)
    {
        output << "enemy " << enemy.spawnPosition.x << ' ' << enemy.spawnPosition.y << ' ' <<
            enemy.leftBound << ' ' << enemy.rightBound << ' ' << enemy.speed << ' ' << std::max(1, enemy.health) << '\n';
    }

    for (const Lift& lift : lifts)
    {
        output << "lift " << lift.basePosition.x << ' ' << lift.basePosition.y << ' ' <<
            lift.axis.x << ' ' << lift.axis.y << ' ' << lift.width << ' ' << lift.height << ' ' <<
            lift.amplitude << ' ' << lift.speed << ' ' << lift.phase << '\n';
    }

    return true;
}

void Platformer2App::SetTile(int x, int y, TileKind kind, int visual)
{
    if (x < 0 || x >= LevelCols || y < 0 || y >= LevelRows)
    {
        return;
    }

    level[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = TileCell{kind, visual};
}

void Platformer2App::FillRectTiles(int x, int y, int width, int height, TileKind kind, int visual)
{
    for (int row = y; row < y + height; ++row)
    {
        for (int col = x; col < x + width; ++col)
        {
            SetTile(col, row, kind, visual);
        }
    }
}

void Platformer2App::FillPlatform(int xStart, int xEnd, int y)
{
    for (int x = xStart; x < xEnd; ++x)
    {
        int tile = TileGroundMiddle;
        if (x == xStart)
        {
            tile = TileGroundLeft;
        }
        else if (x == xEnd - 1)
        {
            tile = TileGroundRight;
        }
        SetTile(x, y, TileKind::Solid, tile);
    }
}

void Platformer2App::AddLadder(int x, int yStart, int yEnd)
{
    for (int y = yStart; y <= yEnd; ++y)
    {
        SetTile(x, y, TileKind::Ladder, TileLadder);
    }
}

void Platformer2App::AddSpikes(int xStart, int xEnd, int y)
{
    for (int x = xStart; x < xEnd; ++x)
    {
        SetTile(x, y, TileKind::Spike, TileSpike);
    }
}

void Platformer2App::AddCoinLine(int xStart, int xEnd, int y)
{
    for (int x = xStart; x < xEnd; x += 2)
    {
        SetTile(x, y, TileKind::Coin, TileCoin);
    }
}

void Platformer2App::AddEnemy(int xTile, int platformY, int leftTile, int rightTile, float speed)
{
    AddEnemyAtWorld(
        xTile * static_cast<float>(WorldTileSize),
        platformY * static_cast<float>(WorldTileSize) - 28.0f,
        leftTile * static_cast<float>(WorldTileSize),
        rightTile * static_cast<float>(WorldTileSize),
        speed);
}

void Platformer2App::AddEnemyAtWorld(float x, float y, float leftBound, float rightBound, float speed)
{
    Enemy enemy;
    enemy.spawnPosition = Vec2{x, y};
    enemy.position = enemy.spawnPosition;
    enemy.leftBound = std::min(leftBound, rightBound - enemy.width);
    enemy.rightBound = std::max(rightBound, enemy.leftBound + enemy.width);
    enemy.speed = speed;
    enemy.facing = -1;
    enemies.push_back(enemy);
}

void Platformer2App::AddLift(float x, float y, Vec2 axis, float amplitude, float speed, float phase)
{
    AddLiftAtWorld(x, y, axis, 96.0f, 16.0f, amplitude, speed, phase);
}

void Platformer2App::AddLiftAtWorld(float x, float y, Vec2 axis, float width, float height, float amplitude, float speed, float phase)
{
    Lift lift;
    lift.basePosition = Vec2{x, y};
    lift.previousPosition = lift.basePosition;
    lift.position = lift.basePosition;
    lift.axis = axis;
    lift.width = width;
    lift.height = height;
    lift.amplitude = amplitude;
    lift.speed = speed;
    lift.phase = phase;
    lifts.push_back(lift);
}

void Platformer2App::ClearGoalTiles()
{
    for (auto& row : level)
    {
        for (TileCell& cell : row)
        {
            if (cell.kind == TileKind::Goal)
            {
                cell = TileCell{};
            }
        }
    }
}

void Platformer2App::SetGoalTilesFromFinish()
{
    ClearGoalTiles();
    const int x = ClampInt(static_cast<int>(std::floor(finish.x / WorldTileSize)), 0, LevelCols - 2);
    const int y = ClampInt(static_cast<int>(std::floor(finish.y / WorldTileSize)), 0, LevelRows - 2);
    finish = RectF{x * static_cast<float>(WorldTileSize), y * static_cast<float>(WorldTileSize), 2.0f * WorldTileSize, 2.0f * WorldTileSize};
    SetTile(x, y, TileKind::Goal, TileDoorTopLeft);
    SetTile(x + 1, y, TileKind::Goal, TileDoorTopRight);
    SetTile(x, y + 1, TileKind::Goal, TileDoorBottomLeft);
    SetTile(x + 1, y + 1, TileKind::Goal, TileDoorBottomRight);
}

void Platformer2App::PollEvents(InputState& input)
{
    input.jumpPressed = false;
    input.shootPressed = false;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
#ifdef AMBER_ENABLE_PLATFORMER2_EDITOR
        if (imguiReady)
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
        }
#endif
        if (event.type == SDL_QUIT)
        {
            running = false;
        }
        else if (event.type == SDL_KEYDOWN)
        {
#ifdef AMBER_ENABLE_PLATFORMER2_EDITOR
            if (!event.key.repeat && event.key.keysym.sym == SDLK_F1)
            {
                ToggleEditorMode();
                continue;
            }
#endif
            if (IsFullscreenToggleKey(event.key))
            {
                ToggleFullscreen();
            }

            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                running = false;
            }
            else if (event.key.keysym.sym == SDLK_r)
            {
                ResetGame();
                pendingJumpPressed = false;
                pendingShootPressed = false;
            }
            else if (event.key.keysym.sym == SDLK_p)
            {
                paused = !paused;
            }
        }
    }

#ifdef AMBER_ENABLE_PLATFORMER2_EDITOR
    if (editorMode)
    {
        input = InputState{};
        jumpKeyWasDown = false;
        shootKeyWasDown = false;
        pendingJumpPressed = false;
        pendingShootPressed = false;
        return;
    }
#endif

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    const bool climbUpDown = keys[SDL_SCANCODE_W] != 0 || keys[SDL_SCANCODE_UP] != 0;
    const bool ladderOverlap = IsKindInRect(PlayerRect(), TileKind::Ladder);
    const bool jumpDown = keys[SDL_SCANCODE_SPACE] != 0 || (climbUpDown && !ladderOverlap);
    const bool shootDown = keys[SDL_SCANCODE_J] != 0 ||
        keys[SDL_SCANCODE_LCTRL] != 0 ||
        keys[SDL_SCANCODE_RCTRL] != 0;

    input.moveLeft = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
    input.moveRight = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
    input.climbUp = climbUpDown;
    input.climbDown = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];
    if (jumpDown && !jumpKeyWasDown)
    {
        pendingJumpPressed = true;
    }
    if (shootDown && !shootKeyWasDown)
    {
        pendingShootPressed = true;
    }

    input.jumpPressed = pendingJumpPressed;
    input.jumpHeld = jumpDown;
    input.shootPressed = pendingShootPressed;

    jumpKeyWasDown = jumpDown;
    shootKeyWasDown = shootDown;
}

void Platformer2App::Step(float dt, const InputState& input)
{
    worldTime += dt;
    shootCooldownTimer = std::max(0.0f, shootCooldownTimer - dt);

    if (player.won)
    {
        UpdateCamera();
        return;
    }

    UpdateLifts(dt);
    if (input.shootPressed)
    {
        TryShoot();
    }
    UpdatePlayer(dt, input);
    UpdateEnemies(dt);
    UpdateProjectiles(dt);
    UpdatePickupsAndHazards();
    UpdateCamera();
}

void Platformer2App::UpdateLifts(float dt)
{
    for (Lift& lift : lifts)
    {
        lift.previousPosition = lift.position;
        const float offset = std::sin(worldTime * lift.speed + lift.phase) * lift.amplitude;
        lift.position.x = lift.basePosition.x + lift.axis.x * offset;
        lift.position.y = lift.basePosition.y + lift.axis.y * offset;
    }
}

void Platformer2App::UpdatePlayer(float dt, const InputState& input)
{
    const Vec2 previousPosition = player.position;
    player.animationTime += dt;

    if (input.jumpPressed)
    {
        jumpBufferTimer = JumpBufferTime;
    }
    else
    {
        jumpBufferTimer = std::max(0.0f, jumpBufferTimer - dt);
    }

    const bool ladderOverlap = IsKindInRect(PlayerRect(), TileKind::Ladder);
    if (ladderOverlap && (input.climbUp || input.climbDown))
    {
        player.onLadder = true;
        player.velocity.y = 0.0f;
    }
    if (!ladderOverlap)
    {
        player.onLadder = false;
    }

    if (player.grounded)
    {
        coyoteTimer = CoyoteTime;
        player.airJumpsRemaining = MaxAirJumps;
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

    if (player.onLadder)
    {
        player.velocity.y = 0.0f;
        if (input.climbUp)
        {
            player.velocity.y = -LadderSpeed;
        }
        else if (input.climbDown)
        {
            player.velocity.y = LadderSpeed;
        }

        player.grounded = false;
        player.airJumpsRemaining = MaxAirJumps;
        if (jumpBufferTimer > 0.0f)
        {
            player.onLadder = false;
            player.velocity.y = JumpVelocity;
            jumpBufferTimer = 0.0f;
        }
    }
    else
    {
        if (jumpBufferTimer > 0.0f && coyoteTimer > 0.0f)
        {
            player.velocity.y = JumpVelocity;
            player.grounded = false;
            coyoteTimer = 0.0f;
            jumpBufferTimer = 0.0f;
        }
        else if (jumpBufferTimer > 0.0f && player.airJumpsRemaining > 0)
        {
            player.velocity.y = JumpVelocity * 0.90f;
            player.grounded = false;
            --player.airJumpsRemaining;
            jumpBufferTimer = 0.0f;
        }
        else if (!input.jumpHeld && !input.jumpPressed && player.velocity.y < JumpCutVelocity)
        {
            player.velocity.y = JumpCutVelocity;
        }

        player.velocity.y = ClampFloat(player.velocity.y + Gravity * dt, -1000.0f, MaxFallSpeed);
    }

    player.position.x += player.velocity.x * dt;
    ResolveTileCollisions(true);

    player.position.y += player.velocity.y * dt;
    player.grounded = false;
    ResolveTileCollisions(false);
    ResolveLiftCollisions(previousPosition);
    player.position.x = ClampFloat(player.position.x, 0.0f, LevelCols * static_cast<float>(WorldTileSize) - player.width);

    if (player.position.y > static_cast<float>(LevelRows * WorldTileSize + 120))
    {
        ResetPlayer();
    }
}

void Platformer2App::UpdateEnemies(float dt)
{
    for (Enemy& enemy : enemies)
    {
        if (!enemy.alive)
        {
            continue;
        }

        enemy.animationTime += dt;
        enemy.velocity.x = enemy.speed * static_cast<float>(enemy.facing);
        enemy.position.x += enemy.velocity.x * dt;
        if (enemy.position.x < enemy.leftBound)
        {
            enemy.position.x = enemy.leftBound;
            enemy.facing = 1;
        }
        else if (enemy.position.x + enemy.width > enemy.rightBound)
        {
            enemy.position.x = enemy.rightBound - enemy.width;
            enemy.facing = -1;
        }

        enemy.velocity.y = ClampFloat(enemy.velocity.y + Gravity * dt, -1000.0f, MaxFallSpeed);
        enemy.position.y += enemy.velocity.y * dt;
        ResolveEnemyVerticalCollision(enemy);
    }
}

void Platformer2App::UpdateProjectiles(float dt)
{
    for (Projectile& projectile : projectiles)
    {
        if (!projectile.active)
        {
            continue;
        }

        projectile.position.x += projectile.velocity.x * dt;
        projectile.position.y += projectile.velocity.y * dt;
        projectile.timeToLive -= dt;
        if (projectile.timeToLive <= 0.0f)
        {
            projectile.active = false;
            continue;
        }

        const RectF projectileBounds = ProjectileRect(projectile);
        const int tileX = static_cast<int>(std::floor((projectileBounds.x + projectileBounds.w * 0.5f) / WorldTileSize));
        const int tileY = static_cast<int>(std::floor((projectileBounds.y + projectileBounds.h * 0.5f) / WorldTileSize));
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

            --enemy.health;
            enemy.velocity.y = std::min(enemy.velocity.y, -120.0f);
            if (enemy.health <= 0)
            {
                enemy.alive = false;
            }
            projectile.active = false;
            break;
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

void Platformer2App::UpdatePickupsAndHazards()
{
    const RectF playerBounds = PlayerRect();
    const int left = ClampInt(static_cast<int>(std::floor(playerBounds.x / WorldTileSize)), 0, LevelCols - 1);
    const int right = ClampInt(static_cast<int>(std::floor((playerBounds.x + playerBounds.w) / WorldTileSize)), 0, LevelCols - 1);
    const int top = ClampInt(static_cast<int>(std::floor(playerBounds.y / WorldTileSize)), 0, LevelRows - 1);
    const int bottom = ClampInt(static_cast<int>(std::floor((playerBounds.y + playerBounds.h) / WorldTileSize)), 0, LevelRows - 1);

    for (int y = top; y <= bottom; ++y)
    {
        for (int x = left; x <= right; ++x)
        {
            TileCell& cell = level[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
            const RectF tileBounds{
                x * static_cast<float>(WorldTileSize),
                y * static_cast<float>(WorldTileSize),
                static_cast<float>(WorldTileSize),
                static_cast<float>(WorldTileSize)
            };

            if (!Intersects(playerBounds, tileBounds))
            {
                continue;
            }

            if (cell.kind == TileKind::Coin)
            {
                cell = TileCell{};
                ++player.coins;
            }
            else if (cell.kind == TileKind::Spike)
            {
                ResetPlayer();
                return;
            }
            else if (cell.kind == TileKind::Goal)
            {
                player.won = true;
            }
        }
    }

    for (const Enemy& enemy : enemies)
    {
        if (enemy.alive && Intersects(playerBounds, EnemyRect(enemy)))
        {
            ResetPlayer();
            return;
        }
    }
}

void Platformer2App::UpdateCamera()
{
    const float levelWidth = static_cast<float>(LevelCols * WorldTileSize);
    const float levelHeight = static_cast<float>(LevelRows * WorldTileSize);
    cameraX = ClampFloat(player.position.x + player.width * 0.5f - WindowWidth * 0.45f, 0.0f, std::max(0.0f, levelWidth - WindowWidth));
    cameraY = ClampFloat(player.position.y + player.height * 0.5f - WindowHeight * 0.58f, 0.0f, std::max(0.0f, levelHeight - WindowHeight));
}

void Platformer2App::TryShoot()
{
    if (shootCooldownTimer > 0.0f)
    {
        return;
    }

    const float direction = player.facing >= 0 ? 1.0f : -1.0f;
    Projectile projectile;
    projectile.position = Vec2{
        player.position.x + player.width * 0.5f + direction * 14.0f,
        player.position.y + player.height * 0.45f
    };
    projectile.velocity = Vec2{ProjectileSpeed * direction, 0.0f};
    projectiles.push_back(projectile);
    shootCooldownTimer = ProjectileCooldown;
}

void Platformer2App::ResolveTileCollisions(bool horizontal)
{
    RectF playerBounds = PlayerRect();
    const int left = ClampInt(static_cast<int>(std::floor(playerBounds.x / WorldTileSize)), 0, LevelCols - 1);
    const int right = ClampInt(static_cast<int>(std::floor((playerBounds.x + playerBounds.w) / WorldTileSize)), 0, LevelCols - 1);
    const int top = ClampInt(static_cast<int>(std::floor(playerBounds.y / WorldTileSize)), 0, LevelRows - 1);
    const int bottom = ClampInt(static_cast<int>(std::floor((playerBounds.y + playerBounds.h) / WorldTileSize)), 0, LevelRows - 1);

    for (int y = top; y <= bottom; ++y)
    {
        for (int x = left; x <= right; ++x)
        {
            if (!IsSolidTile(x, y))
            {
                continue;
            }

            const RectF tileBounds{
                x * static_cast<float>(WorldTileSize),
                y * static_cast<float>(WorldTileSize),
                static_cast<float>(WorldTileSize),
                static_cast<float>(WorldTileSize)
            };

            playerBounds = PlayerRect();
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
                    player.velocity.y = 0.0f;
                    player.grounded = true;
                    player.airJumpsRemaining = MaxAirJumps;
                }
                else if (player.velocity.y < 0.0f)
                {
                    player.position.y = tileBounds.y + tileBounds.h;
                    player.velocity.y = 0.0f;
                }
            }
        }
    }
}

void Platformer2App::ResolveLiftCollisions(const Vec2& previousPlayerPosition)
{
    RectF playerBounds = PlayerRect();
    const RectF previousPlayerBounds{previousPlayerPosition.x, previousPlayerPosition.y, player.width, player.height};

    for (const Lift& lift : lifts)
    {
        const RectF liftBounds = LiftRect(lift);
        const RectF previousLiftBounds{lift.previousPosition.x, lift.previousPosition.y, lift.width, lift.height};
        const bool landingCandidate = OverlapsHorizontally(playerBounds, liftBounds) &&
            previousPlayerBounds.y + previousPlayerBounds.h <= previousLiftBounds.y + 8.0f &&
            playerBounds.y + playerBounds.h >= liftBounds.y &&
            player.velocity.y >= 0.0f;

        if (landingCandidate)
        {
            player.position.y = liftBounds.y - player.height;
            player.position.x += lift.position.x - lift.previousPosition.x;
            player.velocity.y = 0.0f;
            player.grounded = true;
            player.airJumpsRemaining = MaxAirJumps;
            playerBounds = PlayerRect();
            continue;
        }

        if (!Intersects(playerBounds, liftBounds))
        {
            continue;
        }

        if (previousPlayerBounds.y + previousPlayerBounds.h <= previousLiftBounds.y + 8.0f)
        {
            player.position.y = liftBounds.y - player.height;
            player.velocity.y = 0.0f;
            player.grounded = true;
            player.airJumpsRemaining = MaxAirJumps;
        }
        else if (previousPlayerBounds.y >= previousLiftBounds.y + previousLiftBounds.h - 8.0f)
        {
            player.position.y = liftBounds.y + liftBounds.h;
            player.velocity.y = std::max(player.velocity.y, 0.0f);
        }
        else if (playerBounds.x + playerBounds.w * 0.5f < liftBounds.x + liftBounds.w * 0.5f)
        {
            player.position.x = liftBounds.x - player.width;
            player.velocity.x = std::min(player.velocity.x, 0.0f);
        }
        else
        {
            player.position.x = liftBounds.x + liftBounds.w;
            player.velocity.x = std::max(player.velocity.x, 0.0f);
        }

        playerBounds = PlayerRect();
    }
}

void Platformer2App::ResolveEnemyVerticalCollision(Enemy& enemy)
{
    RectF enemyBounds = EnemyRect(enemy);
    const int left = ClampInt(static_cast<int>(std::floor(enemyBounds.x / WorldTileSize)), 0, LevelCols - 1);
    const int right = ClampInt(static_cast<int>(std::floor((enemyBounds.x + enemyBounds.w) / WorldTileSize)), 0, LevelCols - 1);
    const int top = ClampInt(static_cast<int>(std::floor(enemyBounds.y / WorldTileSize)), 0, LevelRows - 1);
    const int bottom = ClampInt(static_cast<int>(std::floor((enemyBounds.y + enemyBounds.h) / WorldTileSize)), 0, LevelRows - 1);

    for (int y = top; y <= bottom; ++y)
    {
        for (int x = left; x <= right; ++x)
        {
            if (!IsSolidTile(x, y))
            {
                continue;
            }

            const RectF tileBounds{
                x * static_cast<float>(WorldTileSize),
                y * static_cast<float>(WorldTileSize),
                static_cast<float>(WorldTileSize),
                static_cast<float>(WorldTileSize)
            };
            enemyBounds = EnemyRect(enemy);
            if (Intersects(enemyBounds, tileBounds) && enemy.velocity.y >= 0.0f)
            {
                enemy.position.y = tileBounds.y - enemy.height;
                enemy.velocity.y = 0.0f;
            }
        }
    }
}

bool Platformer2App::IsSolidTile(int x, int y) const
{
    if (x < 0 || x >= LevelCols || y < 0 || y >= LevelRows)
    {
        return true;
    }

    return level[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)].kind == TileKind::Solid;
}

bool Platformer2App::IsKindInRect(const RectF& rect, TileKind kind) const
{
    const int left = ClampInt(static_cast<int>(std::floor(rect.x / WorldTileSize)), 0, LevelCols - 1);
    const int right = ClampInt(static_cast<int>(std::floor((rect.x + rect.w) / WorldTileSize)), 0, LevelCols - 1);
    const int top = ClampInt(static_cast<int>(std::floor(rect.y / WorldTileSize)), 0, LevelRows - 1);
    const int bottom = ClampInt(static_cast<int>(std::floor((rect.y + rect.h) / WorldTileSize)), 0, LevelRows - 1);

    for (int y = top; y <= bottom; ++y)
    {
        for (int x = left; x <= right; ++x)
        {
            if (level[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)].kind == kind)
            {
                return true;
            }
        }
    }

    return false;
}

Platformer2App::RectF Platformer2App::PlayerRect() const
{
    return RectF{player.position.x, player.position.y, player.width, player.height};
}

Platformer2App::RectF Platformer2App::EnemyRect(const Enemy& enemy) const
{
    return RectF{enemy.position.x, enemy.position.y, enemy.width, enemy.height};
}

Platformer2App::RectF Platformer2App::ProjectileRect(const Projectile& projectile) const
{
    return RectF{projectile.position.x, projectile.position.y, projectile.width, projectile.height};
}

Platformer2App::RectF Platformer2App::LiftRect(const Lift& lift) const
{
    return RectF{lift.position.x, lift.position.y, lift.width, lift.height};
}

bool Platformer2App::IsPlayerOnLift(const Lift& lift) const
{
    const RectF playerBounds = PlayerRect();
    const RectF liftBounds = LiftRect(lift);
    return OverlapsHorizontally(playerBounds, liftBounds) &&
        std::abs((playerBounds.y + playerBounds.h) - liftBounds.y) < 2.5f &&
        player.velocity.y >= 0.0f;
}

bool Platformer2App::Intersects(const RectF& first, const RectF& second)
{
    return first.x < second.x + second.w &&
        first.x + first.w > second.x &&
        first.y < second.y + second.h &&
        first.y + first.h > second.y;
}

bool Platformer2App::OverlapsHorizontally(const RectF& first, const RectF& second)
{
    return first.x < second.x + second.w && first.x + first.w > second.x;
}

int Platformer2App::ClampInt(int value, int minValue, int maxValue)
{
    return std::max(minValue, std::min(value, maxValue));
}

float Platformer2App::ClampFloat(float value, float minValue, float maxValue)
{
    return std::max(minValue, std::min(value, maxValue));
}

float Platformer2App::MoveTowardZero(float value, float amount)
{
    if (value > 0.0f)
    {
        return std::max(0.0f, value - amount);
    }
    if (value < 0.0f)
    {
        return std::min(0.0f, value + amount);
    }

    return 0.0f;
}

const char* Platformer2App::TileKindName(TileKind kind) const
{
    switch (kind)
    {
        case TileKind::Solid: return "Solid";
        case TileKind::Ladder: return "Ladder";
        case TileKind::Spike: return "Spike";
        case TileKind::Coin: return "Coin";
        case TileKind::Goal: return "Goal";
        case TileKind::Decor: return "Decor";
        default: return "Empty";
    }
}

void Platformer2App::Render()
{
    DrawBackground();
    DrawLevel();
    DrawLifts();
    DrawProjectiles();
    DrawEnemies();
    DrawPlayer();
    if (!editorMode)
    {
        DrawHud();
    }
    DrawEditorOverlay();
#ifdef AMBER_ENABLE_PLATFORMER2_EDITOR
    RenderEditor();
#endif
    SDL_RenderPresent(renderer);
}

void Platformer2App::DrawBackground() const
{
    DrawScreenRect(0, 0, WindowWidth, WindowHeight / 2, BackgroundTop);
    DrawScreenRect(0, WindowHeight / 2, WindowWidth, WindowHeight / 2, BackgroundBottom);

    for (int i = 0; i < 22; ++i)
    {
        const int x = static_cast<int>((i * 173 - static_cast<int>(cameraX * 0.18f)) % (WindowWidth + 90)) - 45;
        const int y = 34 + (i * 47) % 250;
        DrawScreenTile(14 + (i % 2), x, y, 16, 16);
    }

    for (int i = 0; i < 9; ++i)
    {
        const int x = static_cast<int>((i * 410 - static_cast<int>(cameraX * 0.35f)) % (WindowWidth + 220)) - 120;
        DrawScreenRect(x, 420 + (i % 3) * 28, 180, 70, SDL_Color{10, 11, 12, 170});
    }
}

void Platformer2App::DrawLevel() const
{
    const int firstCol = ClampInt(static_cast<int>(std::floor(cameraX / WorldTileSize)) - 1, 0, LevelCols - 1);
    const int lastCol = ClampInt(static_cast<int>(std::ceil((cameraX + WindowWidth) / WorldTileSize)) + 1, 0, LevelCols - 1);
    const int firstRow = ClampInt(static_cast<int>(std::floor(cameraY / WorldTileSize)) - 1, 0, LevelRows - 1);
    const int lastRow = ClampInt(static_cast<int>(std::ceil((cameraY + WindowHeight) / WorldTileSize)) + 1, 0, LevelRows - 1);

    for (int y = firstRow; y <= lastRow; ++y)
    {
        for (int x = firstCol; x <= lastCol; ++x)
        {
            const TileCell& cell = level[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
            if (cell.visual < 0)
            {
                continue;
            }

            int tile = cell.visual;
            if (cell.kind == TileKind::Coin)
            {
                tile = TileCoin + (static_cast<int>(worldTime * 8.0f) % 3);
            }
            else if (cell.kind == TileKind::Spike)
            {
                tile = TileSpike + (static_cast<int>(worldTime * 5.0f) % 2);
            }

            DrawTile(tile, x * static_cast<float>(WorldTileSize), y * static_cast<float>(WorldTileSize));
        }
    }
}

void Platformer2App::DrawLifts() const
{
    for (const Lift& lift : lifts)
    {
        DrawSegmentedLift(lift);
    }
}

void Platformer2App::DrawEnemies() const
{
    for (const Enemy& enemy : enemies)
    {
        if (!enemy.alive)
        {
            continue;
        }

        const int frame = TileEnemyWalkStart + (static_cast<int>(enemy.animationTime * 8.0f) % 4);
        DrawTile(frame, enemy.position.x - 4.0f, enemy.position.y - 2.0f, WorldTileSize, WorldTileSize, enemy.facing > 0);
    }
}

void Platformer2App::DrawProjectiles() const
{
    for (const Projectile& projectile : projectiles)
    {
        if (!projectile.active)
        {
            continue;
        }

        DrawWorldRect(ProjectileRect(projectile), ProjectileColor);
        DrawTile(TileProjectile, projectile.position.x - 5.0f, projectile.position.y - 5.0f, 16, 16);
    }
}

void Platformer2App::DrawPlayer() const
{
    int frame = TilePlayerIdle;
    if (player.onLadder)
    {
        frame = TilePlayerClimbStart + (static_cast<int>(player.animationTime * 7.0f) % 4);
    }
    else if (!player.grounded)
    {
        frame = TilePlayerJump;
    }
    else if (std::abs(player.velocity.x) > 20.0f)
    {
        frame = TilePlayerWalkStart + (static_cast<int>(player.animationTime * 10.0f) % 6);
    }

    DrawTile(frame, player.position.x - 5.0f, player.position.y - 2.0f, WorldTileSize, WorldTileSize, player.facing < 0);
}

void Platformer2App::DrawHud() const
{
    DrawScreenRect(12, 12, 260, 42, HudBack);
    DrawScreenTile(TileHeart, 22, 21, 22, 22);
    DrawScreenTile(TileCoin + (static_cast<int>(worldTime * 8.0f) % 3), 62, 19, 26, 26);

    for (int i = 0; i < std::min(player.coins, 12); ++i)
    {
        DrawScreenRect(96 + i * 12, 29, 7, 7, SDL_Color{255, 255, 255, 235});
    }

    if (player.won)
    {
        DrawScreenRect(WindowWidth / 2 - 104, 24, 208, 28, SDL_Color{255, 255, 255, 220});
        for (int i = 0; i < 6; ++i)
        {
            DrawScreenTile(TileCoin + (i % 3), WindowWidth / 2 - 72 + i * 28, 22, 26, 26);
        }
    }
}

void Platformer2App::DrawEditorOverlay() const
{
    if (!editorMode)
    {
        return;
    }

    if (editorShowGrid)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 42);
        const int firstCol = ClampInt(static_cast<int>(std::floor(cameraX / WorldTileSize)), 0, LevelCols - 1);
        const int lastCol = ClampInt(static_cast<int>(std::ceil((cameraX + WindowWidth) / WorldTileSize)) + 1, 0, LevelCols);
        const int firstRow = ClampInt(static_cast<int>(std::floor(cameraY / WorldTileSize)), 0, LevelRows - 1);
        const int lastRow = ClampInt(static_cast<int>(std::ceil((cameraY + WindowHeight) / WorldTileSize)) + 1, 0, LevelRows);

        for (int x = firstCol; x <= lastCol; ++x)
        {
            const int screenX = RoundToInt(x * static_cast<float>(WorldTileSize) - cameraX);
            SDL_RenderDrawLine(renderer, screenX, 0, screenX, WindowHeight);
        }
        for (int y = firstRow; y <= lastRow; ++y)
        {
            const int screenY = RoundToInt(y * static_cast<float>(WorldTileSize) - cameraY);
            SDL_RenderDrawLine(renderer, 0, screenY, WindowWidth, screenY);
        }
    }

    if (editorSelection.type == EditorSelectionType::Tile)
    {
        DrawWorldRect(
            RectF{
                editorSelection.x * static_cast<float>(WorldTileSize),
                editorSelection.y * static_cast<float>(WorldTileSize),
                static_cast<float>(WorldTileSize),
                static_cast<float>(WorldTileSize)
            },
            SDL_Color{255, 255, 255, 72});
    }
    else if (editorSelection.type == EditorSelectionType::Enemy && editorSelection.index < enemies.size())
    {
        DrawWorldRect(EnemyRect(enemies[editorSelection.index]), SDL_Color{255, 220, 80, 88});
    }
    else if (editorSelection.type == EditorSelectionType::Lift && editorSelection.index < lifts.size())
    {
        DrawWorldRect(LiftRect(lifts[editorSelection.index]), SDL_Color{80, 180, 255, 88});
    }
    else if (editorSelection.type == EditorSelectionType::PlayerSpawn)
    {
        DrawWorldRect(RectF{playerSpawn.x, playerSpawn.y, player.width, player.height}, SDL_Color{80, 255, 150, 88});
    }
    else if (editorSelection.type == EditorSelectionType::Goal)
    {
        DrawWorldRect(finish, SDL_Color{255, 255, 80, 88});
    }

    DrawEditorTilePalette();
}

void Platformer2App::DrawEditorTilePalette() const
{
    if (!editorMode || !editorShowPalette || !tilemapTexture)
    {
        return;
    }

    const int tileDisplaySize = SourceTileSize;
    const int paletteColumns = SheetColumns;
    const int paletteRows = 20;
    const int paletteWidth = paletteColumns * tileDisplaySize;
    const int paletteHeight = paletteRows * tileDisplaySize;
    const int paletteX = (WindowWidth - paletteWidth) / 2;
    const int paletteY = 14;

    DrawScreenRect(
        paletteX - 8,
        paletteY - 8,
        paletteWidth + 16,
        paletteHeight + 16,
        SDL_Color{0, 0, 0, 190});

    for (int tileId = 0; tileId < paletteColumns * paletteRows; ++tileId)
    {
        const SDL_Rect source{
            (tileId % SheetColumns) * SourceTileSize,
            (tileId / SheetColumns) * SourceTileSize,
            SourceTileSize,
            SourceTileSize
        };
        const SDL_Rect destination{
            paletteX + (tileId % paletteColumns) * tileDisplaySize,
            paletteY + (tileId / paletteColumns) * tileDisplaySize,
            tileDisplaySize,
            tileDisplaySize
        };
        SDL_RenderCopy(renderer, tilemapTexture, &source, &destination);
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 70);
    const SDL_Rect paletteBorder{paletteX - 1, paletteY - 1, paletteWidth + 2, paletteHeight + 2};
    SDL_RenderDrawRect(renderer, &paletteBorder);

    if (editorTileVisual >= 0 && editorTileVisual < paletteColumns * paletteRows)
    {
        const SDL_Rect selected{
            paletteX + (editorTileVisual % paletteColumns) * tileDisplaySize - 2,
            paletteY + (editorTileVisual / paletteColumns) * tileDisplaySize - 2,
            tileDisplaySize + 4,
            tileDisplaySize + 4
        };
        SDL_SetRenderDrawColor(renderer, 255, 216, 80, 255);
        SDL_RenderDrawRect(renderer, &selected);

        const int previewX = paletteX;
        const int previewY = paletteY + paletteHeight + 12;
        DrawScreenRect(previewX - 4, previewY - 4, 72, 72, SDL_Color{0, 0, 0, 190});
        DrawScreenTile(editorTileVisual, previewX, previewY, 64, 64);
        SDL_SetRenderDrawColor(renderer, 255, 216, 80, 180);
        const SDL_Rect previewBorder{previewX - 4, previewY - 4, 72, 72};
        SDL_RenderDrawRect(renderer, &previewBorder);
    }
}

bool Platformer2App::PickEditorPaletteTile(float logicalX, float logicalY, int& tileId) const
{
    if (!editorMode || !editorShowPalette)
    {
        return false;
    }

    const int tileDisplaySize = SourceTileSize;
    const int paletteColumns = SheetColumns;
    const int paletteRows = 20;
    const int paletteWidth = paletteColumns * tileDisplaySize;
    const int paletteHeight = paletteRows * tileDisplaySize;
    const int paletteX = (WindowWidth - paletteWidth) / 2;
    const int paletteY = 14;

    if (logicalX < paletteX || logicalX >= paletteX + paletteWidth ||
        logicalY < paletteY || logicalY >= paletteY + paletteHeight)
    {
        return false;
    }

    const int col = ClampInt(static_cast<int>((logicalX - paletteX) / tileDisplaySize), 0, paletteColumns - 1);
    const int row = ClampInt(static_cast<int>((logicalY - paletteY) / tileDisplaySize), 0, paletteRows - 1);
    tileId = row * paletteColumns + col;
    return true;
}

Platformer2App::TileKind Platformer2App::GuessTileKindForVisual(int tileId) const
{
    if (tileId == TileLadder || tileId == 81 || tileId == 100 || tileId == 101 || tileId == 120 || tileId == 121)
    {
        return TileKind::Ladder;
    }
    if (tileId == TileSpike || tileId == 184)
    {
        return TileKind::Spike;
    }
    if (tileId >= TileCoin && tileId <= TileCoin + 2)
    {
        return TileKind::Coin;
    }
    if (tileId == TileDoorTopLeft || tileId == TileDoorTopRight ||
        tileId == TileDoorBottomLeft || tileId == TileDoorBottomRight)
    {
        return TileKind::Goal;
    }
    if (tileId >= 14 && tileId <= 19)
    {
        return TileKind::Decor;
    }

    return TileKind::Solid;
}

#ifdef AMBER_ENABLE_PLATFORMER2_EDITOR
void Platformer2App::BeginEditorFrame()
{
    if (!imguiReady)
    {
        return;
    }

    ImGui_ImplSDL2_NewFrame(window);
    ImGuiSDL::ApplyLogicalDisplaySize(window, renderer, WindowWidth, WindowHeight);
    ImGui::NewFrame();
}

void Platformer2App::RenderEditor()
{
    if (!imguiReady)
    {
        return;
    }

    BeginEditorFrame();
    if (editorMode)
    {
        DrawEditorWindows();
        ApplyEditorMouse();
    }
    else
    {
        ImGui::SetNextWindowPos(ImVec2(16.0f, 64.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Platformer2", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::Text("F1: Map Editor");
        ImGui::End();
    }

    ImGui::Render();
    ImGuiSDL::Render(ImGui::GetDrawData());
}

void Platformer2App::DrawEditorWindows()
{
    ImGui::SetNextWindowPos(ImVec2(12.0f, 72.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280.0f, 430.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Map Editor");

    if (ImGui::Button("Save"))
    {
        editorStatus = SaveMap() ? "Saved map" : "Save failed";
    }
    ImGui::SameLine();
    if (ImGui::Button("Load"))
    {
        if (LoadMap())
        {
            ResetPlayer();
            editorStatus = "Loaded map";
        }
        else
        {
            editorStatus = "Load failed";
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Default"))
    {
        BuildDefaultLevel();
        ResetPlayer();
        editorStatus = "Default level restored";
    }

    if (ImGui::Button(editorWasPausedBeforeOpen ? "Resume On Close" : "Stay Paused On Close"))
    {
        editorWasPausedBeforeOpen = !editorWasPausedBeforeOpen;
    }
    ImGui::SameLine();
    if (ImGui::Button("Play From Here"))
    {
        editorMode = false;
        paused = false;
        editorSelection = EditorSelection{};
    }

    ImGui::Checkbox("Grid", &editorShowGrid);
    ImGui::Checkbox("Tileset Palette", &editorShowPalette);
    ImGui::SliderFloat("Camera X", &cameraX, 0.0f, std::max(0.0f, LevelCols * static_cast<float>(WorldTileSize) - WindowWidth));
    ImGui::SliderFloat("Camera Y", &cameraY, 0.0f, std::max(0.0f, LevelRows * static_cast<float>(WorldTileSize) - WindowHeight));

    const char* tools[] = {"Select", "Tile", "Erase", "Enemy", "Lift", "Player Spawn", "Goal"};
    ImGui::Combo("Tool", &editorTool, tools, IM_ARRAYSIZE(tools));

    const char* tileKinds[] = {"Empty", "Solid", "Ladder", "Spike", "Coin", "Goal", "Decor"};
    ImGui::Combo("Tile Kind", &editorTileKind, tileKinds, IM_ARRAYSIZE(tileKinds));
    ImGui::SliderInt("Tile ID", &editorTileVisual, 0, 399);
    ImGui::Text("Selected: %s / tile_%04d", TileKindName(static_cast<TileKind>(editorTileKind)), editorTileVisual);
    if (ImGui::Button("Use solid"))
    {
        editorTileKind = static_cast<int>(TileKind::Solid);
        editorTileVisual = TileGroundMiddle;
    }
    ImGui::SameLine();
    if (ImGui::Button("Use ladder"))
    {
        editorTileKind = static_cast<int>(TileKind::Ladder);
        editorTileVisual = TileLadder;
    }
    if (ImGui::Button("Use spike"))
    {
        editorTileKind = static_cast<int>(TileKind::Spike);
        editorTileVisual = TileSpike;
    }
    ImGui::SameLine();
    if (ImGui::Button("Use coin"))
    {
        editorTileKind = static_cast<int>(TileKind::Coin);
        editorTileVisual = TileCoin;
    }

    if (!editorStatus.empty())
    {
        ImGui::Separator();
        ImGui::TextWrapped("%s", editorStatus.c_str());
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(WindowWidth - 310.0f, 72.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(292.0f, 430.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene Outliner");

    if (ImGui::Selectable("Player Spawn", editorSelection.type == EditorSelectionType::PlayerSpawn))
    {
        editorSelection = EditorSelection{EditorSelectionType::PlayerSpawn};
    }
    if (ImGui::Selectable("Goal", editorSelection.type == EditorSelectionType::Goal))
    {
        editorSelection = EditorSelection{EditorSelectionType::Goal};
    }

    if (ImGui::TreeNode("Enemies"))
    {
        for (std::size_t i = 0; i < enemies.size(); ++i)
        {
            std::string label = "Enemy " + std::to_string(i);
            if (ImGui::Selectable(label.c_str(), editorSelection.type == EditorSelectionType::Enemy && editorSelection.index == i))
            {
                editorSelection = EditorSelection{EditorSelectionType::Enemy, 0, 0, i};
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Lifts"))
    {
        for (std::size_t i = 0; i < lifts.size(); ++i)
        {
            std::string label = "Lift " + std::to_string(i);
            if (ImGui::Selectable(label.c_str(), editorSelection.type == EditorSelectionType::Lift && editorSelection.index == i))
            {
                editorSelection = EditorSelection{EditorSelectionType::Lift, 0, 0, i};
            }
        }
        ImGui::TreePop();
    }

    if (editorSelection.type != EditorSelectionType::None)
    {
        ImGui::Separator();
        ImGui::Text("Selection: %s", SelectionName());
        if (ImGui::Button("Delete"))
        {
            DeleteEditorSelection();
        }
    }

    ImGui::Separator();
    if (editorSelection.type == EditorSelectionType::Tile &&
        editorSelection.x >= 0 && editorSelection.x < LevelCols &&
        editorSelection.y >= 0 && editorSelection.y < LevelRows)
    {
        TileCell& cell = level[static_cast<std::size_t>(editorSelection.y)][static_cast<std::size_t>(editorSelection.x)];
        int kind = static_cast<int>(cell.kind);
        int visual = cell.visual;
        ImGui::Text("Tile: %d, %d", editorSelection.x, editorSelection.y);
        if (ImGui::Combo("Kind", &kind, tileKinds, IM_ARRAYSIZE(tileKinds)))
        {
            cell.kind = static_cast<TileKind>(kind);
        }
        if (ImGui::InputInt("Visual", &visual))
        {
            cell.visual = ClampInt(visual, -1, 399);
        }
    }
    else if (editorSelection.type == EditorSelectionType::Enemy && editorSelection.index < enemies.size())
    {
        Enemy& enemy = enemies[editorSelection.index];
        ImGui::InputFloat("X", &enemy.spawnPosition.x);
        ImGui::InputFloat("Y", &enemy.spawnPosition.y);
        ImGui::InputFloat("Left", &enemy.leftBound);
        ImGui::InputFloat("Right", &enemy.rightBound);
        ImGui::InputFloat("Speed", &enemy.speed);
        ImGui::InputInt("Health", &enemy.health);
        if (ImGui::Button("Move live to spawn"))
        {
            enemy.position = enemy.spawnPosition;
            enemy.velocity = Vec2{};
            enemy.alive = true;
        }
    }
    else if (editorSelection.type == EditorSelectionType::Lift && editorSelection.index < lifts.size())
    {
        Lift& lift = lifts[editorSelection.index];
        ImGui::InputFloat("X", &lift.basePosition.x);
        ImGui::InputFloat("Y", &lift.basePosition.y);
        ImGui::InputFloat("Axis X", &lift.axis.x);
        ImGui::InputFloat("Axis Y", &lift.axis.y);
        ImGui::InputFloat("Width", &lift.width);
        ImGui::InputFloat("Height", &lift.height);
        ImGui::InputFloat("Amplitude", &lift.amplitude);
        ImGui::InputFloat("Speed", &lift.speed);
        ImGui::InputFloat("Phase", &lift.phase);
        lift.position = lift.basePosition;
        lift.previousPosition = lift.position;
    }
    else if (editorSelection.type == EditorSelectionType::PlayerSpawn)
    {
        ImGui::InputFloat("Spawn X", &playerSpawn.x);
        ImGui::InputFloat("Spawn Y", &playerSpawn.y);
        if (ImGui::Button("Move player here"))
        {
            ResetPlayer();
        }
    }
    else if (editorSelection.type == EditorSelectionType::Goal)
    {
        ImGui::InputFloat("Goal X", &finish.x);
        ImGui::InputFloat("Goal Y", &finish.y);
        if (ImGui::Button("Snap goal tiles"))
        {
            SetGoalTilesFromFinish();
        }
    }

    ImGui::End();
}

void Platformer2App::ApplyEditorMouse()
{
    const ImGuiIO& io = ImGui::GetIO();
    int mouseX = 0;
    int mouseY = 0;
    const Uint32 mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
    float logicalX = static_cast<float>(mouseX);
    float logicalY = static_cast<float>(mouseY);
    SDL_RenderWindowToLogical(renderer, mouseX, mouseY, &logicalX, &logicalY);

    const bool mouseDown = (mouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    const bool rightMouseDown = (mouseButtons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    const bool mousePressed = mouseDown && !editorMouseWasDown;
    const bool rightMousePressed = rightMouseDown && !editorRightMouseWasDown;
    bool consumedByPalette = false;

    if (!io.WantCaptureMouse && mousePressed)
    {
        int pickedTile = -1;
        if (PickEditorPaletteTile(logicalX, logicalY, pickedTile))
        {
            editorTileVisual = pickedTile;
            editorTileKind = static_cast<int>(GuessTileKindForVisual(pickedTile));
            editorTool = static_cast<int>(EditorTool::Tile);
            editorStatus = "Selected tile_" + std::to_string(pickedTile);
            consumedByPalette = true;
        }
    }

    if (!consumedByPalette && !io.WantCaptureMouse &&
        logicalX >= 0.0f && logicalX < WindowWidth && logicalY >= 0.0f && logicalY < WindowHeight)
    {
        const float worldX = logicalX + cameraX;
        const float worldY = logicalY + cameraY;
        const int tileX = ClampInt(static_cast<int>(std::floor(worldX / WorldTileSize)), 0, LevelCols - 1);
        const int tileY = ClampInt(static_cast<int>(std::floor(worldY / WorldTileSize)), 0, LevelRows - 1);
        const EditorTool tool = static_cast<EditorTool>(editorTool);

        if (rightMouseDown)
        {
            SetTile(tileX, tileY, TileKind::Empty, -1);
            if (rightMousePressed)
            {
                editorSelection = EditorSelection{EditorSelectionType::Tile, tileX, tileY};
            }
        }
        else if (mouseDown && tool == EditorTool::Tile)
        {
            SetTile(tileX, tileY, static_cast<TileKind>(editorTileKind), editorTileVisual);
            editorSelection = EditorSelection{EditorSelectionType::Tile, tileX, tileY};
        }
        else if (mouseDown && tool == EditorTool::Erase)
        {
            SetTile(tileX, tileY, TileKind::Empty, -1);
            editorSelection = EditorSelection{EditorSelectionType::Tile, tileX, tileY};
        }
        else if (mousePressed && tool == EditorTool::Enemy)
        {
            const float x = tileX * static_cast<float>(WorldTileSize) + 4.0f;
            const float y = (tileY + 1) * static_cast<float>(WorldTileSize) - 28.0f;
            AddEnemyAtWorld(x, y, std::max(0.0f, x - 128.0f), std::min(LevelCols * static_cast<float>(WorldTileSize), x + 192.0f), 78.0f);
            editorSelection = EditorSelection{EditorSelectionType::Enemy, 0, 0, enemies.size() - 1};
        }
        else if (mousePressed && tool == EditorTool::Lift)
        {
            AddLiftAtWorld(
                tileX * static_cast<float>(WorldTileSize),
                tileY * static_cast<float>(WorldTileSize),
                Vec2{0.0f, -1.0f},
                96.0f,
                16.0f,
                120.0f,
                1.0f,
                0.0f);
            editorSelection = EditorSelection{EditorSelectionType::Lift, 0, 0, lifts.size() - 1};
        }
        else if (mousePressed && tool == EditorTool::PlayerSpawn)
        {
            playerSpawn = Vec2{tileX * static_cast<float>(WorldTileSize), (tileY + 1) * static_cast<float>(WorldTileSize) - player.height};
            player.position = playerSpawn;
            editorSelection = EditorSelection{EditorSelectionType::PlayerSpawn};
        }
        else if (mousePressed && tool == EditorTool::Goal)
        {
            finish = RectF{tileX * static_cast<float>(WorldTileSize), tileY * static_cast<float>(WorldTileSize), 2.0f * WorldTileSize, 2.0f * WorldTileSize};
            SetGoalTilesFromFinish();
            editorSelection = EditorSelection{EditorSelectionType::Goal};
        }
        else if (mousePressed && tool == EditorTool::Select)
        {
            SelectAtWorld(worldX, worldY);
        }
    }

    editorMouseWasDown = mouseDown;
    editorRightMouseWasDown = rightMouseDown;
}

void Platformer2App::SelectAtWorld(float worldX, float worldY)
{
    const RectF point{worldX, worldY, 1.0f, 1.0f};
    for (std::size_t i = 0; i < enemies.size(); ++i)
    {
        if (Intersects(point, EnemyRect(enemies[i])))
        {
            editorSelection = EditorSelection{EditorSelectionType::Enemy, 0, 0, i};
            return;
        }
    }
    for (std::size_t i = 0; i < lifts.size(); ++i)
    {
        if (Intersects(point, LiftRect(lifts[i])))
        {
            editorSelection = EditorSelection{EditorSelectionType::Lift, 0, 0, i};
            return;
        }
    }
    if (Intersects(point, RectF{playerSpawn.x, playerSpawn.y, player.width, player.height}))
    {
        editorSelection = EditorSelection{EditorSelectionType::PlayerSpawn};
        return;
    }
    if (Intersects(point, finish))
    {
        editorSelection = EditorSelection{EditorSelectionType::Goal};
        return;
    }

    const int tileX = ClampInt(static_cast<int>(std::floor(worldX / WorldTileSize)), 0, LevelCols - 1);
    const int tileY = ClampInt(static_cast<int>(std::floor(worldY / WorldTileSize)), 0, LevelRows - 1);
    editorSelection = EditorSelection{EditorSelectionType::Tile, tileX, tileY};
}

void Platformer2App::DeleteEditorSelection()
{
    if (editorSelection.type == EditorSelectionType::Enemy && editorSelection.index < enemies.size())
    {
        enemies.erase(enemies.begin() + static_cast<std::ptrdiff_t>(editorSelection.index));
    }
    else if (editorSelection.type == EditorSelectionType::Lift && editorSelection.index < lifts.size())
    {
        lifts.erase(lifts.begin() + static_cast<std::ptrdiff_t>(editorSelection.index));
    }
    else if (editorSelection.type == EditorSelectionType::Tile)
    {
        SetTile(editorSelection.x, editorSelection.y, TileKind::Empty, -1);
    }
    else if (editorSelection.type == EditorSelectionType::Goal)
    {
        ClearGoalTiles();
    }

    editorSelection = EditorSelection{};
}

const char* Platformer2App::EditorToolName(EditorTool tool) const
{
    switch (tool)
    {
        case EditorTool::Tile: return "Tile";
        case EditorTool::Erase: return "Erase";
        case EditorTool::Enemy: return "Enemy";
        case EditorTool::Lift: return "Lift";
        case EditorTool::PlayerSpawn: return "Player Spawn";
        case EditorTool::Goal: return "Goal";
        default: return "Select";
    }
}

const char* Platformer2App::SelectionName() const
{
    switch (editorSelection.type)
    {
        case EditorSelectionType::Tile: return "Tile";
        case EditorSelectionType::Enemy: return "Enemy";
        case EditorSelectionType::Lift: return "Lift";
        case EditorSelectionType::PlayerSpawn: return "Player Spawn";
        case EditorSelectionType::Goal: return "Goal";
        default: return "None";
    }
}
#endif

void Platformer2App::DrawTile(int tileId, float worldX, float worldY, int width, int height, bool flip) const
{
    if (!tilemapTexture || tileId < 0)
    {
        return;
    }

    const SDL_Rect source{
        (tileId % SheetColumns) * SourceTileSize,
        (tileId / SheetColumns) * SourceTileSize,
        SourceTileSize,
        SourceTileSize
    };
    const SDL_Rect destination{
        RoundToInt(worldX - cameraX),
        RoundToInt(worldY - cameraY),
        width,
        height
    };

    SDL_RenderCopyEx(
        renderer,
        tilemapTexture,
        &source,
        &destination,
        0.0,
        nullptr,
        flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}

void Platformer2App::DrawScreenTile(int tileId, int x, int y, int width, int height) const
{
    if (!tilemapTexture || tileId < 0)
    {
        return;
    }

    const SDL_Rect source{
        (tileId % SheetColumns) * SourceTileSize,
        (tileId / SheetColumns) * SourceTileSize,
        SourceTileSize,
        SourceTileSize
    };
    const SDL_Rect destination{x, y, width, height};
    SDL_RenderCopy(renderer, tilemapTexture, &source, &destination);
}

void Platformer2App::DrawWorldRect(const RectF& rect, SDL_Color color) const
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    const SDL_Rect destination{
        RoundToInt(rect.x - cameraX),
        RoundToInt(rect.y - cameraY),
        RoundToInt(rect.w),
        RoundToInt(rect.h)
    };
    SDL_RenderFillRect(renderer, &destination);
}

void Platformer2App::DrawScreenRect(int x, int y, int w, int h, SDL_Color color) const
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    const SDL_Rect rect{x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
}

void Platformer2App::DrawSegmentedLift(const Lift& lift) const
{
    const int segmentCount = std::max(1, static_cast<int>(std::ceil(lift.width / WorldTileSize)));
    for (int segment = 0; segment < segmentCount; ++segment)
    {
        int tile = TileLiftMiddle;
        if (segment == 0)
        {
            tile = TileLiftLeft;
        }
        else if (segment == segmentCount - 1)
        {
            tile = TileLiftRight;
        }

        DrawTile(tile, lift.position.x + segment * WorldTileSize, lift.position.y - 8.0f);
    }
}
