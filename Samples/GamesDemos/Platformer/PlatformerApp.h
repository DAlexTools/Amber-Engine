#ifndef PLATFORMER_APP_H
#define PLATFORMER_APP_H

#include <SDL2/SDL.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <sol/sol.hpp>

#include "Classes/World.h"
#include "Core/BuildConfig.h"
#include "Core/Math/Vector2D.h"
#include "Physics/Objects/Body.h"

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
#include "Scene/Object.h"
#endif

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
        int facing = 1;
        int airJumpsRemaining = 1;
    };

    struct Coin
    {
        RectF bounds;
        bool collected = false;
    };

    struct Enemy
    {
        std::string name;
        AE::Physics::Vector2D spawnPosition;
        AE::Physics::Vector2D position;
        AE::Physics::Vector2D velocity;
        float width = 30.0f;
        float height = 26.0f;
        float speed = 70.0f;
        float direction = 1.0f;
        float leftBound = 0.0f;
        float rightBound = 0.0f;
        float groundY = 0.0f;
        float timeAlive = 0.0f;
        float timeSinceJump = 0.0f;
        float jumpCooldown = 1.2f;
        float jumpVelocity = -340.0f;
        float alertRange = 240.0f;
        int maxHealth = 1;
        int health = 1;
        bool alive = true;
        SDL_Color color{174, 54, 62, 255};
        float shootCooldown = 1.35f;
        float shootTimer = 0.0f;
        float shootRange = 420.0f;
        float projectileSpeed = 360.0f;
        bool canShoot = false;
        sol::function updateScript;
    };

    struct Projectile
    {
        AE::Physics::Vector2D position;
        AE::Physics::Vector2D velocity;
        float width = 16.0f;
        float height = 6.0f;
        float timeToLive = 1.2f;
        int damage = 1;
        bool active = true;
        bool fromPlayer = true;
    };

    struct SolidPlatform
    {
        int tileX = 0;
        int tileY = 0;
        int width = 0;
        int height = 0;
    };

    struct BodyVisual
    {
        AE::Physics::Body* body = nullptr;
        SDL_Color fill{};
        SDL_Color edge{};
        bool gameplayBody = false;
    };

    struct KinematicBody
    {
        AE::Physics::Body* body = nullptr;
        AE::Physics::Vector2D basePosition;
        AE::Physics::Vector2D axis;
        float amplitude = 0.0f;
        float speed = 0.0f;
        float phase = 0.0f;
    };

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
    struct EditorSceneProp
    {
        std::string name;
        std::string assetId;
        RectF bounds;
        float rotationDegrees = 0.0f;
        SDL_Texture* texture = nullptr;
    };
#endif

    struct InputState
    {
        bool moveLeft = false;
        bool moveRight = false;
        bool jumpPressed = false;
        bool jumpHeld = false;
        bool shootPressed = false;
    };

    static constexpr int WindowWidth = 960;
    static constexpr int WindowHeight = 540;
    static constexpr int TileSize = 32;
    static constexpr int LevelCols = 96;
    static constexpr int LevelRows = 17;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* frameTexture = nullptr;
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
    bool imageSystemInitialized = false;
#endif
    bool running = false;
#if SMOKE_TEST
    bool smokeMode = false;
#endif
    bool fullscreen = false;
    bool paused = false;
    bool jumpKeyWasDown = false;
    bool shootKeyWasDown = false;
    bool pendingJumpPressed = false;
    bool pendingShootPressed = false;
    float cameraX = 0.0f;
    float coyoteTimer = 0.0f;
    float jumpBufferTimer = 0.0f;
    double lastUpdateMs = 0.0;
    double lastRenderMs = 0.0;
    int fixedStepsThisFrame = 0;
    float shootCooldownTimer = 0.0f;
    float physicsSceneTime = 0.0f;
    bool scriptedEnemiesLoaded = false;
    std::string enemyScriptPath;

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
    AE::Editor::SampleDiagnosticsOverlay diagnostics;
