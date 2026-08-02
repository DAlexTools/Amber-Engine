#include "PlatformerGameModule.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <sstream>
#include <utility>

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
#include <SDL2/SDL_image.h>
#endif

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
#include "imgui.h"
#endif

#include "Logging/Logger.h"
#include "Game/RuntimeViewerSDL.h"
#include "Physics/Constraints/Constraint.h"
#include "Physics/Geometry/Shape.h"
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
#include "PlatformerSceneObjects.h"
#include "Scene/ObjectFactory.h"
#include "Scene/SceneAsset.h"
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
constexpr int MaxAirJumps = 1;
constexpr float EnemyGravity = 1350.0f;
constexpr float ProjectileSpeed = 680.0f;
constexpr float ProjectileCooldown = 0.18f;
constexpr float PhysicsFixedTimeStep = 1.0f / 120.0f;
constexpr uint32 PhysicsCategoryTerrain = 0x00000001u;
constexpr uint32 PhysicsCategoryDynamic = 0x00000002u;
constexpr uint32 PhysicsCategorySensor = 0x00000004u;
constexpr float Pi = 3.14159265358979323846f;

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
SDL_Color EnemyProjectileColor{235, 88, 118, 255};
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

bool ReadBool(sol::table table, const char* key, bool defaultValue)
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
		static_cast<Uint8>(ReadInt(color, "a", defaultValue.a))};
}

std::filesystem::path FindPlatformerEnemyScript()
{
	namespace fs = std::filesystem;

	const std::array<fs::path, 2> relativePaths = {
		fs::path("Content") / "Scripts" / "PlatformerEnemies.lua",
		fs::path("Projects") / "Platformer" / "Content" / "Scripts" / "PlatformerEnemies.lua"};
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
		fs::current_path() / ".." / ".." / ".." / ".." / "AmberEngine"};

	for (const fs::path& root : candidateRoots)
	{
		for (const fs::path& relativePath : relativePaths)
		{
			const fs::path candidate = root / relativePath;
			std::error_code error;
			if (fs::exists(candidate, error))
			{
				return fs::weakly_canonical(candidate, error);
			}
		}
	}

	return {};
}

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
std::filesystem::path FindProjectContentRoot()
{
	namespace fs = std::filesystem;

	const std::array<fs::path, 2> markerPaths = {
		fs::path("Content") / "Scripts" / "PlatformerEnemies.lua",
		fs::path("Projects") / "Platformer" / "Content" / "Scripts" / "PlatformerEnemies.lua"};
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
		fs::current_path() / ".." / ".." / ".." / ".." / "AmberEngine"};

	for (const fs::path& root : candidateRoots)
	{
		for (const fs::path& markerPath : markerPaths)
		{
			const fs::path marker = root / markerPath;
			std::error_code error;
			if (fs::exists(marker, error))
			{
				return markerPath.is_relative() && markerPath.begin()->string() == "Projects" ? fs::weakly_canonical(root / "Projects" / "Platformer" / "Content", error) : fs::weakly_canonical(root / "Content", error);
			}
		}
	}

	return {};
}

std::filesystem::path FindEngineContentRoot()
{
	namespace fs = std::filesystem;

	const fs::path relativePath = fs::path("Engine") / "Content";
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
		fs::current_path() / ".." / ".." / ".." / ".." / "AmberEngine"};

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

bool HasPrefix(const std::string& value, const std::string& prefix)
{
	return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

bool ContainsText(const std::string& value, const std::string& token)
{
	return std::search(
			   value.begin(),
			   value.end(),
			   token.begin(),
			   token.end(),
			   [](char left, char right)
			   {
				   return std::tolower(static_cast<unsigned char>(left)) ==
						  std::tolower(static_cast<unsigned char>(right));
			   }) != value.end();
}
#endif
} // namespace

PlatformerGameModule::PlatformerGameModule()
{
	AE::Logger::SetConsoleEnabled(false);
	lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);
	BuildLevel();
	ResetLevel();
}

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
void PlatformerGameModule::SetEditorScenePath(std::filesystem::path Path)
{
	editorScenePathOverride = std::move(Path);
}
#endif

const char* PlatformerGameModule::GetName() const
{
	return "PlatformerGameModule";
}

#if SMOKE_TEST && AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
AE::Math::FVector2D PlatformerGameModule::GetPlayerSpawnForTests() const
{
	return playerSpawn;
}

SizeT PlatformerGameModule::GetEditorSolidPlatformCountForTests() const
{
	return EditorSolidPlatforms.size();
}
#endif

void PlatformerGameModule::RegisterSceneObjects(AE::Scene::ObjectFactory& objectFactory)
{
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	PlatformerScene::RegisterPlatformerSceneObjects(objectFactory);
#else
	(void)objectFactory;
#endif
}

bool PlatformerGameModule::StartPlay(const AE::GameModuleStartContext& context, std::string*)
{
	runtimePlayerMode = true;
	SimulateOnly = context.RunMode == AE::EGameModuleRunMode::Simulate;
	running = true;
	paused = false;
	fullscreen = false;
	fixedStepAccumulator = 0.0f;
	fixedStepsThisFrame = 0;
	jumpKeyWasDown = false;
	shootKeyWasDown = false;
	pauseKeyWasDown = false;
	resetKeyWasDown = false;
	pendingJumpPressed = false;
	pendingShootPressed = false;
	lastUpdateMs = 0.0;
	lastRenderMs = 0.0;

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	projectContentRoot = context.projectRoot / "Content";
	if (!context.scenePath.empty())
	{
		editorScenePathOverride = context.scenePath;
	}
	const std::string SceneLabel = context.scenePath.empty() ? std::string("PIE") : context.scenePath.string();
	LoadEditorSceneProps(context.sceneDocument, SceneLabel);
#else
	(void)context;
#endif
	ResetLevel();
	AE::Logger::Log("PlatformerGameModule StartPlay", "Platformer");
	return true;
}

void PlatformerGameModule::Tick(const AE::GameModuleTickContext& context)
{
	StepRuntime(context.deltaSeconds);
}

void PlatformerGameModule::Render(const AE::GameModuleRenderContext& context)
{
	const auto* runtimeContext = static_cast<const AE::RuntimeRenderContextSDL*>(context.nativeRenderContext);
	if (!runtimeContext || !AttachRuntimeRenderer(*runtimeContext))
	{
		return;
	}

	SDL_Texture* previousTarget = SDL_GetRenderTarget(renderer);
	SDL_Rect previousViewport{};
	SDL_RenderGetViewport(renderer, &previousViewport);
	int previousLogicalWidth = 0;
	int previousLogicalHeight = 0;
	SDL_RenderGetLogicalSize(renderer, &previousLogicalWidth, &previousLogicalHeight);
	float previousScaleX = 1.0f;
	float previousScaleY = 1.0f;
	SDL_RenderGetScale(renderer, &previousScaleX, &previousScaleY);
	SDL_BlendMode previousBlendMode = SDL_BLENDMODE_NONE;
	SDL_GetRenderDrawBlendMode(renderer, &previousBlendMode);

	RenderFrameContents();

	SDL_SetRenderTarget(renderer, previousTarget);
	SDL_RenderSetLogicalSize(renderer, previousLogicalWidth, previousLogicalHeight);
	SDL_RenderSetViewport(renderer, &previousViewport);
	SDL_RenderSetScale(renderer, previousScaleX, previousScaleY);
	CopyFrameTextureToTarget(runtimeContext->viewport);
	SDL_SetRenderDrawBlendMode(renderer, previousBlendMode);
}

void PlatformerGameModule::StopPlay()
{
	if (runtimePlayerMode)
	{
		Shutdown();
		runtimePlayerMode = false;
	}
	SimulateOnly = false;
	AE::Logger::Log("PlatformerGameModule StopPlay", "Platformer");
}

namespace
{
double ElapsedMs(Uint64 startCounter, Uint64 endCounter)
{
	return static_cast<double>(endCounter - startCounter) * 1000.0 /
		   static_cast<double>(SDL_GetPerformanceFrequency());
}
} // namespace

int PlatformerGameModule::Run()
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
			pendingJumpPressed = false;
			pendingShootPressed = false;
		}
		lastUpdateMs = ElapsedMs(updateStart, SDL_GetPerformanceCounter());

		Render();
	}

	Shutdown();
	return 0;
}

