#ifndef PLATFORMER_GAME_MODULE_H
#define PLATFORMER_GAME_MODULE_H

#include <SDL2/SDL.h>

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <sol/sol.hpp>

#include "Classes/World.h"
#include "Core/BuildConfig.h"
#include "Core/Math/Vector2D.h"
#include "Core/Platform/PlatformTypes.h"
#include "Game/GameModuleInterface.h"
#include "Physics/Dynamics/Body.h"

namespace AE
{
struct RuntimeRenderContextSDL;
}

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
#include "Scene/Object.h"
#endif

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
#include "SampleDiagnosticsOverlay.h"
#endif

class PlatformerGameModule : public AE::IGameModule
{
public:
	PlatformerGameModule();

	const char* GetName() const override;
	void RegisterSceneObjects(AE::Scene::ObjectFactory& objectFactory) override;
	bool StartPlay(const AE::GameModuleStartContext& context, std::string* error) override;
	void Tick(const AE::GameModuleTickContext& context) override;
	void Render(const AE::GameModuleRenderContext& context) override;
	void StopPlay() override;

	int Run();
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	void SetEditorScenePath(std::filesystem::path Path);
#endif
#if SMOKE_TEST
	bool RunSmokeTest();
#endif
#if SMOKE_TEST && AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	AE::Math::FVector2D GetPlayerSpawnForTests() const;
	SizeT GetEditorSolidPlatformCountForTests() const;
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
		AE::Math::FVector2D position;
		AE::Math::FVector2D velocity;
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
		AE::Math::FVector2D spawnPosition;
		AE::Math::FVector2D position;
		AE::Math::FVector2D velocity;
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
		AE::Math::FVector2D position;
		AE::Math::FVector2D velocity;
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
		AE::Math::FVector2D basePosition;
		AE::Math::FVector2D axis;
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

	struct FEditorSolidPlatform
	{
		RectF Bounds;
		float RotationDegrees = 0.0f;
	};

	struct FCollisionProjection
	{
		float Min = 0.0f;
		float Max = 0.0f;
	};

	struct FCollisionResult
	{
		bool Intersects = false;
		AE::Math::FVector2D MinimumTranslation;
	};

	struct ScenePhysicsBodySpec
	{
		enum class Type
		{
			Box,
			Circle,
			MovingPlatform
		};

		Type type = Type::Box;
		std::string name;
		RectF bounds;
		bool verticalMotion = false;
		float Mass = 1.1f;
		float Friction = 0.24f;
		float Restitution = 0.08f;
		float MotionAmplitude = 92.0f;
		float MotionSpeed = 1.15f;
		float MotionPhase = 0.0f;
		float RotationDegrees = 0.0f;
	};

	struct ScenePhysicsRigSpec
	{
		enum class Type
		{
			Bridge,
			Chain
		};

		Type type = Type::Bridge;
		std::string name;
		RectF bounds;
		int32 SegmentCount = 0;
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
	bool ownsImageSystem = false;
#endif
	bool running = false;
	bool SimulateOnly = false;
#if SMOKE_TEST
	bool smokeMode = false;
#endif
	bool runtimePlayerMode = false;
	bool ownsSdl = false;
	bool ownsWindow = false;
	bool ownsRenderer = false;
	bool fullscreen = false;
	bool paused = false;
	bool jumpKeyWasDown = false;
	bool shootKeyWasDown = false;
	bool pauseKeyWasDown = false;
	bool resetKeyWasDown = false;
	bool pendingJumpPressed = false;
	bool pendingShootPressed = false;
	float cameraX = 0.0f;
	float fixedStepAccumulator = 0.0f;
	float coyoteTimer = 0.0f;
	float jumpBufferTimer = 0.0f;
	double lastUpdateMs = 0.0;
	double lastRenderMs = 0.0;
	int fixedStepsThisFrame = 0;
	float shootCooldownTimer = 0.0f;
	float physicsSceneTime = 0.0f;
	bool scriptedEnemiesLoaded = false;
	bool sceneEnemiesLoaded = false;
	bool scenePhysicsLoaded = false;
	std::string enemyScriptPath;

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
	AE::Editor::SampleDiagnosticsOverlay diagnostics;
#endif

	sol::state lua;
	std::unique_ptr<AE::Physics::World> physicsWorld;
	std::vector<std::string> levelTiles;
	std::vector<SolidPlatform> solidPlatforms;
	Player player;
	AE::Math::FVector2D playerSpawn;
	std::vector<Coin> coins;
	std::vector<Enemy> enemies;
	std::vector<Projectile> projectiles;
	std::vector<BodyVisual> physicsVisuals;
	std::vector<KinematicBody> kinematicBodies;
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	std::filesystem::path projectContentRoot;
	std::filesystem::path editorScenePathOverride;
	std::unique_ptr<Registry> editorSceneRegistry;
	std::vector<std::unique_ptr<AE::Scene::Object>> editorSceneObjects;
	std::vector<EditorSceneProp> editorSceneProps;
	std::vector<FEditorSolidPlatform> EditorSolidPlatforms;
	std::vector<Enemy> editorSceneEnemies;
	std::vector<ScenePhysicsBodySpec> editorScenePhysicsBodies;
	std::vector<ScenePhysicsRigSpec> editorScenePhysicsRigs;
	std::unordered_map<std::string, SDL_Texture*> editorSceneTextures;
	bool sceneDrivenLevel = false;
#endif
	RectF finish;

