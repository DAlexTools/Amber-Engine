#include "PlatformerApp.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
#include "imgui.h"
#endif

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

    SDL_Color SkyTop{100, 168, 235, 255};
    SDL_Color SkyBottom{170, 215, 245, 255};
    SDL_Color Ground{72, 124, 55, 255};
    SDL_Color Dirt{126, 86, 51, 255};
    SDL_Color Brick{158, 92, 72, 255};
    SDL_Color PlayerBody{35, 123, 172, 255};
    SDL_Color PlayerAccent{242, 225, 179, 255};
    SDL_Color CoinColor{245, 196, 48, 255};
    SDL_Color EnemyColor{174, 54, 62, 255};

    bool IsFullscreenToggleKey(const SDL_KeyboardEvent& keyEvent)
    {
        const SDL_Keycode key = keyEvent.keysym.sym;
        const bool altPressed = (keyEvent.keysym.mod & KMOD_ALT) != 0;
        return key == SDLK_F11 || (altPressed && (key == SDLK_RETURN || key == SDLK_KP_ENTER));
    }
}

PlatformerApp::PlatformerApp()
{
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
            accumulator -= FixedTimeStep;
            stepped = true;
            ++fixedStepsThisFrame;
        }
        if (stepped)
        {
            input.jumpPressed = false;
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

    return movedRight && stayedInWorld && hasGroundState && coinCollected && hazardResetsPlayer && finishWorks &&
        bufferedJumpWorks && coyoteJumpWorks;
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

    auto fillTiles = [this](int tileX, int tileY, int width, int height, char value)
    {
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

    enemies = {
        Enemy{AE::Physics::Vector2D(19.0f * TileSize, 15.0f * TileSize - 26.0f), {}, 30.0f, 26.0f, 70.0f, 18.0f * TileSize, 24.0f * TileSize},
        Enemy{AE::Physics::Vector2D(45.0f * TileSize, 15.0f * TileSize - 26.0f), {}, 30.0f, 26.0f, -60.0f, 43.0f * TileSize, 49.0f * TileSize},
        Enemy{AE::Physics::Vector2D(66.0f * TileSize, 13.0f * TileSize - 26.0f), {}, 30.0f, 26.0f, 55.0f, 63.0f * TileSize, 71.0f * TileSize}
    };
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
    }
    ResetPlayer();
}

void PlatformerApp::ResetPlayer()
{
    player.position = playerSpawn;
    player.velocity = AE::Physics::Vector2D::Zero;
    player.grounded = false;
    player.won = false;
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

    UpdateEnemies(dt);
    UpdatePlayer(dt, input);
    UpdateCoins();
    UpdateHazards();
    UpdateGoal();
    UpdateCamera();
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
        player.velocity.x -= MoveAcceleration * dt;
    }
    else if (input.moveRight)
    {
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
        enemy.position.x += enemy.velocityX * dt;
        if (enemy.position.x < enemy.leftBound)
        {
            enemy.position.x = enemy.leftBound;
            enemy.velocityX = std::abs(enemy.velocityX);
        }
        else if (enemy.position.x + enemy.width > enemy.rightBound)
        {
            enemy.position.x = enemy.rightBound - enemy.width;
            enemy.velocityX = -std::abs(enemy.velocityX);
        }
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
    DrawCoins();
    DrawFinish();
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
        " | coins " + std::to_string(collectedCoins) + "/" + std::to_string(coins.size());

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
        const RectF bounds = EnemyRect(enemy);
        DrawRect(bounds, EnemyColor);
        DrawRect(RectF{bounds.x + 6.0f, bounds.y + 6.0f, 5.0f, 5.0f}, SDL_Color{255, 245, 220, 255});
        DrawRect(RectF{bounds.x + bounds.w - 11.0f, bounds.y + 6.0f, 5.0f, 5.0f}, SDL_Color{255, 245, 220, 255});
        DrawRect(RectF{bounds.x, bounds.y + bounds.h - 5.0f, bounds.w, 5.0f}, SDL_Color{110, 36, 50, 255});
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
    DrawScreenRect(18, 18, 180, 42, SDL_Color{24, 40, 56, 220});

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

void PlatformerApp::DrawScreenRect(int x, int y, int w, int h, SDL_Color color) const
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect{x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
}