#if SMOKE_TEST
bool PlatformerGameModule::RunSmokeTest()
{
	smokeMode = true;
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	if (!editorScenePathOverride.empty())
	{
		LoadEditorSceneProps();
	}
#endif
	ResetLevel();

	const float startX = player.position.X;
	InputState runRight;
	runRight.moveRight = true;

	for (int frame = 0; frame < 140; ++frame)
	{
		runRight.jumpPressed = frame == 8 || frame == 72;
		runRight.jumpHeld = (frame >= 8 && frame < 26) || (frame >= 72 && frame < 90);
		Step(FixedTimeStep, runRight);
	}

	const bool movedRight = player.position.X > startX + 110.0f;
	const bool stayedInWorld = player.position.Y < static_cast<float>(LevelRows * TileSize);
	const bool hasGroundState = player.grounded || player.velocity.Y >= 0.0f;

	if (!coins.empty())
	{
		player.position.X = coins.front().bounds.x;
		player.position.Y = coins.front().bounds.y;
		UpdateCoins();
	}
	const bool coinCollected = !coins.empty() && coins.front().collected;

	if (!enemies.empty())
	{
		player.position.X = enemies.front().position.X;
		player.position.Y = enemies.front().position.Y;
		UpdateHazards();
	}
	const bool hazardResetsPlayer = std::abs(player.position.X - playerSpawn.X) < 0.5f &&
									std::abs(player.position.Y - playerSpawn.Y) < 0.5f;

	player.position.X = finish.x;
	player.position.Y = finish.y;
	UpdateGoal();
	const bool finishWorks = player.won;

	ResetPlayer();
	for (int frame = 0; frame < 12; ++frame)
	{
		Step(FixedTimeStep, InputState{});
	}
	const float groundedY = player.position.Y;
	player.position.Y = groundedY - 18.0f;
	player.velocity.Y = 340.0f;
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
	const bool bufferedJumpWorks = player.velocity.Y < 0.0f && !player.grounded;

	ResetPlayer();
	for (int frame = 0; frame < 12; ++frame)
	{
		Step(FixedTimeStep, InputState{});
	}
	player.position.X += 2.0f * TileSize;
	player.grounded = false;
	InputState coyoteJumpInput;
	coyoteJumpInput.jumpPressed = true;
	coyoteJumpInput.jumpHeld = true;
	Step(FixedTimeStep, coyoteJumpInput);
	const bool coyoteJumpWorks = player.velocity.Y < 0.0f && !player.grounded;

	ResetPlayer();
	for (int frame = 0; frame < 12; ++frame)
	{
		Step(FixedTimeStep, InputState{});
	}
	InputState firstJumpInput;
	firstJumpInput.jumpPressed = true;
	firstJumpInput.jumpHeld = true;
	Step(FixedTimeStep, firstJumpInput);
	firstJumpInput.jumpPressed = false;
	for (int frame = 0; frame < 10; ++frame)
	{
		Step(FixedTimeStep, firstJumpInput);
	}
	const float beforeDoubleJumpVelocity = player.velocity.Y;
	InputState secondJumpInput;
	secondJumpInput.jumpPressed = true;
	secondJumpInput.jumpHeld = true;
	Step(FixedTimeStep, secondJumpInput);
	const bool doubleJumpWorks = !player.grounded && player.velocity.Y < beforeDoubleJumpVelocity - 90.0f;

	const bool scriptedEnemiesWork = (scriptedEnemiesLoaded || sceneEnemiesLoaded) && enemies.size() >= 4;

	ResetLevel();
	bool shootingWorks = false;
	if (!enemies.empty())
	{
		for (Enemy& enemy : enemies)
		{
			enemy.shootTimer = 999.0f;
		}

		Enemy* target = &enemies.back();
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
		if (sceneEnemiesLoaded)
		{
			for (Enemy& enemy : enemies)
			{
				if (ContainsText(enemy.name, "hopper"))
				{
					target = &enemy;
					break;
				}
			}
		}
#endif
		const float projectileY = target->position.Y + target->height * 0.5f - 3.0f;
		player.position = AE::Math::FVector2D(target->position.X - 190.0f, projectileY - player.height * 0.42f);
		player.velocity = AE::Math::FVector2D::Zero;
		player.facing = 1;

		for (int shot = 0; shot < 5 && target->alive; ++shot)
		{
			InputState shootInput;
			shootInput.shootPressed = true;
			Step(FixedTimeStep, shootInput);
			shootInput.shootPressed = false;
			for (int frame = 0; frame < 34 && target->alive; ++frame)
			{
				Step(FixedTimeStep, shootInput);
			}
		}
		shootingWorks = !target->alive;
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
		const AE::Math::FVector2D startPosition = testBody->position;
		player.position = AE::Math::FVector2D(testBody->position.X - 115.0f, testBody->position.Y - player.height * 0.45f);
		player.velocity = AE::Math::FVector2D::Zero;
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

	ResetLevel();
	bool movingPlatformLandingWorks = false;
	if (!kinematicBodies.empty() && kinematicBodies.front().body)
	{
		const RectF platformBounds = BodyBounds(*kinematicBodies.front().body);
		player.position = AE::Math::FVector2D(
			platformBounds.x + platformBounds.w * 0.5f - player.width * 0.5f,
			platformBounds.y - player.height + 5.0f);
		player.velocity = AE::Math::FVector2D(0.0f, 120.0f);
		player.grounded = false;
		Step(FixedTimeStep, InputState{});
		movingPlatformLandingWorks = player.grounded &&
									 std::abs((player.position.Y + player.height) - BodyBounds(*kinematicBodies.front().body).y) < 1.5f;
	}

	ResetLevel();
	bool enemyShootingWorks = false;
	Enemy* shootingEnemy = nullptr;
	for (Enemy& enemy : enemies)
	{
		if (enemy.canShoot)
		{
			shootingEnemy = &enemy;
			break;
		}
	}
	if (shootingEnemy)
	{
		shootingEnemy->shootTimer = 0.0f;
		player.position = AE::Math::FVector2D(shootingEnemy->position.X + 180.0f, shootingEnemy->position.Y);
		player.velocity = AE::Math::FVector2D::Zero;
		Step(FixedTimeStep, InputState{});

		int enemyProjectileCount = 0;
		for (const Projectile& projectile : projectiles)
		{
			if (!projectile.fromPlayer)
			{
				++enemyProjectileCount;
			}
		}
		enemyShootingWorks = enemyProjectileCount > 0;
	}

	const bool physicsSceneWorks = PhysicsBodyCount() > static_cast<int>(solidPlatforms.size()) + 18 &&
								   PhysicsConstraintCount() >= 12 &&
								   physicsWorld &&
								   physicsWorld->GetLastStats().bodyCount > 0;

	bool editorSceneFileLoads = true;
	bool editorSceneGameplayObjectsWork = true;
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	SizeT editorScenePropCount = 0;
	SizeT editorSceneSpawnCount = 0;
	SizeT editorSceneGoalCount = 0;
	SizeT editorSceneCoinCount = 0;
	SizeT editorSceneSolidPlatformCount = 0;
	SizeT editorSceneEnemyCount = 0;
	SizeT editorScenePhysicsBodyCount = 0;
	SizeT editorScenePhysicsRigCount = 0;
	SizeT editorSceneMovingPlatformCount = 0;
	SizeT editorRuntimeObjectCount = 0;
	editorSceneFileLoads = false;
	editorSceneGameplayObjectsWork = false;
	const std::filesystem::path projectRoot = FindProjectContentRoot();
	if (!projectRoot.empty())
	{
		AE::Scene::Document editorScene;
		if (AE::Scene::LoadScene(projectRoot / "Scenes" / "PlatformerTest.amber.scene", editorScene))
		{
			for (const AE::Scene::ObjectData& object : editorScene.objects)
			{
				if (object.kind == AE::Scene::ObjectKind::AssetInstance && object.visible)
				{
					++editorScenePropCount;
				}
				if (object.className == "PlayerSpawnObject")
				{
					++editorSceneSpawnCount;
				}
				else if (object.className == "GoalObject")
				{
					++editorSceneGoalCount;
				}
				else if (object.className == "CoinObject")
				{
					++editorSceneCoinCount;
				}
				else if (object.className == "SolidPlatformObject")
				{
					++editorSceneSolidPlatformCount;
				}
				else if (object.className == "EnemySpawnObject")
				{
					++editorSceneEnemyCount;
				}
				else if (object.className == "PhysicsBoxObject" || object.className == "PhysicsCircleObject")
				{
					++editorScenePhysicsBodyCount;
				}
				else if (object.className == "PhysicsBridgeObject" || object.className == "PhysicsChainObject")
				{
					++editorScenePhysicsRigCount;
				}
				else if (object.className == "MovingPlatformObject")
				{
					++editorSceneMovingPlatformCount;
				}
			}

			AE::Scene::ObjectFactory factory;
			PlatformerScene::RegisterPlatformerSceneObjects(factory);
			Registry registry;
			const std::vector<std::unique_ptr<AE::Scene::Object>> runtimeObjects = factory.CreateObjects(editorScene, &registry);
			registry.Update();
			for (const std::unique_ptr<AE::Scene::Object>& object : runtimeObjects)
			{
				if (object && object->HasEntity())
				{
					++editorRuntimeObjectCount;
				}
			}
			editorSceneFileLoads = !editorScene.objects.empty();
			editorSceneGameplayObjectsWork = editorSceneSpawnCount >= 1 &&
											 editorSceneGoalCount >= 1 &&
											 editorSceneCoinCount >= 1 &&
											 editorSceneSolidPlatformCount >= 1 &&
											 editorSceneEnemyCount >= 1 &&
											 editorScenePhysicsBodyCount >= 1 &&
											 editorScenePhysicsRigCount >= 1 &&
											 editorSceneMovingPlatformCount >= 1 &&
											 editorRuntimeObjectCount == editorScene.objects.size();
		}
	}
#endif

	const bool passed = movedRight && stayedInWorld && hasGroundState && coinCollected && hazardResetsPlayer && finishWorks &&
						bufferedJumpWorks && coyoteJumpWorks && doubleJumpWorks && scriptedEnemiesWork && shootingWorks && physicsSceneWorks &&
						physicsBodyReacts && movingPlatformLandingWorks && enemyShootingWorks && editorSceneFileLoads && editorSceneGameplayObjectsWork;
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
			<< " doubleJumpWorks=" << doubleJumpWorks
			<< " scriptedEnemiesWork=" << scriptedEnemiesWork
			<< " scriptedEnemiesLoaded=" << scriptedEnemiesLoaded
			<< " sceneEnemiesLoaded=" << sceneEnemiesLoaded
			<< " scenePhysicsLoaded=" << scenePhysicsLoaded
			<< " shootingWorks=" << shootingWorks
			<< " enemyShootingWorks=" << enemyShootingWorks
			<< " physicsSceneWorks=" << physicsSceneWorks
			<< " physicsBodyReacts=" << physicsBodyReacts
			<< " movingPlatformLandingWorks=" << movingPlatformLandingWorks
			<< " editorSceneFileLoads=" << editorSceneFileLoads
			<< " editorSceneGameplayObjectsWork=" << editorSceneGameplayObjectsWork
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
			<< " editorScenePropCount=" << editorScenePropCount
			<< " editorSceneSpawnCount=" << editorSceneSpawnCount
			<< " editorSceneGoalCount=" << editorSceneGoalCount
			<< " editorSceneCoinCount=" << editorSceneCoinCount
			<< " editorSceneSolidPlatformCount=" << editorSceneSolidPlatformCount
			<< " editorSceneEnemyCount=" << editorSceneEnemyCount
			<< " editorScenePhysicsBodyCount=" << editorScenePhysicsBodyCount
			<< " editorScenePhysicsRigCount=" << editorScenePhysicsRigCount
			<< " editorSceneMovingPlatformCount=" << editorSceneMovingPlatformCount
			<< " editorRuntimeObjectCount=" << editorRuntimeObjectCount
#endif
			<< " enemies=" << enemies.size()
			<< " alive=" << AliveEnemyCount()
			<< " bodies=" << PhysicsBodyCount()
			<< " constraints=" << PhysicsConstraintCount()
			<< std::endl;
	}
	return passed;
}
#endif

bool PlatformerGameModule::Initialize()
{
	SDL_SetMainReady();
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
		return false;
	}
	ownsSdl = true;

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
	ownsWindow = true;

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
	ownsRenderer = true;

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	if ((IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & IMG_INIT_PNG) != 0)
	{
		imageSystemInitialized = true;
		ownsImageSystem = true;
	}
	else
	{
		std::cerr << "IMG_Init failed for editor scene props: " << IMG_GetError() << std::endl;
	}
#endif

	if (!CreateFrameTexture())
	{
		Shutdown();
		return false;
	}

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
	diagnostics.Initialize(window, renderer, WindowWidth, WindowHeight);
#endif

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	LoadEditorSceneProps();
#endif

	ResetLevel();
	return true;
}

void PlatformerGameModule::Shutdown()
{
#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
	diagnostics.Shutdown();
#endif

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	ClearEditorSceneProps();
#endif

	if (frameTexture)
	{
		SDL_DestroyTexture(frameTexture);
		frameTexture = nullptr;
	}
	if (renderer && ownsRenderer)
	{
		SDL_DestroyRenderer(renderer);
	}
	renderer = nullptr;
	ownsRenderer = false;
	if (window && ownsWindow)
	{
		SDL_DestroyWindow(window);
	}
	window = nullptr;
	ownsWindow = false;
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	if (imageSystemInitialized && ownsImageSystem)
	{
		IMG_Quit();
	}
	imageSystemInitialized = false;
	ownsImageSystem = false;
#endif
	if (ownsSdl)
	{
		SDL_Quit();
	}
	ownsSdl = false;
	fixedStepAccumulator = 0.0f;
}

void PlatformerGameModule::ToggleFullscreen()
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

bool PlatformerGameModule::AttachRuntimeRenderer(const AE::RuntimeRenderContextSDL& context)
{
	if (!context.renderer)
	{
		return false;
	}

	if (renderer != context.renderer)
	{
		if (frameTexture)
		{
			SDL_DestroyTexture(frameTexture);
			frameTexture = nullptr;
		}
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
		for (EditorSceneProp& prop : editorSceneProps)
		{
			prop.texture = nullptr;
		}
		for (auto& texture : editorSceneTextures)
		{
			if (texture.second)
			{
				SDL_DestroyTexture(texture.second);
			}
		}
		editorSceneTextures.clear();
#endif
	}

	window = context.window;
	renderer = context.renderer;
	ownsWindow = false;
	ownsRenderer = false;
	ownsSdl = false;

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	const int imageFlags = IMG_Init(0);
	imageSystemInitialized = (imageFlags & IMG_INIT_PNG) != 0;
	ownsImageSystem = false;
	RefreshEditorSceneTextures();
#endif

	return frameTexture || CreateFrameTexture();
}

void PlatformerGameModule::BuildLevel()
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

	fillTiles(11, 12, 3, 1, '#');
	fillTiles(16, 10, 3, 1, '#');
	fillTiles(21, 8, 4, 1, '#');
	fillTiles(30, 10, 4, 1, '#');
	fillTiles(45, 9, 4, 1, '#');
	fillTiles(55, 8, 4, 1, '#');
	fillTiles(68, 9, 4, 1, '#');
	fillTiles(74, 7, 3, 1, '#');
	fillTiles(84, 8, 4, 1, '#');

	fillTiles(7, 14, 1, 1, '#');
	fillTiles(8, 13, 1, 2, '#');
	fillTiles(9, 12, 1, 3, '#');
	fillTiles(86, 14, 1, 1, '#');
	fillTiles(87, 13, 1, 2, '#');
	fillTiles(88, 12, 1, 3, '#');

	playerSpawn = AE::Math::FVector2D(64.0f, 15.0f * TileSize - 42.0f);
	finish = RectF{
		91.0f * TileSize,
		15.0f * TileSize - 96.0f,
		32.0f,
		96.0f};

	coins = {
		Coin{RectF{10.5f * TileSize, 11.8f * TileSize, 18.0f, 18.0f}},
		Coin{RectF{27.0f * TileSize, 12.0f * TileSize, 18.0f, 18.0f}},
		Coin{RectF{39.5f * TileSize, 10.8f * TileSize, 18.0f, 18.0f}},
		Coin{RectF{53.0f * TileSize, 9.8f * TileSize, 18.0f, 18.0f}},
		Coin{RectF{67.0f * TileSize, 11.8f * TileSize, 18.0f, 18.0f}},
		Coin{RectF{80.0f * TileSize, 8.8f * TileSize, 18.0f, 18.0f}}};

	LoadScriptedEnemies();
}

