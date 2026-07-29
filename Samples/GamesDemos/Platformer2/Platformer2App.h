#ifndef PLATFORMER2_APP_H
#define PLATFORMER2_APP_H

#include <SDL2/SDL.h>

#include <array>
#include <string>
#include <vector>

#include "Core/Platform/PlatformTypes.h"

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

	enum class EditorTool
	{
		Select,
		Tile,
		Erase,
		Enemy,
		Lift,
		PlayerSpawn,
		Goal
	};

	enum class EditorSelectionType
	{
		None,
		Tile,
		Enemy,
		Lift,
		PlayerSpawn,
		Goal
	};

	struct EditorSelection
	{
		EditorSelectionType type = EditorSelectionType::None;
		int x = 0;
		int y = 0;
		SizeT index = 0;
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
#ifdef AMBER_ENABLE_PLATFORMER2_EDITOR
	bool imguiReady = false;
#endif
	bool running = false;
	bool fullscreen = false;
	bool paused = false;
	bool jumpKeyWasDown = false;
	bool shootKeyWasDown = false;
	bool pendingJumpPressed = false;
	bool pendingShootPressed = false;
	float cameraX = 0.0f;
	float cameraY = 0.0f;
	float worldTime = 0.0f;
	float coyoteTimer = 0.0f;
	float jumpBufferTimer = 0.0f;
	float shootCooldownTimer = 0.0f;
	bool forceDefaultMap = false;

	bool editorMode = false;
	bool editorShowGrid = true;
	bool editorShowPalette = true;
	bool editorViewportHovered = false;
	bool editorPaletteHovered = false;
	bool editorMouseWasDown = false;
	bool editorRightMouseWasDown = false;
	bool editorWasPausedBeforeOpen = false;
	float editorViewportX = 304.0f;
	float editorViewportY = 86.0f;
	float editorViewportW = 420.0f;
	float editorViewportH = 400.0f;
	float editorPaletteX = 22.0f;
	float editorPaletteY = 220.0f;
	float editorPaletteW = 248.0f;
	float editorPaletteH = 248.0f;
	float editorZoom = 0.75f;
	int editorTool = static_cast<int>(EditorTool::Select);
	int editorTileKind = static_cast<int>(TileKind::Solid);
	int editorTileVisual = 160;
	EditorSelection editorSelection;
	std::string editorStatus;

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
	void ToggleEditorMode();

	void ResetGame();
	void ResetPlayer();
	void BuildLevel();
	void BuildDefaultLevel();
	bool LoadMap();
	bool SaveMap() const;
	void SetTile(int x, int y, TileKind kind, int visual);
	void FillRectTiles(int x, int y, int width, int height, TileKind kind, int visual);
	void FillPlatform(int xStart, int xEnd, int y);
	void AddLadder(int x, int yStart, int yEnd);
	void AddSpikes(int xStart, int xEnd, int y);
	void AddCoinLine(int xStart, int xEnd, int y);
	void AddEnemy(int xTile, int platformY, int leftTile, int rightTile, float speed);
	void AddEnemyAtWorld(float x, float y, float leftBound, float rightBound, float speed);
	void AddLift(float x, float y, Vec2 axis, float amplitude, float speed, float phase);
	void AddLiftAtWorld(float x, float y, Vec2 axis, float width, float height, float amplitude, float speed, float phase);
	void ClearGoalTiles();
	void SetGoalTilesFromFinish();

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
	const char* TileKindName(TileKind kind) const;

	void Render();
	void DrawBackground() const;
	void DrawLevel() const;
	void DrawLifts() const;
	void DrawEnemies() const;
	void DrawProjectiles() const;
	void DrawPlayer() const;
	void DrawHud() const;
	void DrawEditorOverlay() const;
	void DrawEditorViewport() const;
	void DrawEditorWorldTile(int tileId, float worldX, float worldY, int width = WorldTileSize, int height = WorldTileSize, bool flip = false) const;
	void DrawEditorWorldRect(const RectF& rect, SDL_Color color) const;
	void DrawEditorSegmentedLift(const Lift& lift) const;
	void DrawEditorTilePalette() const;
	bool PickEditorPaletteTile(float logicalX, float logicalY, int& tileId) const;
	bool EditorScreenToWorld(float logicalX, float logicalY, float& worldX, float& worldY) const;
	TileKind GuessTileKindForVisual(int tileId) const;
#ifdef AMBER_ENABLE_PLATFORMER2_EDITOR
	void BeginEditorFrame();
	void RenderEditor();
	void DrawEditorWindows();
	void ApplyEditorMouse();
	void SelectAtWorld(float worldX, float worldY);
	void DeleteEditorSelection();
	const char* EditorToolName(EditorTool tool) const;
	const char* SelectionName() const;
#endif
	void DrawTile(int tileId, float worldX, float worldY, int width = WorldTileSize, int height = WorldTileSize, bool flip = false) const;
	void DrawScreenTile(int tileId, int x, int y, int width = WorldTileSize, int height = WorldTileSize) const;
	void DrawWorldRect(const RectF& rect, SDL_Color color) const;
	void DrawScreenRect(int x, int y, int w, int h, SDL_Color color) const;
	void DrawSegmentedLift(const Lift& lift) const;
};

#endif