#endif

    sol::state lua;
    std::unique_ptr<AE::Physics::World> physicsWorld;
    std::vector<std::string> levelTiles;
    std::vector<SolidPlatform> solidPlatforms;
    Player player;
    AE::Physics::Vector2D playerSpawn;
    std::vector<Coin> coins;
    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;
    std::vector<BodyVisual> physicsVisuals;
    std::vector<KinematicBody> kinematicBodies;
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
    std::filesystem::path projectContentRoot;
    std::unique_ptr<Registry> editorSceneRegistry;
    std::vector<std::unique_ptr<AE::Scene::Object>> editorSceneObjects;
    std::vector<EditorSceneProp> editorSceneProps;
    std::vector<RectF> editorSolidPlatforms;
    std::unordered_map<std::string, SDL_Texture*> editorSceneTextures;
#endif
    RectF finish;

    bool Initialize();
    void Shutdown();
    void ToggleFullscreen();
    void BuildLevel();
    void LoadScriptedEnemies();
    void LoadFallbackEnemies();
    void BuildPhysicsScene();
    void ResetLevel();
    void ResetPlayer();
    void PollEvents(InputState& input);
    void Step(float dt, const InputState& input);
    void TryShoot();
    void TryEnemyShoot(Enemy& enemy);
    void UpdatePlayer(float dt, const InputState& input);
    void UpdateEnemies(float dt);
    void UpdateProjectiles(float dt);
    void StepPhysics(float dt);
    void UpdateKinematicPhysicsBodies(float dt);
    void ResolvePlayerPhysicsContacts(float dt);
    void ApplyProjectilePhysicsHit(Projectile& projectile);
    void UpdateCoins();
    void UpdateHazards();
    void UpdateGoal();
    void ResolveTileCollisions(bool horizontal);
    void UpdateCamera();

    bool IsSolidTile(int tileX, int tileY) const;
    RectF PlayerRect() const;
    RectF EnemyRect(const Enemy& enemy) const;
    RectF ProjectileRect(const Projectile& projectile) const;
    RectF BodyBounds(const AE::Physics::Body& body) const;
    bool IsKinematicBody(const AE::Physics::Body* body) const;
    int AliveEnemyCount() const;
    int PhysicsBodyCount() const;
    int PhysicsConstraintCount() const;
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
    void DrawProjectiles() const;
    void DrawPhysics() const;
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
    void LoadEditorSceneProps();
    void ClearEditorSceneProps();
    RectF EditorSceneObjectBounds(const AE::Scene::ObjectData& objectData) const;
    std::filesystem::path ResolveEditorSceneAssetPath(const std::string& assetId) const;
    SDL_Texture* GetEditorSceneTexture(const std::string& assetId, const std::filesystem::path& path);
    void DrawEditorSceneProps() const;
#endif
    void DrawPlayer() const;
    void DrawFinish() const;
    void DrawHud() const;
    void DrawRect(const RectF& rect, SDL_Color color) const;
    void DrawWorldLine(const AE::Physics::Vector2D& from, const AE::Physics::Vector2D& to, SDL_Color color) const;
    void DrawFilledCircle(int centerX, int centerY, int radius, SDL_Color color) const;
    void DrawFilledPolygon(const std::vector<AE::Physics::Vector2D>& vertices, SDL_Color color) const;
    void DrawPolyline(const std::vector<AE::Physics::Vector2D>& vertices, SDL_Color color, bool closed) const;
    void DrawScreenRect(int x, int y, int w, int h, SDL_Color color) const;
    AE::Physics::Body* AddPhysicsBox(
        AE::Physics::Vector2D position,
        float width,
        float height,
        float mass,
        SDL_Color fill,
        SDL_Color edge,
        float rotation = 0.0f,
        bool gameplayBody = true);
    AE::Physics::Body* AddPhysicsCircle(
        AE::Physics::Vector2D position,
        float radius,
        float mass,
        SDL_Color fill,
        SDL_Color edge,
        bool gameplayBody = true);
    void AddPhysicsJoint(AE::Physics::Body* first, AE::Physics::Body* second);
};

#endif