void PlatformerGameModule::LoadScriptedEnemies()
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
			enemy.spawnPosition = AE::Math::FVector2D(
				ReadFloat(enemyTable, "x", 19.0f * TileSize),
				ReadFloat(enemyTable, "y", 15.0f * TileSize - enemy.height));
			enemy.position = enemy.spawnPosition;
			enemy.width = ReadFloat(enemyTable, "width", enemy.width);
			enemy.height = ReadFloat(enemyTable, "height", enemy.height);
			enemy.speed = std::abs(ReadFloat(enemyTable, "speed", enemy.speed));
			enemy.direction = ReadFloat(enemyTable, "direction", enemy.direction) < 0.0f ? -1.0f : 1.0f;
			enemy.leftBound = ReadFloat(enemyTable, "left", enemy.spawnPosition.X - 96.0f);
			enemy.rightBound = ReadFloat(enemyTable, "right", enemy.spawnPosition.X + 160.0f);
			enemy.groundY = ReadFloat(enemyTable, "ground_y", enemy.spawnPosition.Y);
			enemy.maxHealth = std::max(1, ReadInt(enemyTable, "health", enemy.maxHealth));
			enemy.health = enemy.maxHealth;
			enemy.jumpCooldown = ReadFloat(enemyTable, "jump_cooldown", enemy.jumpCooldown);
			enemy.jumpVelocity = ReadFloat(enemyTable, "jump_velocity", enemy.jumpVelocity);
			enemy.alertRange = ReadFloat(enemyTable, "alert_range", enemy.alertRange);
			enemy.color = ReadColor(enemyTable, "color", enemy.color);
			enemy.canShoot = ReadBool(enemyTable, "can_shoot", enemy.canShoot);
			enemy.shootCooldown = ReadFloat(enemyTable, "shoot_cooldown", enemy.shootCooldown);
			enemy.shootRange = ReadFloat(enemyTable, "shoot_range", enemy.shootRange);
			enemy.projectileSpeed = ReadFloat(enemyTable, "projectile_speed", enemy.projectileSpeed);
			enemy.shootTimer = ReadFloat(enemyTable, "shoot_delay", 0.2f + static_cast<float>(enemies.size() % 3) * 0.25f);
			enemy.velocity = AE::Math::FVector2D(enemy.speed * enemy.direction, 0.0f);

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

void PlatformerGameModule::LoadFallbackEnemies()
{
	scriptedEnemiesLoaded = false;
	enemies = {
		Enemy{
			"fallback_patrol",
			AE::Math::FVector2D(19.0f * TileSize, 15.0f * TileSize - 26.0f),
			{},
			AE::Math::FVector2D(70.0f, 0.0f),
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
			EnemyColor},
		Enemy{
			"fallback_hopper",
			AE::Math::FVector2D(45.0f * TileSize, 15.0f * TileSize - 28.0f),
			{},
			AE::Math::FVector2D(-58.0f, 0.0f),
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
			SDL_Color{90, 132, 210, 255}},
		Enemy{
			"fallback_sentry",
			AE::Math::FVector2D(66.0f * TileSize, 13.0f * TileSize - 28.0f),
			{},
			AE::Math::FVector2D(54.0f, 0.0f),
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
			SDL_Color{151, 83, 188, 255}}};

	for (Enemy& enemy : enemies)
	{
		enemy.position = enemy.spawnPosition;
		if (enemy.name == "fallback_hopper" || enemy.name == "fallback_sentry")
		{
			enemy.canShoot = true;
			enemy.shootCooldown = enemy.name == "fallback_sentry" ? 1.05f : 1.45f;
			enemy.shootRange = enemy.name == "fallback_sentry" ? 460.0f : 340.0f;
			enemy.projectileSpeed = 360.0f;
			enemy.shootTimer = 0.25f;
		}
	}
}

void PlatformerGameModule::BuildPhysicsScene()
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
		const AE::Math::FVector2D center(
			static_cast<float>(platform.tileX * TileSize) + width * 0.5f,
			static_cast<float>(platform.tileY * TileSize) + height * 0.5f);
		AE::Physics::Body* body = AddPhysicsBox(center, width, height, 0.0f, SDL_Color{72, 124, 55, 80}, SDL_Color{55, 82, 46, 120}, 0.0f, false);
		body->collisionCategory = PhysicsCategoryTerrain;
		body->collisionMask = PhysicsCategoryDynamic | PhysicsCategorySensor;
	}

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	for (const FEditorSolidPlatform& Platform : EditorSolidPlatforms)
	{
		const RectF& Bounds = Platform.Bounds;
		const AE::Math::FVector2D Center = RectCenter(Bounds);
		AE::Physics::Body* body = AddPhysicsBox(
			Center,
			Bounds.w,
			Bounds.h,
			0.0f,
			SDL_Color{72, 124, 55, 95},
			SDL_Color{55, 82, 46, 150},
			DegreesToRadians(Platform.RotationDegrees),
			false);
		body->collisionCategory = PhysicsCategoryTerrain;
		body->collisionMask = PhysicsCategoryDynamic | PhysicsCategorySensor;
	}

	if (scenePhysicsLoaded)
	{
		BuildEditorScenePhysics();
		return;
	}
#endif

	BuildDefaultPhysicsPlayground();
}

