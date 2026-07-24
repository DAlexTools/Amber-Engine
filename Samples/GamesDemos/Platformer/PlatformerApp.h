#ifndef PLATFORMER_APP_H
#define PLATFORMER_APP_H

#include <SDL2/SDL.h>

#include <string>
#include <vector>

#include "Core/BuildConfig.h"
#include "Core/Math/Vector2D.h"

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
#include "Editor/Diagnostics/SampleDiagnosticsOverlay.h"
#endif

class PlatformerApp
{
public:
    PlatformerApp();

    int Run();
#if SMOKE_TEST
    bool RunSmokeTest();
#endif

private:
    struct RectF
    {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
    };

    struct Player
    {
        AE::Physics::Vector2D position;
        AE::Physics::Vector2D velocity;
        float width = 28.0f;
        float height = 42.0f;
        bool grounded = false;
        bool won = false;
    };

    struct Coin
    {
        RectF bounds;
        bool collected = false;
    };

    struct Enemy
    {
        AE::Physics::Vector2D spawnPosition;
        AE::Physics::Vector2D position;
        float width = 30.0f;
        float height = 26.0f;
        float velocityX = 70.0f;
        float leftBound = 0.0f;
        float rightBound = 0.0f;
    };

    struct InputState
    {
        bool moveLeft = false;
        bool moveRight = false;
        bool jumpPressed = false;
        bool jumpHeld = false;
    };

    static constexpr int WindowWidth = 960;
    static constexpr int WindowHeight = 540;
    static constexpr int TileSize = 32;
    static constexpr int LevelCols = 96;
    static constexpr int LevelRows = 17;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* frameTexture = nullptr;
    bool running = false;
#if SMOKE_TEST
    bool smokeMode = false;
#endif
    bool fullscreen = false;
    bool paused = false;
    float cameraX = 0.0f;
    float coyoteTimer = 0.0f;
    float jumpBufferTimer = 0.0f;
    double lastUpdateMs = 0.0;
    double lastRenderMs = 0.0;
    int fixedStepsThisFrame = 0;

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
    AE::Editor::SampleDiagnosticsOverlay diagnostics;
#endif

    std::vector<std::string> levelTiles;
    Player player;
    AE::Physics::Vector2D playerSpawn;
    std::vector<Coin> coins;
    std::vector<Enemy> enemies;
    RectF finish;

    bool Initialize();
    void Shutdown();
    void ToggleFullscreen();
    void BuildLevel();
    void ResetLevel();
    void ResetPlayer();
    void PollEvents(InputState& input);
    void Step(float dt, const InputState& input);
    void UpdatePlayer(float dt, const InputState& input);
    void UpdateEnemies(float dt);
    void UpdateCoins();
    void UpdateHazards();
    void UpdateGoal();
    void ResolveTileCollisions(bool horizontal);
    void UpdateCamera();

    bool IsSolidTile(int tileX, int tileY) const;
    RectF PlayerRect() const;
    RectF EnemyRect(const Enemy& enemy) const;
    static bool Intersects(const RectF& first, const RectF& second);
    static int ClampInt(int value, int minValue, int maxValue);
    static float ClampFloat(float value, float minValue, float maxValue);
    static float MoveTowardZero(float value, float amount);

    void Render();
    bool CreateFrameTexture();
    void BeginFrameTexture();
    void PresentFrameTexture();
    SDL_Rect CalculateFrameViewport() const;
    void RenderDiagnostics();
    void DrawBackground() const;
    void DrawLevel() const;
    void DrawCoins() const;
    void DrawEnemies() const;
    void DrawPlayer() const;
    void DrawFinish() const;
    void DrawHud() const;
    void DrawRect(const RectF& rect, SDL_Color color) const;
    void DrawScreenRect(int x, int y, int w, int h, SDL_Color color) const;
};

#endif