	bool Initialize();
	void Shutdown();
	void ToggleFullscreen();
	bool AttachRuntimeRenderer(const AE::RuntimeRenderContextSDL& context);
	void BuildLevel();
	void LoadScriptedEnemies();
	void LoadFallbackEnemies();
	void BuildPhysicsScene();
	void BuildDefaultPhysicsPlayground();
	void ResetLevel();
	void ResetPlayer();
	void PollEvents(InputState& input);
	void ReadKeyboardInput(InputState& input);
	void StepRuntime(float deltaSeconds);
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
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	static float DegreesToRadians(float Degrees);
	static AE::Math::FVector2D RectCenter(const RectF& Bounds);
	static std::array<AE::Math::FVector2D, 4> RectVertices(const RectF& Bounds);
	static std::array<AE::Math::FVector2D, 4> RotatedRectVertices(const RectF& Bounds, float RotationDegrees);
	static FCollisionProjection ProjectVertices(const std::array<AE::Math::FVector2D, 4>& Vertices, const AE::Math::FVector2D& Axis);
	static FCollisionResult IntersectAabbWithRotatedRect(const RectF& Bounds, const FEditorSolidPlatform& Platform);
	static bool IntersectsEditorSolidPlatform(const RectF& Bounds, const FEditorSolidPlatform& Platform);
	bool ResolveEditorSolidPlatformCollision(const FEditorSolidPlatform& Platform, bool Horizontal);
#endif

	void Render();
	void RenderFrameContents();
	bool CreateFrameTexture();
	void BeginFrameTexture();
	void PresentFrameTexture();
	SDL_Rect CalculateFrameViewport() const;
	SDL_Rect CalculateFrameViewport(const SDL_Rect& outputBounds) const;
	void CopyFrameTextureToTarget(const SDL_Rect& outputBounds);
	void RenderDiagnostics();
	void DrawBackground() const;
	void DrawLevel() const;
	void DrawCoins() const;
	void DrawEnemies() const;
	void DrawProjectiles() const;
	void DrawPhysics() const;
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	void LoadEditorSceneProps();
	void LoadEditorSceneProps(const AE::Scene::Document& SceneDocument, const std::string& SceneLabel);
	void ClearEditorSceneProps();
	void RefreshEditorSceneTextures();
	bool BuildEditorSceneEnemy(const AE::Scene::ObjectData& objectData, const RectF& bounds, Enemy& enemy) const;
	void BuildEditorScenePhysics();
	void AddScenePhysicsBody(const ScenePhysicsBodySpec& spec);
	void AddScenePhysicsBridge(const ScenePhysicsRigSpec& spec);
	void AddScenePhysicsChain(const ScenePhysicsRigSpec& spec);
	RectF EditorSceneObjectBounds(const AE::Scene::ObjectData& objectData) const;
	std::filesystem::path ResolveEditorSceneAssetPath(const std::string& assetId) const;
	SDL_Texture* GetEditorSceneTexture(const std::string& assetId, const std::filesystem::path& path);
	void DrawEditorSceneProps() const;
#endif
	void DrawPlayer() const;
	void DrawFinish() const;
	void DrawHud() const;
	void DrawRect(const RectF& rect, SDL_Color color) const;
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	void DrawRotatedRect(const RectF& Bounds, float RotationDegrees, SDL_Color Color) const;
#endif
	void DrawWorldLine(const AE::Math::FVector2D& from, const AE::Math::FVector2D& to, SDL_Color color) const;
	void DrawFilledCircle(int centerX, int centerY, int radius, SDL_Color color) const;
	void DrawFilledPolygon(const std::vector<AE::Math::FVector2D>& vertices, SDL_Color color) const;
	void DrawPolyline(const std::vector<AE::Math::FVector2D>& vertices, SDL_Color color, bool closed) const;
	void DrawScreenRect(int x, int y, int w, int h, SDL_Color color) const;
	AE::Physics::Body* AddPhysicsBox(
		AE::Math::FVector2D position,
		float width,
		float height,
		float mass,
		SDL_Color fill,
		SDL_Color edge,
		float rotation = 0.0f,
		bool gameplayBody = true);
	AE::Physics::Body* AddPhysicsCircle(
		AE::Math::FVector2D position,
		float radius,
		float mass,
		SDL_Color fill,
		SDL_Color edge,
		bool gameplayBody = true);
	void AddPhysicsJoint(AE::Physics::Body* first, AE::Physics::Body* second);
};

#endif