void PlatformerGameModule::BuildDefaultPhysicsPlayground()
{
	for (int row = 0; row < 3; ++row)
	{
		for (int column = 0; column < 4 - row; ++column)
		{
			const float x = 560.0f + static_cast<float>(column) * 34.0f + static_cast<float>(row) * 17.0f;
			const float y = 450.0f - static_cast<float>(row) * 31.0f;
			AE::Physics::Body* crate = AddPhysicsBox(
				AE::Math::FVector2D(x, y),
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
			AE::Math::FVector2D(1110.0f + static_cast<float>(index % 4) * 28.0f, 250.0f - static_cast<float>(index / 4) * 32.0f),
			13.0f,
			0.85f,
			index % 2 == 0 ? PhysicsBall : SDL_Color{230, 178, 72, 255},
			PhysicsMetalEdge,
			true);
		ball->friction = 0.05f;
		ball->restitution = 0.32f;
	}

	AE::Physics::Body* bridgeLeftAnchor = AddPhysicsBox(AE::Math::FVector2D(1440.0f, 270.0f), 46.0f, 24.0f, 0.0f, PhysicsMetal, PhysicsMetalEdge, 0.0f, false);
	AE::Physics::Body* bridgeRightAnchor = AddPhysicsBox(AE::Math::FVector2D(1810.0f, 270.0f), 46.0f, 24.0f, 0.0f, PhysicsMetal, PhysicsMetalEdge, 0.0f, false);
	std::vector<AE::Physics::Body*> bridgeLinks;
	constexpr int BridgeSegments = 10;
	for (int index = 0; index < BridgeSegments; ++index)
	{
		const float t = static_cast<float>(index) / static_cast<float>(BridgeSegments - 1);
		AE::Physics::Body* link = AddPhysicsBox(
			AE::Math::FVector2D(1480.0f + t * 290.0f, 304.0f + std::sin(t * 3.14159f) * 12.0f),
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
		for (SizeT index = 1; index < bridgeLinks.size(); ++index)
		{
			AddPhysicsJoint(bridgeLinks[index - 1], bridgeLinks[index]);
		}
		AddPhysicsJoint(bridgeLinks.back(), bridgeRightAnchor);
	}

	AE::Physics::Body* chainAnchor = AddPhysicsBox(AE::Math::FVector2D(2220.0f, 150.0f), 48.0f, 22.0f, 0.0f, PhysicsMetal, PhysicsMetalEdge, 0.0f, false);
	AE::Physics::Body* previous = chainAnchor;
	for (int index = 0; index < 7; ++index)
	{
		AE::Physics::Body* link = AddPhysicsBox(
			AE::Math::FVector2D(2220.0f, 188.0f + static_cast<float>(index) * 28.0f),
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
	AE::Physics::Body* load = AddPhysicsCircle(AE::Math::FVector2D(2220.0f, 405.0f), 22.0f, 3.5f, SDL_Color{205, 74, 75, 255}, PhysicsMetalEdge, true);
	AddPhysicsJoint(previous, load);

	AE::Physics::Body* movingPlatform = AddPhysicsBox(
		AE::Math::FVector2D(2420.0f, 360.0f),
		154.0f,
		20.0f,
		0.0f,
		SDL_Color{104, 184, 116, 255},
		PhysicsMetalEdge,
		0.0f,
		true);
	kinematicBodies.push_back(KinematicBody{
		movingPlatform,
		movingPlatform->position,
		AE::Math::FVector2D(1.0f, 0.0f),
		92.0f,
		1.15f,
		0.0f});

	AE::Physics::Body* elevatorPlatform = AddPhysicsBox(
		AE::Math::FVector2D(1730.0f, 390.0f),
		132.0f,
		18.0f,
		0.0f,
		SDL_Color{104, 184, 116, 255},
		PhysicsMetalEdge,
		0.0f,
		true);
	kinematicBodies.push_back(KinematicBody{
		elevatorPlatform,
		elevatorPlatform->position,
		AE::Math::FVector2D(0.0f, 1.0f),
		72.0f,
		1.05f,
		1.4f});
}

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
void PlatformerGameModule::BuildEditorScenePhysics()
{
	for (const ScenePhysicsBodySpec& spec : editorScenePhysicsBodies)
	{
		AddScenePhysicsBody(spec);
	}

	for (const ScenePhysicsRigSpec& spec : editorScenePhysicsRigs)
	{
		if (spec.type == ScenePhysicsRigSpec::Type::Bridge)
		{
			AddScenePhysicsBridge(spec);
		}
		else if (spec.type == ScenePhysicsRigSpec::Type::Chain)
		{
			AddScenePhysicsChain(spec);
		}
	}
}

void PlatformerGameModule::AddScenePhysicsBody(const ScenePhysicsBodySpec& spec)
{
	const float width = std::max(8.0f, spec.bounds.w);
	const float height = std::max(8.0f, spec.bounds.h);
	const AE::Math::FVector2D center(spec.bounds.x + width * 0.5f, spec.bounds.y + height * 0.5f);

	if (spec.type == ScenePhysicsBodySpec::Type::Circle)
	{
		AE::Physics::Body* ball = AddPhysicsCircle(
			center,
			std::max(width, height) * 0.5f,
			std::max(0.0f, spec.Mass),
			ContainsText(spec.name, "gold") ? SDL_Color{230, 178, 72, 255} : PhysicsBall,
			PhysicsMetalEdge,
			true);
		ball->friction = std::max(0.0f, spec.Friction);
		ball->restitution = std::max(0.0f, spec.Restitution);
		return;
	}

	if (spec.type == ScenePhysicsBodySpec::Type::MovingPlatform)
	{
		AE::Physics::Body* platform = AddPhysicsBox(
			center,
			width,
			height,
			0.0f,
			SDL_Color{104, 184, 116, 255},
			PhysicsMetalEdge,
			DegreesToRadians(spec.RotationDegrees),
			true);
		kinematicBodies.push_back(KinematicBody{
			platform,
			platform->position,
			spec.verticalMotion ? AE::Math::FVector2D(0.0f, 1.0f) : AE::Math::FVector2D(1.0f, 0.0f),
			std::max(0.0f, spec.MotionAmplitude),
			std::max(0.0f, spec.MotionSpeed),
			spec.MotionPhase});
		return;
	}

	AE::Physics::Body* crate = AddPhysicsBox(
		center,
		width,
		height,
		std::max(0.0f, spec.Mass),
		PhysicsWood,
		PhysicsWoodEdge,
		DegreesToRadians(spec.RotationDegrees),
		true);
	crate->friction = std::max(0.0f, spec.Friction);
	crate->restitution = std::max(0.0f, spec.Restitution);
}

void PlatformerGameModule::AddScenePhysicsBridge(const ScenePhysicsRigSpec& spec)
{
	const float width = std::max(180.0f, spec.bounds.w);
	const float height = std::max(54.0f, spec.bounds.h);
	const float leftX = spec.bounds.x;
	const float rightX = spec.bounds.x + width;
	const float anchorY = spec.bounds.y + height * 0.22f;
	const float linkStartX = leftX + 40.0f;
	const float linkEndX = rightX - 40.0f;
	const float linkY = spec.bounds.y + height * 0.62f;

	AE::Physics::Body* leftAnchor = AddPhysicsBox(AE::Math::FVector2D(leftX, anchorY), 46.0f, 24.0f, 0.0f, PhysicsMetal, PhysicsMetalEdge, 0.0f, false);
	AE::Physics::Body* rightAnchor = AddPhysicsBox(AE::Math::FVector2D(rightX, anchorY), 46.0f, 24.0f, 0.0f, PhysicsMetal, PhysicsMetalEdge, 0.0f, false);

	std::vector<AE::Physics::Body*> links;
	const int BridgeSegments = std::max(2, spec.SegmentCount > 0 ? spec.SegmentCount : 10);
	for (int index = 0; index < BridgeSegments; ++index)
	{
		const float t = static_cast<float>(index) / static_cast<float>(BridgeSegments - 1);
		AE::Physics::Body* link = AddPhysicsBox(
			AE::Math::FVector2D(linkStartX + t * (linkEndX - linkStartX), linkY + std::sin(t * 3.14159f) * 12.0f),
			32.0f,
			14.0f,
			0.8f,
			index % 2 == 0 ? SDL_Color{92, 174, 184, 255} : SDL_Color{230, 178, 72, 255},
			PhysicsMetalEdge,
			0.0f,
			true);
		link->friction = 0.16f;
		links.push_back(link);
	}

	if (!links.empty())
	{
		AddPhysicsJoint(leftAnchor, links.front());
		for (SizeT index = 1; index < links.size(); ++index)
		{
			AddPhysicsJoint(links[index - 1], links[index]);
		}
		AddPhysicsJoint(links.back(), rightAnchor);
	}
}

void PlatformerGameModule::AddScenePhysicsChain(const ScenePhysicsRigSpec& spec)
{
	const float width = std::max(48.0f, spec.bounds.w);
	const float height = std::max(210.0f, spec.bounds.h);
	const float centerX = spec.bounds.x + width * 0.5f;
	const float topY = spec.bounds.y;
	const float bottomY = spec.bounds.y + height;

	AE::Physics::Body* anchor = AddPhysicsBox(AE::Math::FVector2D(centerX, topY), 48.0f, 22.0f, 0.0f, PhysicsMetal, PhysicsMetalEdge, 0.0f, false);
	AE::Physics::Body* previous = anchor;
	const int ChainLinks = std::max(1, spec.SegmentCount > 0 ? spec.SegmentCount : 7);
	const float spacing = std::max(22.0f, (height - 70.0f) / static_cast<float>(ChainLinks + 1));
	for (int index = 0; index < ChainLinks; ++index)
	{
		AE::Physics::Body* link = AddPhysicsBox(
			AE::Math::FVector2D(centerX, topY + 38.0f + static_cast<float>(index) * spacing),
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
	AE::Physics::Body* load = AddPhysicsCircle(AE::Math::FVector2D(centerX, bottomY), 22.0f, 3.5f, SDL_Color{205, 74, 75, 255}, PhysicsMetalEdge, true);
	AddPhysicsJoint(previous, load);
}
#endif

void PlatformerGameModule::ResetLevel()
{
	for (Coin& coin : coins)
	{
		coin.collected = false;
	}
	for (Enemy& enemy : enemies)
	{
		enemy.position = enemy.spawnPosition;
		enemy.velocity = AE::Math::FVector2D(enemy.speed * enemy.direction, 0.0f);
		enemy.health = enemy.maxHealth;
		enemy.alive = true;
		enemy.timeAlive = 0.0f;
		enemy.timeSinceJump = 0.0f;
		enemy.shootTimer = enemy.canShoot ? 0.2f : 0.0f;
	}
	projectiles.clear();
	shootCooldownTimer = 0.0f;
	pendingJumpPressed = false;
	pendingShootPressed = false;
	BuildPhysicsScene();
	ResetPlayer();
}

void PlatformerGameModule::ResetPlayer()
{
	player.position = playerSpawn;
	player.velocity = AE::Math::FVector2D::Zero;
	player.grounded = false;
	player.won = false;
	player.facing = 1;
	player.airJumpsRemaining = MaxAirJumps;
	coyoteTimer = 0.0f;
	jumpBufferTimer = 0.0f;
}

void PlatformerGameModule::PollEvents(InputState& input)
{
	input.jumpPressed = false;
	input.shootPressed = false;

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
			default:
				break;
			}
		}
	}

	ReadKeyboardInput(input);
}

void PlatformerGameModule::ReadKeyboardInput(InputState& input)
{
	input.jumpPressed = false;
	input.shootPressed = false;

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
	if (diagnostics.WantsKeyboard())
	{
		input = InputState{};
		jumpKeyWasDown = false;
		shootKeyWasDown = false;
		pauseKeyWasDown = false;
		resetKeyWasDown = false;
		pendingJumpPressed = false;
		pendingShootPressed = false;
		return;
	}
#endif

	const Uint8* keys = SDL_GetKeyboardState(nullptr);
	const bool jumpDown = keys[SDL_SCANCODE_W] != 0 ||
						  keys[SDL_SCANCODE_UP] != 0 ||
						  keys[SDL_SCANCODE_SPACE] != 0;
	const bool shootDown = keys[SDL_SCANCODE_J] != 0 ||
						   keys[SDL_SCANCODE_LCTRL] != 0 ||
						   keys[SDL_SCANCODE_RCTRL] != 0;
	const bool pauseDown = keys[SDL_SCANCODE_P] != 0;
	const bool resetDown = keys[SDL_SCANCODE_R] != 0;

	input.moveLeft = keys[SDL_SCANCODE_A] != 0 || keys[SDL_SCANCODE_LEFT] != 0;
	input.moveRight = keys[SDL_SCANCODE_D] != 0 || keys[SDL_SCANCODE_RIGHT] != 0;
	if (pauseDown && !pauseKeyWasDown)
	{
		paused = !paused;
	}
	if (resetDown && !resetKeyWasDown)
	{
		ResetLevel();
	}
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
	pauseKeyWasDown = pauseDown;
	resetKeyWasDown = resetDown;
}

void PlatformerGameModule::StepRuntime(float deltaSeconds)
{
	const Uint64 updateStart = SDL_GetPerformanceCounter();
	InputState input;
	if (SimulateOnly)
	{
		jumpKeyWasDown = false;
		shootKeyWasDown = false;
		pauseKeyWasDown = false;
		resetKeyWasDown = false;
		pendingJumpPressed = false;
		pendingShootPressed = false;
	}
	else
	{
		ReadKeyboardInput(input);
	}

	fixedStepAccumulator += ClampFloat(deltaSeconds, 0.0f, 0.05f);
	InputState stepInput = input;
	bool stepped = false;
	fixedStepsThisFrame = 0;
	while (!paused && fixedStepAccumulator >= FixedTimeStep)
	{
		Step(FixedTimeStep, stepInput);
		stepInput.jumpPressed = false;
		stepInput.shootPressed = false;
		fixedStepAccumulator -= FixedTimeStep;
		stepped = true;
		++fixedStepsThisFrame;
	}
	if (stepped)
	{
		pendingJumpPressed = false;
		pendingShootPressed = false;
	}

	lastUpdateMs = ElapsedMs(updateStart, SDL_GetPerformanceCounter());
}

void PlatformerGameModule::Step(float dt, const InputState& input)
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

void PlatformerGameModule::TryShoot()
{
	if (shootCooldownTimer > 0.0f)
	{
		return;
	}

	const float direction = player.facing >= 0 ? 1.0f : -1.0f;
	Projectile projectile;
	projectile.position = AE::Math::FVector2D(
		player.position.X + player.width * 0.5f + direction * 18.0f,
		player.position.Y + player.height * 0.42f);
	projectile.velocity = AE::Math::FVector2D(ProjectileSpeed * direction, 0.0f);
	projectile.damage = 1;
	projectile.fromPlayer = true;
	projectiles.push_back(projectile);
	shootCooldownTimer = ProjectileCooldown;
}

void PlatformerGameModule::TryEnemyShoot(Enemy& enemy)
{
	if (!enemy.canShoot || enemy.shootTimer > 0.0f || player.won)
	{
		return;
	}

	const float enemyCenterX = enemy.position.X + enemy.width * 0.5f;
	const float enemyCenterY = enemy.position.Y + enemy.height * 0.45f;
	const float playerCenterX = player.position.X + player.width * 0.5f;
	const float playerCenterY = player.position.Y + player.height * 0.45f;
	const float deltaX = playerCenterX - enemyCenterX;
	const float deltaY = playerCenterY - enemyCenterY;
	const float distanceSquared = deltaX * deltaX + deltaY * deltaY;
	if (distanceSquared < 1.0f || distanceSquared > enemy.shootRange * enemy.shootRange || std::abs(deltaY) > 180.0f)
	{
		return;
	}

	const float invDistance = 1.0f / std::sqrt(distanceSquared);
	const float directionX = deltaX * invDistance;
	const float directionY = deltaY * invDistance;

	Projectile projectile;
	projectile.width = 12.0f;
	projectile.height = 8.0f;
	projectile.timeToLive = 2.0f;
	projectile.damage = 1;
	projectile.fromPlayer = false;
	projectile.position = AE::Math::FVector2D(
		enemyCenterX + directionX * 20.0f - projectile.width * 0.5f,
		enemyCenterY + directionY * 14.0f - projectile.height * 0.5f);
	projectile.velocity = AE::Math::FVector2D(directionX * enemy.projectileSpeed, directionY * enemy.projectileSpeed);
	projectiles.push_back(projectile);
	enemy.shootTimer = enemy.shootCooldown;
}

void PlatformerGameModule::UpdatePlayer(float dt, const InputState& input)
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
		player.velocity.X = MoveTowardZero(player.velocity.X, GroundFriction * dt);
	}
	else if (input.moveLeft)
	{
		player.facing = -1;
		player.velocity.X -= MoveAcceleration * dt;
	}
	else if (input.moveRight)
	{
		player.facing = 1;
		player.velocity.X += MoveAcceleration * dt;
	}
	player.velocity.X = ClampFloat(player.velocity.X, -MaxRunSpeed, MaxRunSpeed);

	if (jumpBufferTimer > 0.0f && coyoteTimer > 0.0f)
	{
		player.velocity.Y = JumpVelocity;
		player.grounded = false;
		coyoteTimer = 0.0f;
		jumpBufferTimer = 0.0f;
	}
	else if (jumpBufferTimer > 0.0f && player.airJumpsRemaining > 0)
	{
		player.velocity.Y = JumpVelocity * 0.92f;
		player.grounded = false;
		--player.airJumpsRemaining;
		jumpBufferTimer = 0.0f;
	}
	else if (!input.jumpHeld && !input.jumpPressed && player.velocity.Y < JumpCutVelocity)
	{
		player.velocity.Y = JumpCutVelocity;
	}

	player.velocity.Y = ClampFloat(player.velocity.Y + Gravity * dt, -1000.0f, MaxFallSpeed);

	player.position.X += player.velocity.X * dt;
	ResolveTileCollisions(true);

	player.position.Y += player.velocity.Y * dt;
	player.grounded = false;
	ResolveTileCollisions(false);
	if (player.grounded)
	{
		coyoteTimer = CoyoteTime;
		player.airJumpsRemaining = MaxAirJumps;
	}

	if (player.position.Y > static_cast<float>(LevelRows * TileSize + 120))
	{
		ResetPlayer();
	}
}

void PlatformerGameModule::UpdateEnemies(float dt)
{
	for (Enemy& enemy : enemies)
	{
		if (!enemy.alive)
		{
			continue;
		}

		enemy.timeAlive += dt;
		enemy.timeSinceJump += dt;
		enemy.shootTimer = std::max(0.0f, enemy.shootTimer - dt);
		const bool enemyOnGround = std::abs(enemy.position.Y - enemy.groundY) < 0.5f && enemy.velocity.Y >= 0.0f;

		if (enemy.updateScript.valid())
		{
			sol::table enemyState = lua.create_table();
			enemyState["name"] = enemy.name;
			enemyState["x"] = enemy.position.X;
			enemyState["y"] = enemy.position.Y;
			enemyState["velocity_x"] = enemy.velocity.X;
			enemyState["velocity_y"] = enemy.velocity.Y;
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
			enemyState["can_shoot"] = enemy.canShoot;
			enemyState["shoot_timer"] = enemy.shootTimer;
			enemyState["shoot_cooldown"] = enemy.shootCooldown;
			enemyState["shoot_range"] = enemy.shootRange;
			enemyState["projectile_speed"] = enemy.projectileSpeed;
			enemyState["on_ground"] = enemyOnGround;

			sol::table playerState = lua.create_table();
			playerState["x"] = player.position.X;
			playerState["y"] = player.position.Y;
			playerState["velocity_x"] = player.velocity.X;
			playerState["velocity_y"] = player.velocity.Y;

			sol::protected_function update = enemy.updateScript;
			sol::protected_function_result result = update(enemyState, playerState, dt);
			if (result.valid())
			{
				enemy.velocity.X = ReadFloat(enemyState, "velocity_x", enemy.velocity.X);
				enemy.velocity.Y = ReadFloat(enemyState, "velocity_y", enemy.velocity.Y);
				enemy.direction = ReadFloat(enemyState, "direction", enemy.direction) < 0.0f ? -1.0f : 1.0f;
				enemy.timeSinceJump = ReadFloat(enemyState, "time_since_jump", enemy.timeSinceJump);
				enemy.canShoot = ReadBool(enemyState, "can_shoot", enemy.canShoot);
				enemy.shootTimer = ReadFloat(enemyState, "shoot_timer", enemy.shootTimer);
				enemy.shootCooldown = ReadFloat(enemyState, "shoot_cooldown", enemy.shootCooldown);
				enemy.shootRange = ReadFloat(enemyState, "shoot_range", enemy.shootRange);
				enemy.projectileSpeed = ReadFloat(enemyState, "projectile_speed", enemy.projectileSpeed);
			}
			else
			{
				enemy.velocity.X = enemy.speed * enemy.direction;
			}
		}
		else
		{
			enemy.velocity.X = enemy.speed * enemy.direction;
		}

		enemy.velocity.Y = ClampFloat(enemy.velocity.Y + EnemyGravity * dt, -620.0f, MaxFallSpeed);
		enemy.position.X += enemy.velocity.X * dt;
		enemy.position.Y += enemy.velocity.Y * dt;

		if (enemy.position.X < enemy.leftBound)
		{
			enemy.position.X = enemy.leftBound;
			enemy.direction = 1.0f;
			enemy.velocity.X = enemy.speed;
		}
		else if (enemy.position.X + enemy.width > enemy.rightBound)
		{
			enemy.position.X = enemy.rightBound - enemy.width;
			enemy.direction = -1.0f;
			enemy.velocity.X = -enemy.speed;
		}

		if (enemy.position.Y > enemy.groundY)
		{
			enemy.position.Y = enemy.groundY;
			enemy.velocity.Y = 0.0f;
		}

		TryEnemyShoot(enemy);
	}
}

void PlatformerGameModule::UpdateProjectiles(float dt)
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

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
		bool hitEditorSolidPlatform = false;
		for (const FEditorSolidPlatform& Platform : EditorSolidPlatforms)
		{
			if (IntersectsEditorSolidPlatform(projectileBounds, Platform))
			{
				hitEditorSolidPlatform = true;
				break;
			}
		}
		if (hitEditorSolidPlatform)
		{
			projectile.active = false;
			continue;
		}
#endif

		if (projectile.fromPlayer)
		{
			for (Enemy& enemy : enemies)
			{
				if (!enemy.alive || !Intersects(projectileBounds, EnemyRect(enemy)))
				{
					continue;
				}

				enemy.health -= projectile.damage;
				enemy.velocity.X += projectile.velocity.X * 0.12f;
				enemy.velocity.Y = std::min(enemy.velocity.Y, -120.0f);
				if (enemy.health <= 0)
				{
					enemy.alive = false;
				}
				projectile.active = false;
				break;
			}
		}
		else if (Intersects(projectileBounds, PlayerRect()))
		{
			projectile.active = false;
			ResetPlayer();
			continue;
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

void PlatformerGameModule::ApplyProjectilePhysicsHit(Projectile& projectile)
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

		const float direction = projectile.velocity.X >= 0.0f ? 1.0f : -1.0f;
		body->ApplyImpulseLinear(AE::Math::FVector2D(180.0f * direction, -36.0f));
		body->angularVelocity += 2.5f * direction;
		physicsWorld->WakeBody(*body);
		projectile.active = false;
		return;
	}
}

