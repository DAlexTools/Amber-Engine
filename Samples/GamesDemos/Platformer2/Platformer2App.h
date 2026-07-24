#ifndef PLATFORMER2_APP_H
#define PLATFORMER2_APP_H

#include <SDL2/SDL.h>

#include <array>
#include <string>
#include <vector>

#ifndef SMOKE_TEST
#define SMOKE_TEST 0
#endif

class Platformer2App
{
public:
    Platformer2App();

    int Run();
#if SMOKE_TEST
    bool RunSmokeTest();
#endif

private:
    struct Vec2
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct RectF
    {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
    };

    enum class TileKind
    {
        Empty,
        Solid,
        Ladder,
        Spike,
        Coin,
        Goal,
        Decor
    };

    struct TileCell
    {
        TileKind kind = TileKind::Empty;
        int visual = -1;
    };

    struct Player
    {
        Vec2 position;
        Vec2 velocity;
        float width = 22.0f;
        float height = 30.0f;
        int facing = 1;
        int airJumpsRemaining = 1;
        int coins = 0;
        bool grounded = false;
        bool onLadder = false;
        bool won = false;
        float animationTime = 0.0f;
    };

    struct Enemy
    {
        Vec2 spawnPosition;
        Vec2 position;
        Vec2 velocity;
        float width = 24.0f;
        float height = 28.0f;
        float leftBound = 0.0f;
        float rightBound = 0.0f;
        float speed = 78.0f;
        int facing = -1;
        int health = 2;
        bool alive = true;
        float animationTime = 0.0f;
    };

    struct Projectile
    {
        Vec2 position;
        Vec2 velocity;
        float width = 12.0f;
        float height = 6.0f;
        float timeToLive = 1.1f;
        bool active = true;
    };

    struct Lift
    {
        Vec2 basePosition;
        Vec2 previousPosition;
        Vec2 position;
        Vec2 axis;
        float width = 96.0f;
        float height = 16.0f;
        float amplitude = 120.0f;
        float speed = 1.0f;
        float phase = 0.0f;
    };

    struct InputState
    {
        bool moveLeft = false;
        bool moveRight = false;
        bool climbUp = false;
        bool climbDown = false;
        bool jumpPressed = false;
        bool jumpHeld = false;
        bool shootPressed = false;
    };

    static constexpr int WindowWidth = 960;
    static constexpr int WindowHeight = 540;
    static constexpr int SourceTileSize = 16;
    static constexpr int WorldTileSize = 32;
    static constexpr int SheetColumns = 20;
    static constexpr int LevelCols = 118;
    static constexpr int LevelRows = 21;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* tilemapTexture = nullptr;
    bool imageSystemInitialized = false;
    bool running = false;
    bool fullscreen = false;
    bool paused = false;
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    float worldTime = 0.0f;
    float coyoteTimer = 0.0f;
    float jumpBufferTimer = 0.0f;
    float shootCooldownTimer = 0.0f;

    std::array<std::array<TileCell, LevelCols>, LevelRows> level;
    Player player;
    Vec2 playerSpawn;
    RectF finish;
    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;
    std::vector<Lift> lifts;

    bool Initialize();
    bool LoadContent();
    void Shutdown();
    void ToggleFullscreen();

    void ResetGame();
    void ResetPlayer();
    void BuildLevel();
    void SetTile(int x, int y, TileKind kind, int visual);
    void FillRectTiles(int x, int y, int width, int height, TileKind kind, int visual);
    void FillPlatform(int xStart, int xEnd, int y);
    void AddLadder(int x, int yStart, int yEnd);
    void AddSpikes(int xStart, int xEnd, int y);
    void AddCoinLine(int xStart, int xEnd, int y);
    void AddEnemy(int xTile, int platformY, int leftTile, int rightTile, float speed);
    void AddLift(float x, float y, Vec2 axis, float amplitude, float speed, float phase);

    void PollEvents(InputState& input);
    void Step(float dt, const InputState& input);
    void UpdateLifts(float dt);
    void UpdatePlayer(float dt, const InputState& input);
    void UpdateEnemies(float dt);
    void UpdateProjectiles(float dt);
    void UpdatePickupsAndHazards();
    void UpdateCamera();
    void TryShoot();
    void ResolveTileCollisions(bool horizontal);
    void ResolveLiftCollisions(const Vec2& previousPlayerPosition);
    void ResolveEnemyVerticalCollision(Enemy& enemy);

    bool IsSolidTile(int x, int y) const;
    bool IsKindInRect(const RectF& rect, TileKind kind) const;
    RectF PlayerRect() const;
    RectF EnemyRect(const Enemy& enemy) const;
    RectF ProjectileRect(const Projectile& projectile) const;
    RectF LiftRect(const Lift& lift) const;
    bool IsPlayerOnLift(const Lift& lift) const;
    static bool Intersects(const RectF& first, const RectF& second);
    static bool OverlapsHorizontally(const RectF& first, const RectF& second);
    static int ClampInt(int value, int minValue, int maxValue);
    static float ClampFloat(float value, float minValue, float maxValue);
    static float MoveTowardZero(float value, float amount);

    void Render();
    void DrawBackground() const;
    void DrawLevel() const;
    void DrawLifts() const;
    void DrawEnemies() const;
    void DrawProjectiles() const;
    void DrawPlayer() const;
    void DrawHud() const;
    void DrawTile(int tileId, float worldX, float worldY, int width = WorldTileSize, int height = WorldTileSize, bool flip = false) const;
    void DrawScreenTile(int tileId, int x, int y, int width = WorldTileSize, int height = WorldTileSize) const;
    void DrawWorldRect(const RectF& rect, SDL_Color color) const;
    void DrawScreenRect(int x, int y, int w, int h, SDL_Color color) const;
    void DrawSegmentedLift(const Lift& lift) const;
};

#endif