void PlatformerGameModule::StepPhysics(float dt)
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

	ResolvePlayerPhysicsContacts(dt);
}

void PlatformerGameModule::UpdateKinematicPhysicsBodies(float dt)
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
		const AE::Math::FVector2D previousPosition = kinematic.body->position;
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

void PlatformerGameModule::ResolvePlayerPhysicsContacts(float dt)
{
	if (!physicsWorld)
	{
		return;
	}

	RectF playerBounds = PlayerRect();
	for (BodyVisual& visual : physicsVisuals)
	{
		AE::Physics::Body* body = visual.body;
		const bool isKinematicBody = IsKinematicBody(body);
		if (!body || !visual.gameplayBody || (body->IsStatic() && !isKinematicBody))
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
		const bool landingOnBody = player.velocity.Y >= body->velocity.Y - 12.0f &&
								   playerBounds.y < bodyTop &&
								   overlapFromTop >= 0.0f &&
								   overlapFromTop < 24.0f;

		if (landingOnBody)
		{
			player.position.Y = bodyTop - player.height;
			player.position.X += body->velocity.X * dt;
			player.velocity.Y = std::min(0.0f, body->velocity.Y);
			player.grounded = true;
			coyoteTimer = CoyoteTime;
			player.airJumpsRemaining = MaxAirJumps;
			if (!body->IsStatic())
			{
				body->ApplyImpulseLinear(AE::Math::FVector2D(player.velocity.X * 0.08f, 0.0f));
			}
		}
		else
		{
			const float playerCenterX = playerBounds.x + playerBounds.w * 0.5f;
			const float bodyCenterX = bodyBounds.x + bodyBounds.w * 0.5f;
			const float pushDirection = playerCenterX < bodyCenterX ? 1.0f : -1.0f;
			const float overlapLeft = (playerBounds.x + playerBounds.w) - bodyBounds.x;
			const float overlapRight = (bodyBounds.x + bodyBounds.w) - playerBounds.x;
			const float horizontalOverlap = std::min(overlapLeft, overlapRight);
			player.position.X -= pushDirection * std::min(horizontalOverlap + 0.5f, 12.0f);
			player.velocity.X = MoveTowardZero(player.velocity.X, 180.0f);
			if (!body->IsStatic())
			{
				body->ApplyImpulseLinear(AE::Math::FVector2D(pushDirection * 95.0f, -18.0f));
				body->angularVelocity += pushDirection * 1.8f;
			}
		}

		if (!body->IsStatic())
		{
			physicsWorld->WakeBody(*body);
		}
		playerBounds = PlayerRect();
	}
}

void PlatformerGameModule::UpdateCoins()
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

void PlatformerGameModule::UpdateHazards()
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

void PlatformerGameModule::UpdateGoal()
{
	if (Intersects(PlayerRect(), finish))
	{
		player.won = true;
		player.velocity = AE::Math::FVector2D::Zero;
	}
}

void PlatformerGameModule::ResolveTileCollisions(bool horizontal)
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
				static_cast<float>(TileSize)};

			if (!Intersects(playerBounds, tileBounds))
			{
				continue;
			}

			if (horizontal)
			{
				if (player.velocity.X > 0.0f)
				{
					player.position.X = tileBounds.x - player.width;
				}
				else if (player.velocity.X < 0.0f)
				{
					player.position.X = tileBounds.x + tileBounds.w;
				}
				player.velocity.X = 0.0f;
			}
			else
			{
				if (player.velocity.Y > 0.0f)
				{
					player.position.Y = tileBounds.y - player.height;
					player.grounded = true;
				}
				else if (player.velocity.Y < 0.0f)
				{
					player.position.Y = tileBounds.y + tileBounds.h;
				}
				player.velocity.Y = 0.0f;
			}

			playerBounds = PlayerRect();
		}
	}

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	for (const FEditorSolidPlatform& Platform : EditorSolidPlatforms)
	{
		if (!ResolveEditorSolidPlatformCollision(Platform, horizontal))
		{
			continue;
		}

		playerBounds = PlayerRect();
	}
#endif
}

void PlatformerGameModule::UpdateCamera()
{
	const float targetCameraX = player.position.X + player.width * 0.5f - static_cast<float>(WindowWidth) * 0.45f;
	const float maxCameraX = static_cast<float>(LevelCols * TileSize - WindowWidth);
	cameraX = ClampFloat(targetCameraX, 0.0f, std::max(0.0f, maxCameraX));

	const float targetCameraY =
		player.position.Y +
		player.height * 0.5f -
		static_cast<float>(WindowHeight) * 0.5f;

	const float maxCameraY =
		static_cast<float>(LevelRows * TileSize - WindowHeight);

	cameraY = ClampFloat(targetCameraY, 0.0f, std::max(0.0f, maxCameraY));
}

bool PlatformerGameModule::IsSolidTile(int tileX, int tileY) const
{
	if (tileX < 0 || tileX >= LevelCols || tileY < 0 || tileY >= LevelRows)
	{
		return false;
	}
	return levelTiles[tileY][tileX] == '#';
}

PlatformerGameModule::RectF PlatformerGameModule::PlayerRect() const
{
	return RectF{player.position.X, player.position.Y, player.width, player.height};
}

PlatformerGameModule::RectF PlatformerGameModule::EnemyRect(const Enemy& enemy) const
{
	return RectF{enemy.position.X, enemy.position.Y, enemy.width, enemy.height};
}

PlatformerGameModule::RectF PlatformerGameModule::ProjectileRect(const Projectile& projectile) const
{
	return RectF{projectile.position.X, projectile.position.Y, projectile.width, projectile.height};
}

PlatformerGameModule::RectF PlatformerGameModule::BodyBounds(const AE::Physics::Body& body) const
{
	if (!body.shape)
	{
		return RectF{body.position.X, body.position.Y, 0.0f, 0.0f};
	}

	if (body.shape->GetType() == AE::Physics::CIRCLE)
	{
		const AE::Physics::CircleShape* circle = static_cast<const AE::Physics::CircleShape*>(body.shape);
		return RectF{
			body.position.X - circle->radius,
			body.position.Y - circle->radius,
			circle->radius * 2.0f,
			circle->radius * 2.0f};
	}

	const AE::Physics::PolygonShape* polygon = static_cast<const AE::Physics::PolygonShape*>(body.shape);
	float minX = std::numeric_limits<float>::max();
	float minY = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::lowest();
	float maxY = std::numeric_limits<float>::lowest();
	for (const AE::Math::FVector2D& vertex : polygon->worldVertices)
	{
		minX = std::min(minX, vertex.X);
		minY = std::min(minY, vertex.Y);
		maxX = std::max(maxX, vertex.X);
		maxY = std::max(maxY, vertex.Y);
	}

	return RectF{minX, minY, maxX - minX, maxY - minY};
}

bool PlatformerGameModule::IsKinematicBody(const AE::Physics::Body* body) const
{
	for (const KinematicBody& kinematic : kinematicBodies)
	{
		if (kinematic.body == body)
		{
			return true;
		}
	}
	return false;
}

int PlatformerGameModule::AliveEnemyCount() const
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

int PlatformerGameModule::PhysicsBodyCount() const
{
	return physicsWorld ? static_cast<int>(physicsWorld->GetBodies().size()) : 0;
}

int PlatformerGameModule::PhysicsConstraintCount() const
{
	return physicsWorld ? static_cast<int>(physicsWorld->GetConstraints().size()) : 0;
}

bool PlatformerGameModule::Intersects(const RectF& first, const RectF& second)
{
	return first.x < second.x + second.w &&
		   first.x + first.w > second.x &&
		   first.y < second.y + second.h &&
		   first.y + first.h > second.y;
}

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
float PlatformerGameModule::DegreesToRadians(float Degrees)
{
	return Degrees * Pi / 180.0f;
}

AE::Math::FVector2D PlatformerGameModule::RectCenter(const RectF& Bounds)
{
	return AE::Math::FVector2D(Bounds.x + Bounds.w * 0.5f, Bounds.y + Bounds.h * 0.5f);
}

std::array<AE::Math::FVector2D, 4> PlatformerGameModule::RectVertices(const RectF& Bounds)
{
	return {
		AE::Math::FVector2D(Bounds.x, Bounds.y),
		AE::Math::FVector2D(Bounds.x + Bounds.w, Bounds.y),
		AE::Math::FVector2D(Bounds.x + Bounds.w, Bounds.y + Bounds.h),
		AE::Math::FVector2D(Bounds.x, Bounds.y + Bounds.h)};
}

std::array<AE::Math::FVector2D, 4> PlatformerGameModule::RotatedRectVertices(const RectF& Bounds, float RotationDegrees)
{
	const AE::Math::FVector2D Center = RectCenter(Bounds);
	const float HalfWidth = Bounds.w * 0.5f;
	const float HalfHeight = Bounds.h * 0.5f;
	const float RotationRadians = DegreesToRadians(RotationDegrees);
	const float CosValue = std::cos(RotationRadians);
	const float SinValue = std::sin(RotationRadians);
	const std::array<AE::Math::FVector2D, 4> LocalVertices{
		AE::Math::FVector2D(-HalfWidth, -HalfHeight),
		AE::Math::FVector2D(HalfWidth, -HalfHeight),
		AE::Math::FVector2D(HalfWidth, HalfHeight),
		AE::Math::FVector2D(-HalfWidth, HalfHeight)};
	std::array<AE::Math::FVector2D, 4> Vertices{};

	for (SizeT Index = 0; Index < Vertices.size(); ++Index)
	{
		const AE::Math::FVector2D& Local = LocalVertices[Index];
		Vertices[Index] = AE::Math::FVector2D(
			Center.X + Local.X * CosValue - Local.Y * SinValue,
			Center.Y + Local.X * SinValue + Local.Y * CosValue);
	}

	return Vertices;
}

PlatformerGameModule::FCollisionProjection PlatformerGameModule::ProjectVertices(
	const std::array<AE::Math::FVector2D, 4>& Vertices,
	const AE::Math::FVector2D& Axis)
{
	const float FirstProjection = Vertices[0].DotProduct(Axis);
	FCollisionProjection Projection{FirstProjection, FirstProjection};
	for (SizeT Index = 1; Index < Vertices.size(); ++Index)
	{
		const float Value = Vertices[Index].DotProduct(Axis);
		Projection.Min = std::min(Projection.Min, Value);
		Projection.Max = std::max(Projection.Max, Value);
	}
	return Projection;
}

PlatformerGameModule::FCollisionResult PlatformerGameModule::IntersectAabbWithRotatedRect(
	const RectF& Bounds,
	const FEditorSolidPlatform& Platform)
{
	FCollisionResult Result;
	const std::array<AE::Math::FVector2D, 4> AabbVertices = RectVertices(Bounds);
	const std::array<AE::Math::FVector2D, 4> PlatformVertices = RotatedRectVertices(Platform.Bounds, Platform.RotationDegrees);
	const float RotationRadians = DegreesToRadians(Platform.RotationDegrees);
	const float CosValue = std::cos(RotationRadians);
	const float SinValue = std::sin(RotationRadians);
	const std::array<AE::Math::FVector2D, 4> Axes{
		AE::Math::FVector2D(1.0f, 0.0f),
		AE::Math::FVector2D(0.0f, 1.0f),
		AE::Math::FVector2D(CosValue, SinValue),
		AE::Math::FVector2D(-SinValue, CosValue)};

	float SmallestOverlap = std::numeric_limits<float>::max();
	AE::Math::FVector2D BestAxis = AE::Math::FVector2D::UnitY;
	const AE::Math::FVector2D CenterDelta = RectCenter(Bounds) - RectCenter(Platform.Bounds);

	for (const AE::Math::FVector2D& Axis : Axes)
	{
		const FCollisionProjection AabbProjection = ProjectVertices(AabbVertices, Axis);
		const FCollisionProjection PlatformProjection = ProjectVertices(PlatformVertices, Axis);
		const float Overlap = std::min(AabbProjection.Max, PlatformProjection.Max) - std::max(AabbProjection.Min, PlatformProjection.Min);
		if (Overlap <= 0.0f)
		{
			return Result;
		}

		if (Overlap < SmallestOverlap)
		{
			SmallestOverlap = Overlap;
			BestAxis = CenterDelta.DotProduct(Axis) < 0.0f ? -Axis : Axis;
		}
	}

	Result.Intersects = true;
	Result.MinimumTranslation = BestAxis * SmallestOverlap;
	return Result;
}

bool PlatformerGameModule::IntersectsEditorSolidPlatform(const RectF& Bounds, const FEditorSolidPlatform& Platform)
{
	return IntersectAabbWithRotatedRect(Bounds, Platform).Intersects;
}

bool PlatformerGameModule::ResolveEditorSolidPlatformCollision(const FEditorSolidPlatform& Platform, bool Horizontal)
{
	const FCollisionResult Collision = IntersectAabbWithRotatedRect(PlayerRect(), Platform);
	if (!Collision.Intersects)
	{
		return false;
	}

	const float AbsX = std::abs(Collision.MinimumTranslation.X);
	const float AbsY = std::abs(Collision.MinimumTranslation.Y);
	constexpr float CollisionEpsilon = 0.001f;
	if (Horizontal && AbsX <= AbsY + CollisionEpsilon)
	{
		return false;
	}
	if (!Horizontal && AbsY + CollisionEpsilon < AbsX)
	{
		return false;
	}

	player.position += Collision.MinimumTranslation;
	if (Horizontal)
	{
		if (AbsX > CollisionEpsilon)
		{
			player.velocity.X = 0.0f;
		}
		return true;
	}

	if (Collision.MinimumTranslation.Y < -CollisionEpsilon)
	{
		player.grounded = true;
		coyoteTimer = CoyoteTime;
		player.airJumpsRemaining = MaxAirJumps;
		if (player.velocity.Y > 0.0f)
		{
			player.velocity.Y = 0.0f;
		}
	}
	else if (Collision.MinimumTranslation.Y > CollisionEpsilon && player.velocity.Y < 0.0f)
	{
		player.velocity.Y = 0.0f;
	}
	else if (AbsY > CollisionEpsilon)
	{
		player.velocity.Y = 0.0f;
	}
	return true;
}
#endif

int PlatformerGameModule::ClampInt(int value, int minValue, int maxValue)
{
	return std::max(minValue, std::min(maxValue, value));
}

float PlatformerGameModule::ClampFloat(float value, float minValue, float maxValue)
{
	return std::max(minValue, std::min(maxValue, value));
}

float PlatformerGameModule::MoveTowardZero(float value, float amount)
{
	if (std::abs(value) <= amount)
	{
		return 0.0f;
	}
	return value > 0.0f ? value - amount : value + amount;
}

AE::Physics::Body* PlatformerGameModule::AddPhysicsBox(
	AE::Math::FVector2D position,
	float width,
	float height,
	float mass,
	SDL_Color fill,
	SDL_Color edge,
	float rotation,
	bool gameplayBody)
{
	AE::Physics::BoxShape shape(width, height);
	AE::Physics::Body* body = new AE::Physics::Body(shape, position.X, position.Y, mass);
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

AE::Physics::Body* PlatformerGameModule::AddPhysicsCircle(
	AE::Math::FVector2D position,
	float radius,
	float mass,
	SDL_Color fill,
	SDL_Color edge,
	bool gameplayBody)
{
	AE::Physics::CircleShape shape(radius);
	AE::Physics::Body* body = new AE::Physics::Body(shape, position.X, position.Y, mass);
	body->collisionCategory = mass == 0.0f ? PhysicsCategoryTerrain : PhysicsCategoryDynamic;
	body->collisionMask = PhysicsCategoryTerrain | PhysicsCategoryDynamic | PhysicsCategorySensor;
	if (physicsWorld)
	{
		physicsWorld->AddBody(body);
	}
	physicsVisuals.push_back(BodyVisual{body, fill, edge, gameplayBody});
	return body;
}

void PlatformerGameModule::AddPhysicsJoint(AE::Physics::Body* first, AE::Physics::Body* second)
{
	if (!physicsWorld || !first || !second)
	{
		return;
	}

	physicsWorld->AddConstraint(new AE::Physics::JointConstraint(first, second, (first->position + second->position) * 0.5f));
}

void PlatformerGameModule::Render()
{
	if (!renderer || !frameTexture)
	{
		return;
	}

	RenderFrameContents();
	PresentFrameTexture();
}

void PlatformerGameModule::RenderFrameContents()
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
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	DrawEditorSceneProps();
#endif
	DrawPhysics();
	DrawCoins();
	DrawFinish();
	DrawProjectiles();
	DrawEnemies();
	DrawPlayer();
	DrawHud();
	RenderDiagnostics();

	lastRenderMs = ElapsedMs(renderStart, SDL_GetPerformanceCounter());
}

bool PlatformerGameModule::CreateFrameTexture()
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

void PlatformerGameModule::BeginFrameTexture()
{
	SDL_SetRenderTarget(renderer, frameTexture);
	SDL_RenderSetLogicalSize(renderer, 0, 0);
	SDL_RenderSetViewport(renderer, nullptr);
	SDL_RenderSetScale(renderer, 1.0f, 1.0f);
}

void PlatformerGameModule::PresentFrameTexture()
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

SDL_Rect PlatformerGameModule::CalculateFrameViewport() const
{
	int outputWidth = WindowWidth;
	int outputHeight = WindowHeight;
	SDL_GetRendererOutputSize(renderer, &outputWidth, &outputHeight);

	return CalculateFrameViewport(SDL_Rect{0, 0, outputWidth, outputHeight});
}

SDL_Rect PlatformerGameModule::CalculateFrameViewport(const SDL_Rect& outputBounds) const
{
	const float scale = std::min(
		static_cast<float>(outputBounds.w) / static_cast<float>(WindowWidth),
		static_cast<float>(outputBounds.h) / static_cast<float>(WindowHeight));
	const int width = static_cast<int>(std::round(static_cast<float>(WindowWidth) * scale));
	const int height = static_cast<int>(std::round(static_cast<float>(WindowHeight) * scale));
	return SDL_Rect{
		outputBounds.x + (outputBounds.w - width) / 2,
		outputBounds.y + (outputBounds.h - height) / 2,
		width,
		height};
}

void PlatformerGameModule::CopyFrameTextureToTarget(const SDL_Rect& outputBounds)
{
	if (!renderer || !frameTexture || outputBounds.w <= 0 || outputBounds.h <= 0)
	{
		return;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderFillRect(renderer, &outputBounds);

	const SDL_Rect destination = CalculateFrameViewport(outputBounds);
	SDL_RenderCopy(renderer, frameTexture, nullptr, &destination);
}

void PlatformerGameModule::RenderDiagnostics()
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
		"Player x " + std::to_string(static_cast<int>(player.position.X)) +
		" y " + std::to_string(static_cast<int>(player.position.Y)) +
		" | coins " + std::to_string(collectedCoins) + "/" + std::to_string(coins.size()) +
		" | enemies " + std::to_string(AliveEnemyCount()) + "/" + std::to_string(enemies.size()) +
		" | bodies " + std::to_string(PhysicsBodyCount())
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
		+ " | editor props " + std::to_string(editorSceneProps.size()) +
		" | editor solids " + std::to_string(EditorSolidPlatforms.size())
#endif
		;

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
        int playerProjectileCount = 0;
        int enemyProjectileCount = 0;
        for (const Projectile& projectile : projectiles)
        {
            if (projectile.fromPlayer)
            {
                ++playerProjectileCount;
            }
            else
            {
                ++enemyProjectileCount;
            }
        }
        ImGui::Text("Coins: %d / %zu", collectedCoins, coins.size());
        ImGui::Text("Enemies: %d / %zu", AliveEnemyCount(), enemies.size());
        ImGui::Text("Projectiles: %d player / %d enemy", playerProjectileCount, enemyProjectileCount);
        ImGui::Text("Scripted enemies: %s", scriptedEnemiesLoaded ? "yes" : "no");
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
        ImGui::Text("Editor scene props: %zu", editorSceneProps.size());
        ImGui::Text("Editor solid platforms: %zu", EditorSolidPlatforms.size());
#endif
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
        ImGui::Text("Camera X: %.1f", cameraX); });
	diagnostics.Render();
#endif
}

void PlatformerGameModule::DrawBackground() const
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

void PlatformerGameModule::DrawLevel() const
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
				static_cast<float>(TileSize)};
			DrawRect(tile, y >= 15 ? Dirt : Brick);

			const int screenX = static_cast<int>(tile.x - cameraX);
			const int screenY = static_cast<int>(tile.y);
			DrawScreenRect(screenX, screenY, TileSize, 4, Ground);
			DrawScreenRect(screenX, screenY, 2, TileSize, SDL_Color{205, 137, 96, 255});
		}
	}

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
	for (const FEditorSolidPlatform& Platform : EditorSolidPlatforms)
	{
		DrawRotatedRect(Platform.Bounds, Platform.RotationDegrees, Brick);
	}
#endif
}

void PlatformerGameModule::DrawCoins() const
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

void PlatformerGameModule::DrawEnemies() const
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

void PlatformerGameModule::DrawProjectiles() const
{
	for (const Projectile& projectile : projectiles)
	{
		DrawRect(ProjectileRect(projectile), projectile.fromPlayer ? ProjectileColor : EnemyProjectileColor);
		DrawRect(
			RectF{projectile.position.X - (projectile.velocity.X > 0.0f ? 7.0f : -7.0f), projectile.position.Y + 2.0f, 8.0f, 2.0f},
			projectile.fromPlayer ? SDL_Color{255, 255, 245, 170} : SDL_Color{255, 190, 202, 170});
	}
}

void PlatformerGameModule::DrawPhysics() const
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
				RoundToInt(visual.body->position.X - cameraX),
				RoundToInt(visual.body->position.Y),
				RoundToInt(circle->radius),
				visual.fill);
			DrawFilledCircle(
				RoundToInt(visual.body->position.X - cameraX),
				RoundToInt(visual.body->position.Y),
				2,
				visual.edge);
		}
		else
		{
			const AE::Physics::PolygonShape* polygon = static_cast<const AE::Physics::PolygonShape*>(visual.body->shape);
			std::vector<AE::Math::FVector2D> screenVertices;
			screenVertices.reserve(polygon->worldVertices.size());
			for (const AE::Math::FVector2D& vertex : polygon->worldVertices)
			{
				screenVertices.emplace_back(vertex.X - cameraX, vertex.Y);
			}
			DrawFilledPolygon(screenVertices, visual.fill);
			DrawPolyline(screenVertices, visual.edge, true);
		}
	}

	for (const AE::Physics::Contact& contact : physicsWorld->GetContacts())
	{
		DrawWorldLine(contact.start, contact.end, SDL_Color{255, 245, 145, 160});
		DrawFilledCircle(RoundToInt(contact.start.X - cameraX), RoundToInt(contact.start.Y), 2, SDL_Color{255, 245, 145, 190});
	}
}

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
PlatformerGameModule::RectF PlatformerGameModule::EditorSceneObjectBounds(const AE::Scene::ObjectData& objectData) const
{
	const float width = std::max(1.0f, std::abs(objectData.size.x * objectData.transform.scale.x));
	const float height = std::max(1.0f, std::abs(objectData.size.y * objectData.transform.scale.y));
	return RectF{
		objectData.transform.position.x - width * 0.5f,
		objectData.transform.position.y - height * 0.5f,
		width,
		height};
}

bool PlatformerGameModule::BuildEditorSceneEnemy(const AE::Scene::ObjectData& objectData, const RectF& bounds, Enemy& enemy) const
{
	if (!objectData.visible)
	{
		return false;
	}

	enemy.name = objectData.name.empty() ? std::string("scene_enemy") : objectData.name;
	enemy.width = std::max(18.0f, bounds.w);
	enemy.height = std::max(18.0f, bounds.h);
	enemy.spawnPosition = AE::Math::FVector2D(bounds.x, bounds.y);
	enemy.position = enemy.spawnPosition;

	const bool hopper = ContainsText(enemy.name, "hopper");
	const bool sentry = ContainsText(enemy.name, "sentry");
	const bool shooter = sentry || ContainsText(enemy.name, "shooter") || ContainsText(enemy.name, "turret");
	enemy.speed = sentry ? 54.0f : (hopper ? 58.0f : 70.0f);
	enemy.direction = ContainsText(enemy.name, "left") ? -1.0f : 1.0f;
	enemy.velocity = AE::Math::FVector2D(enemy.speed * enemy.direction, 0.0f);

	const float patrolWidth = sentry ? 260.0f : 192.0f;
	enemy.leftBound = bounds.x - patrolWidth * 0.45f;
	enemy.rightBound = bounds.x + enemy.width + patrolWidth;
	enemy.groundY = bounds.y;
	enemy.maxHealth = sentry ? 4 : (hopper ? 3 : 2);
	enemy.health = enemy.maxHealth;
	enemy.jumpCooldown = hopper ? 1.1f : 1.2f;
	enemy.jumpVelocity = hopper ? -360.0f : -340.0f;
	enemy.alertRange = shooter ? 460.0f : 260.0f;
	enemy.color = sentry ? SDL_Color{151, 83, 188, 255} : (hopper ? SDL_Color{90, 132, 210, 255} : EnemyColor);
	enemy.canShoot = shooter;
	enemy.shootCooldown = sentry ? 1.05f : 1.45f;
	enemy.shootRange = sentry ? 460.0f : 340.0f;
	enemy.projectileSpeed = 360.0f;
	enemy.shootTimer = shooter ? 0.25f : 0.0f;
	return true;
}

void PlatformerGameModule::LoadEditorSceneProps()
{
	ClearEditorSceneProps();

	projectContentRoot.clear();
	if (!editorScenePathOverride.empty())
	{
		std::error_code CanonicalError;
		std::filesystem::path ScenePath = std::filesystem::weakly_canonical(editorScenePathOverride, CanonicalError);
		if (ScenePath.empty())
		{
			ScenePath = editorScenePathOverride;
		}

		const std::filesystem::path SceneDirectory = ScenePath.parent_path();
		if (SceneDirectory.filename() == "Scenes" && SceneDirectory.has_parent_path())
		{
			projectContentRoot = SceneDirectory.parent_path();
		}
	}
	if (projectContentRoot.empty())
	{
		projectContentRoot = FindProjectContentRoot();
	}
	if (projectContentRoot.empty())
	{
		return;
	}

	const std::filesystem::path ScenePath = editorScenePathOverride.empty() ? projectContentRoot / "Scenes" / "PlatformerTest.amber.scene" : editorScenePathOverride;
	std::error_code ErrorCode;
	if (!std::filesystem::exists(ScenePath, ErrorCode))
	{
		return;
	}

	AE::Scene::Document SceneDocument;
	std::string Error;
	if (!AE::Scene::LoadScene(ScenePath, SceneDocument, &Error))
	{
		std::cerr << "Platformer editor scene load failed: " << Error << std::endl;
		return;
	}

	LoadEditorSceneProps(SceneDocument, ScenePath.string());
}

void PlatformerGameModule::LoadEditorSceneProps(const AE::Scene::Document& SceneDocument, const std::string& SceneLabel)
{
	ClearEditorSceneProps();

	AE::Scene::ObjectFactory Factory;
	PlatformerScene::RegisterPlatformerSceneObjects(Factory);
	editorSceneRegistry = std::make_unique<Registry>();
	editorSceneObjects = Factory.CreateObjects(SceneDocument, editorSceneRegistry.get());
	editorSceneRegistry->Update();

	std::vector<Coin> SceneCoins;
	std::vector<FEditorSolidPlatform> SceneSolidPlatforms;
	std::vector<Enemy> SceneEnemies;
	std::vector<ScenePhysicsBodySpec> ScenePhysicsBodies;
	std::vector<ScenePhysicsRigSpec> ScenePhysicsRigs;
	SizeT GameplayObjectCount = 0;

	for (const std::unique_ptr<AE::Scene::Object>& Object : editorSceneObjects)
	{
		if (!Object)
		{
			continue;
		}

		const AE::Scene::ObjectData& ObjectData = Object->GetData();
		const RectF Bounds = EditorSceneObjectBounds(ObjectData);
		if (ObjectData.className == "PlayerSpawnObject")
		{
			playerSpawn = AE::Math::FVector2D(Bounds.x, Bounds.y);
			++GameplayObjectCount;
			continue;
		}
		if (ObjectData.className == "GoalObject")
		{
			finish = Bounds;
			++GameplayObjectCount;
			continue;
		}
		if (ObjectData.className == "CoinObject")
		{
			SceneCoins.push_back(Coin{Bounds, false});
			++GameplayObjectCount;
			continue;
		}
		if (ObjectData.className == "SolidPlatformObject")
		{
			SceneSolidPlatforms.push_back(FEditorSolidPlatform{Bounds, ObjectData.transform.rotationDegrees});
			++GameplayObjectCount;
			continue;
		}
		if (ObjectData.className == "EnemySpawnObject")
		{
			Enemy Enemy;
			if (BuildEditorSceneEnemy(ObjectData, Bounds, Enemy))
			{
				if (Object->HasComponent<PlatformerScene::FEnemySpawnComponent>())
				{
					const PlatformerScene::FEnemySpawnComponent& Component = Object->GetComponent<PlatformerScene::FEnemySpawnComponent>();
					Enemy.speed = std::max(0.0f, Component.Speed);
					Enemy.direction = Component.Direction < 0.0f ? -1.0f : 1.0f;
					Enemy.velocity = AE::Math::FVector2D(Enemy.speed * Enemy.direction, 0.0f);
					Enemy.leftBound = Bounds.x - std::max(0.0f, Component.PatrolWidth) * 0.45f;
					Enemy.rightBound = Bounds.x + Enemy.width + std::max(0.0f, Component.PatrolWidth);
					Enemy.maxHealth = std::max(1, Component.MaxHealth);
					Enemy.health = Enemy.maxHealth;
					Enemy.jumpCooldown = std::max(0.0f, Component.JumpCooldown);
					Enemy.jumpVelocity = Component.JumpVelocity;
					Enemy.alertRange = std::max(0.0f, Component.AlertRange);
					Enemy.canShoot = Component.CanShoot;
					Enemy.shootCooldown = std::max(0.0f, Component.ShootCooldown);
					Enemy.shootRange = std::max(0.0f, Component.ShootRange);
					Enemy.projectileSpeed = std::max(0.0f, Component.ProjectileSpeed);
					Enemy.shootTimer = Enemy.canShoot ? 0.25f : 0.0f;
				}
				SceneEnemies.push_back(Enemy);
			}
			++GameplayObjectCount;
			continue;
		}
		if (ObjectData.className == "PhysicsBoxObject")
		{
			ScenePhysicsBodySpec Spec{
				ScenePhysicsBodySpec::Type::Box,
				ObjectData.name,
				Bounds,
				false};
			Spec.RotationDegrees = ObjectData.transform.rotationDegrees;
			if (Object->HasComponent<PlatformerScene::FPhysicsBoxComponent>())
			{
				const PlatformerScene::FPhysicsBoxComponent& Component = Object->GetComponent<PlatformerScene::FPhysicsBoxComponent>();
				Spec.Mass = Component.Mass;
				Spec.Friction = Component.Friction;
				Spec.Restitution = Component.Restitution;
			}
			ScenePhysicsBodies.push_back(Spec);
			++GameplayObjectCount;
			continue;
		}
		if (ObjectData.className == "PhysicsCircleObject")
		{
			ScenePhysicsBodySpec Spec{
				ScenePhysicsBodySpec::Type::Circle,
				ObjectData.name,
				Bounds,
				false};
			Spec.RotationDegrees = ObjectData.transform.rotationDegrees;
			if (Object->HasComponent<PlatformerScene::FPhysicsCircleComponent>())
			{
				const PlatformerScene::FPhysicsCircleComponent& Component = Object->GetComponent<PlatformerScene::FPhysicsCircleComponent>();
				Spec.Mass = Component.Mass;
				Spec.Friction = Component.Friction;
				Spec.Restitution = Component.Restitution;
			}
			ScenePhysicsBodies.push_back(Spec);
			++GameplayObjectCount;
			continue;
		}
		if (ObjectData.className == "MovingPlatformObject")
		{
			ScenePhysicsBodySpec Spec{
				ScenePhysicsBodySpec::Type::MovingPlatform,
				ObjectData.name,
				Bounds,
				ContainsText(ObjectData.name, "elevator") || ContainsText(ObjectData.name, "vertical") || Bounds.h > Bounds.w};
			Spec.RotationDegrees = ObjectData.transform.rotationDegrees;
			if (Object->HasComponent<PlatformerScene::FMovingPlatformComponent>())
			{
				const PlatformerScene::FMovingPlatformComponent& Component = Object->GetComponent<PlatformerScene::FMovingPlatformComponent>();
				Spec.verticalMotion = Component.VerticalMotion;
				Spec.MotionAmplitude = Component.Amplitude;
				Spec.MotionSpeed = Component.Speed;
				Spec.MotionPhase = Component.Phase;
			}
			ScenePhysicsBodies.push_back(Spec);
			++GameplayObjectCount;
			continue;
		}
		if (ObjectData.className == "PhysicsBridgeObject")
		{
			ScenePhysicsRigSpec Spec{
				ScenePhysicsRigSpec::Type::Bridge,
				ObjectData.name,
				Bounds};
			if (Object->HasComponent<PlatformerScene::FPhysicsBridgeComponent>())
			{
				const PlatformerScene::FPhysicsBridgeComponent& Component = Object->GetComponent<PlatformerScene::FPhysicsBridgeComponent>();
				Spec.SegmentCount = Component.SegmentCount;
			}
			ScenePhysicsRigs.push_back(Spec);
			++GameplayObjectCount;
			continue;
		}
		if (ObjectData.className == "PhysicsChainObject")
		{
			ScenePhysicsRigSpec Spec{
				ScenePhysicsRigSpec::Type::Chain,
				ObjectData.name,
				Bounds};
			if (Object->HasComponent<PlatformerScene::FPhysicsChainComponent>())
			{
				const PlatformerScene::FPhysicsChainComponent& Component = Object->GetComponent<PlatformerScene::FPhysicsChainComponent>();
				Spec.SegmentCount = Component.LinkCount;
			}
			ScenePhysicsRigs.push_back(Spec);
			++GameplayObjectCount;
			continue;
		}
		if (PlatformerScene::IsPlatformerGameplayClass(ObjectData.className))
		{
			++GameplayObjectCount;
			continue;
		}

		if (!ObjectData.visible || ObjectData.kind != AE::Scene::ObjectKind::AssetInstance || ObjectData.assetId.empty())
		{
			continue;
		}

		const std::filesystem::path AssetPath = ResolveEditorSceneAssetPath(ObjectData.assetId);

		EditorSceneProp Prop;
		Prop.name = ObjectData.name;
		Prop.assetId = ObjectData.assetId;
		Prop.bounds = Bounds;
		Prop.rotationDegrees = ObjectData.transform.rotationDegrees;
		Prop.texture = GetEditorSceneTexture(ObjectData.assetId, AssetPath);
		editorSceneProps.push_back(Prop);
	}

	if (!SceneCoins.empty())
	{
		coins = std::move(SceneCoins);
	}
	EditorSolidPlatforms = std::move(SceneSolidPlatforms);
	sceneDrivenLevel = !EditorSolidPlatforms.empty();
	if (sceneDrivenLevel)
	{
		solidPlatforms.clear();
		levelTiles.assign(LevelRows, std::string(LevelCols, '.'));
	}
	editorSceneEnemies = std::move(SceneEnemies);
	if (!editorSceneEnemies.empty())
	{
		enemies = editorSceneEnemies;
		sceneEnemiesLoaded = true;
		scriptedEnemiesLoaded = false;
		enemyScriptPath = "scene:" + (SceneLabel.empty() ? std::string("memory") : SceneLabel);
	}
	editorScenePhysicsBodies = std::move(ScenePhysicsBodies);
	editorScenePhysicsRigs = std::move(ScenePhysicsRigs);
	scenePhysicsLoaded = !editorScenePhysicsBodies.empty() || !editorScenePhysicsRigs.empty();

	if (!editorSceneProps.empty() || GameplayObjectCount > 0)
	{
		std::cout
			<< "Loaded Platformer editor scene: props=" << editorSceneProps.size()
			<< " gameplayObjects=" << GameplayObjectCount
			<< " solidPlatforms=" << EditorSolidPlatforms.size()
			<< " enemies=" << editorSceneEnemies.size()
			<< " physicsBodies=" << editorScenePhysicsBodies.size()
			<< " physicsRigs=" << editorScenePhysicsRigs.size()
			<< std::endl;
	}
}

void PlatformerGameModule::ClearEditorSceneProps()
{
	sceneDrivenLevel = false;
	sceneEnemiesLoaded = false;
	scenePhysicsLoaded = false;
	editorSceneProps.clear();
	EditorSolidPlatforms.clear();
	editorSceneEnemies.clear();
	editorScenePhysicsBodies.clear();
	editorScenePhysicsRigs.clear();
	editorSceneObjects.clear();
	editorSceneRegistry.reset();
	for (auto& texture : editorSceneTextures)
	{
		if (texture.second)
		{
			SDL_DestroyTexture(texture.second);
		}
	}
	editorSceneTextures.clear();
}

void PlatformerGameModule::RefreshEditorSceneTextures()
{
	if (!imageSystemInitialized || !renderer)
	{
		return;
	}

	for (EditorSceneProp& prop : editorSceneProps)
	{
		if (!prop.texture)
		{
			prop.texture = GetEditorSceneTexture(prop.assetId, ResolveEditorSceneAssetPath(prop.assetId));
		}
	}
}

std::filesystem::path PlatformerGameModule::ResolveEditorSceneAssetPath(const std::string& assetId) const
{
	const std::string projectPrefix = "Project/";
	const std::string enginePrefix = "Engine/";

	if (HasPrefix(assetId, projectPrefix))
	{
		return projectContentRoot / assetId.substr(projectPrefix.size());
	}
	if (HasPrefix(assetId, enginePrefix))
	{
		const std::filesystem::path engineContentRoot = FindEngineContentRoot();
		if (!engineContentRoot.empty())
		{
			return engineContentRoot / assetId.substr(enginePrefix.size());
		}
	}

	return projectContentRoot / assetId;
}

SDL_Texture* PlatformerGameModule::GetEditorSceneTexture(const std::string& assetId, const std::filesystem::path& path)
{
	const auto cached = editorSceneTextures.find(assetId);
	if (cached != editorSceneTextures.end())
	{
		return cached->second;
	}

	SDL_Texture* texture = nullptr;
	if (!imageSystemInitialized || !renderer || path.empty())
	{
		return nullptr;
	}

	if (imageSystemInitialized && renderer && !path.empty())
	{
		SDL_Surface* surface = IMG_Load(path.string().c_str());
		if (surface)
		{
			texture = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_FreeSurface(surface);
			if (texture)
			{
				SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
			}
		}
	}

	if (texture)
	{
		editorSceneTextures[assetId] = texture;
	}
	return texture;
}

void PlatformerGameModule::DrawEditorSceneProps() const
{
	for (const EditorSceneProp& prop : editorSceneProps)
	{
		if (prop.texture)
		{
			SDL_Rect destination{
				RoundToInt(prop.bounds.x - cameraX),
				RoundToInt(prop.bounds.y),
				RoundToInt(prop.bounds.w),
				RoundToInt(prop.bounds.h)};
			SDL_RenderCopyEx(renderer, prop.texture, nullptr, &destination, prop.rotationDegrees, nullptr, SDL_FLIP_NONE);
		}
		else
		{
			DrawRotatedRect(prop.bounds, prop.rotationDegrees, SDL_Color{92, 153, 214, 185});
		}
	}
}
#endif

void PlatformerGameModule::DrawPlayer() const
{
	const RectF bounds = PlayerRect();
	DrawRect(bounds, PlayerBody);
	DrawRect(RectF{bounds.x + 5.0f, bounds.y + 6.0f, bounds.w - 10.0f, 10.0f}, PlayerAccent);
	DrawRect(RectF{bounds.x + 7.0f, bounds.y + 18.0f, 6.0f, 6.0f}, SDL_Color{20, 35, 48, 255});
	DrawRect(RectF{bounds.x + bounds.w - 13.0f, bounds.y + 18.0f, 6.0f, 6.0f}, SDL_Color{20, 35, 48, 255});
	DrawRect(RectF{bounds.x + 6.0f, bounds.y + bounds.h - 7.0f, 8.0f, 7.0f}, SDL_Color{24, 70, 105, 255});
	DrawRect(RectF{bounds.x + bounds.w - 14.0f, bounds.y + bounds.h - 7.0f, 8.0f, 7.0f}, SDL_Color{24, 70, 105, 255});
}

void PlatformerGameModule::DrawFinish() const
{
	DrawRect(RectF{finish.x + 4.0f, finish.y, 5.0f, finish.h}, SDL_Color{48, 62, 74, 255});
	DrawRect(RectF{finish.x + 9.0f, finish.y + 6.0f, 34.0f, 22.0f}, player.won ? SDL_Color{245, 196, 48, 255} : SDL_Color{220, 68, 78, 255});
}

void PlatformerGameModule::DrawHud() const
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

void PlatformerGameModule::DrawRect(const RectF& rect, SDL_Color color) const
{
	DrawScreenRect(
		static_cast<int>(std::round(rect.x - cameraX)),
		static_cast<int>(std::round(rect.y)),
		static_cast<int>(std::round(rect.w)),
		static_cast<int>(std::round(rect.h)),
		color);
}

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
void PlatformerGameModule::DrawRotatedRect(const RectF& Bounds, float RotationDegrees, SDL_Color Color) const
{
	const std::array<AE::Math::FVector2D, 4> WorldVertices = RotatedRectVertices(Bounds, RotationDegrees);
	std::vector<AE::Math::FVector2D> ScreenVertices;
	ScreenVertices.reserve(WorldVertices.size());
	for (const AE::Math::FVector2D& Vertex : WorldVertices)
	{
		ScreenVertices.emplace_back(Vertex.X - cameraX, Vertex.Y);
	}

	DrawFilledPolygon(ScreenVertices, Color);
	const auto DarkenChannel = [](Uint8 Channel)
	{
		return static_cast<Uint8>(Channel > 48 ? Channel - 48 : 0);
	};
	DrawPolyline(ScreenVertices, SDL_Color{DarkenChannel(Color.r), DarkenChannel(Color.g), DarkenChannel(Color.b), Color.a}, true);
}
#endif

void PlatformerGameModule::DrawWorldLine(const AE::Math::FVector2D& from, const AE::Math::FVector2D& to, SDL_Color color) const
{
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawLine(
		renderer,
		RoundToInt(from.X - cameraX),
		RoundToInt(from.Y),
		RoundToInt(to.X - cameraX),
		RoundToInt(to.Y));
}

void PlatformerGameModule::DrawFilledCircle(int centerX, int centerY, int radius, SDL_Color color) const
{
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	for (int y = -radius; y <= radius; ++y)
	{
		const int span = static_cast<int>(std::sqrt(static_cast<float>(radius * radius - y * y)));
		SDL_RenderDrawLine(renderer, centerX - span, centerY + y, centerX + span, centerY + y);
	}
}

void PlatformerGameModule::DrawFilledPolygon(const std::vector<AE::Math::FVector2D>& vertices, SDL_Color color) const
{
	if (vertices.size() < 3)
	{
		return;
	}

	float minY = vertices.front().Y;
	float maxY = vertices.front().Y;
	for (const AE::Math::FVector2D& vertex : vertices)
	{
		minY = std::min(minY, vertex.Y);
		maxY = std::max(maxY, vertex.Y);
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

		for (SizeT i = 0; i < vertices.size(); ++i)
		{
			const AE::Math::FVector2D& a = vertices[i];
			const AE::Math::FVector2D& b = vertices[(i + 1) % vertices.size()];
			if ((a.Y <= scanY && b.Y > scanY) || (b.Y <= scanY && a.Y > scanY))
			{
				const float t = (scanY - a.Y) / (b.Y - a.Y);
				intersections.push_back(a.X + t * (b.X - a.X));
			}
		}

		std::sort(intersections.begin(), intersections.end());
		for (SizeT i = 0; i + 1 < intersections.size(); i += 2)
		{
			const int x0 = std::max(0, static_cast<int>(std::ceil(intersections[i])));
			const int x1 = std::min(WindowWidth - 1, static_cast<int>(std::floor(intersections[i + 1])));
			SDL_RenderDrawLine(renderer, x0, y, x1, y);
		}
	}
}

void PlatformerGameModule::DrawPolyline(const std::vector<AE::Math::FVector2D>& vertices, SDL_Color color, bool closed) const
{
	if (vertices.size() < 2)
	{
		return;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	for (SizeT i = 0; i + 1 < vertices.size(); ++i)
	{
		SDL_RenderDrawLine(
			renderer,
			RoundToInt(vertices[i].X),
			RoundToInt(vertices[i].Y),
			RoundToInt(vertices[i + 1].X),
			RoundToInt(vertices[i + 1].Y));
	}
	if (closed)
	{
		SDL_RenderDrawLine(
			renderer,
			RoundToInt(vertices.back().X),
			RoundToInt(vertices.back().Y),
			RoundToInt(vertices.front().X),
			RoundToInt(vertices.front().Y));
	}
}

void PlatformerGameModule::DrawScreenRect(int x, int y, int w, int h, SDL_Color color) const
{
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_Rect rect{x, y, w, h};
	SDL_RenderFillRect(renderer, &rect);
}
