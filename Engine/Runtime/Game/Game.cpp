#include "Game.h"
#include <filesystem>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "LevelLoader.h"
#include "../Utilities/Utils.h"
#include "../Logging/Logger.h"
#include "../Components/TransformComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/KeyboardControlledComponent.h"
#include "../Components/CameraFollowComponent.h"
#include "../Components/HealthComponent.h"
#include "../Components/TextRenderComponent.h"
#include "../Components/ProjectileComponent.h"
#include "../Events/CollisionEvent.h"

#include "../Systems/RenderSystem.h"
#include "../Systems/MovementSystem.h"
#include "../Systems/AnimationSystem.h"
#include "../Systems/CollisionSystem.h"
#include "../Systems/RenderColliderSystem.h"
#include "../Systems/DamageSystem.h"
#include "../Systems/KeyboardControlSystem.h"
#include "../Systems/CameraMovementSystem.h"
#include "../Systems/ProjectileEmitterSystem.h"
#include "../Systems/ProjectileLifeCycleSystem.h"
#include "../Systems/RenderTextSystem.h"
#include "../Systems/RenderHealthBarSystem.h"
#include "../Systems/RenderGUISystem.h"
#include "../EnginePhysicsBridge/EnginePhysicsBridge.h"
#include "../Systems/PhysicsContactResponseSystem.h"
#include "../Systems/PhysicsDebugRenderSystem.h"
#include "../Logging/LogBus.h"


#include <imgui/imgui_impl_sdl.h>


#include "../Utilities/Macro.h"

namespace
{
	constexpr int DefaultWindowedWidth = 1280;
	constexpr int DefaultWindowedHeight = 720;

	double ElapsedMs(Uint64 startCounter, Uint64 endCounter)
	{
		return static_cast<double>(endCounter - startCounter) * 1000.0 /
			static_cast<double>(SDL_GetPerformanceFrequency());
	}

	bool IsFullscreenToggleKey(const SDL_KeyboardEvent& keyEvent)
	{
		const SDL_Keycode key = keyEvent.keysym.sym;
		const bool altPressed = (keyEvent.keysym.mod & KMOD_ALT) != 0;
		return key == SDLK_F11 || (altPressed && (key == SDLK_RETURN || key == SDLK_KP_ENTER));
	}

	bool SetContentWorkingDirectory()
	{
		namespace fs = std::filesystem;

		const fs::path levelScriptPath = fs::path("Content") / "scripts" / "Level1.lua";
		if (fs::exists(levelScriptPath))
		{
			return true;
		}

		const fs::path candidateRoots[] = {
			fs::path("AmberEngine"),
			fs::path(".."),
			fs::path("..") / "..",
			fs::path("..") / ".." / "..",
			fs::path("..") / "AmberEngine",
			fs::path("..") / ".." / "AmberEngine"
		};

		for (const auto& candidateRoot : candidateRoots)
		{
			if (!fs::exists(candidateRoot / levelScriptPath))
			{
				continue;
			}

			std::error_code error;
			fs::current_path(candidateRoot, error);
			if (error)
			{
				AE::Logger::Err("Failed to set content working directory to " + candidateRoot.string() + ": " + error.message());
				return false;
			}

			return true;
		}

		AE::Logger::Err("Could not locate Content/scripts/Level1.lua from the current working directory.");
		return false;
	}
}

int Game::WindowWidth;
int Game::WindowHeight;
int Game::MapHeight;
int Game::MapWidth;

bool Game::isEditMode = false;
/**
 * 
 */
Game::Game()
{
	isRunning = false;
	isDebug = false;
	registry = std::make_unique<Registry>();
	assetManager = std::make_unique<AssetManager>();
	eventBus = std::make_unique<EventBus>();
	AE::Logger::Log("Game costructor called");
}

/**
 * 
 */
Game::~Game()
{
	AE::Logger::Log("Game destructor called");
}

/**
 * 
 */
void Game::Initialize()
{
	AE::Logger::Log("Initialize Game engine");
	check(SDL_Init(SDL_INIT_EVERYTHING) == 0, "Error initialize SDL.");
	check(TTF_Init() == 0, "Error initialize SDL TTF ");
	
	WindowWidth = DefaultWindowedWidth;
	WindowHeight = DefaultWindowedHeight;

	SDL_DisplayMode displayMode{};
	if (isFullscreen && SDL_GetCurrentDisplayMode(0, &displayMode) == 0 && displayMode.w > 0 && displayMode.h > 0)
	{
		WindowWidth = displayMode.w;
		WindowHeight = displayMode.h;
	}
	else if (isFullscreen)
	{
		AE::Logger::Warn(std::string("SDL display mode unavailable, using fallback 1280x720: ") + SDL_GetError());
	}

	window = SDL_CreateWindow(
		"AmberEngine",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		WindowWidth,
		WindowHeight,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	check(window, "Error Creating SDL Window");

	renderer = SDL_CreateRenderer(window, -1, 0);
	check(renderer, "Error Creating SDL renderer");

	// imgui context 
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForD3D(window);
	ImGuiSDL::Initialize(renderer, WindowWidth, WindowHeight);


	//initialize the camera
	camera.x = 0;
	camera.y = 0;
	camera.h = WindowHeight;
	camera.w = WindowWidth;

	ApplyFullscreenMode();
	isRunning = true;
}

bool Game::IsRunning() const
{
	return isRunning;
}

void Game::SetDebugEnabled(bool enabled)
{
	isDebug = enabled;
}

void Game::SetFullscreenEnabled(bool enabled)
{
	isFullscreen = enabled;
	if (window)
	{
		ApplyFullscreenMode();
	}
}

bool Game::IsFullscreenEnabled() const
{
	return isFullscreen;
}

void Game::SetLevelNumber(int level)
{
	levelNumber = level > 0 ? level : 1;
}

void Game::InitializeSystems()
{
	// registry logic 
	registry->AddSystem<MovementSystem>();
	registry->AddSystem<RenderSystem>();
	registry->AddSystem<AnimationSystem>();
	registry->AddSystem<CollisionSystem>();
	registry->AddSystem<RenderColliderSystem>();
	registry->AddSystem<DamageSystem>();
	registry->AddSystem<KeyboardControlSystem>();
	registry->AddSystem<CameraMovementSystem>();
	registry->AddSystem<ProjectileEmitterSystem>();
	registry->AddSystem<ProjectileLifeCycleSystem>();
	registry->AddSystem<RenderTextSystem>();
	registry->AddSystem<RenderHealthBarSystem>();
	registry->AddSystem<RenderGUISystem>();
	registry->AddSystem<PhysicsBodyLifecycleSystem>();
	registry->AddSystem<PhysicsContactEventSystem>();
	registry->AddSystem<PhysicsContactResponseSystem>();
	registry->AddSystem<PhysicsDebugRenderSystem>();
	registry->AddSystem<PhysicsSyncSystem>();
	registry->AddSystem<PhysicsVelocitySystem>();
	registry->AddSystem<PhysicsWorldSystem>(0.0f);
}

/**
 * 
 */
void Game::Setup()
{
	InitializeSystems();
	if (!SetContentWorkingDirectory())
	{
		return;
	}

	LevelLoader loader;
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::os);
	loader.LoadLevel(lua,registry,assetManager,renderer,levelNumber);
}

/**
 * 
 */
void Game::Run()
{
	Setup();

	while(isRunning)
	{
		ProcessInput();
		if (!isEditMode) Update();
		
		Render();

	}
}

bool Game::RunPhysicsContactSmokeTest()
{
	InitializeSystems();

	auto& physicsWorldSystem = registry->GetSystem<PhysicsWorldSystem>();

	Entity enemy = registry->CreateEntity();
	enemy.Group("enemies");
	enemy.AddComponent<TransformComponent>(glm::vec2(100.0f, 100.0f), glm::vec2(1.0f, 1.0f), 0.0);
	enemy.AddComponent<RigidBodyComponent>(glm::vec2(0.0f, 0.0f));
	enemy.AddComponent<BoxCollisionComponent>(0, 0);
	enemy.AddComponent<HealthComponent>(100);

	PhysicsBodyDefinition enemyBodyDefinition;
	enemyBodyDefinition.mass = 0.0f;
	enemyBodyDefinition.width = 16.0f;
	enemyBodyDefinition.height = 16.0f;
	enemyBodyDefinition.collisionCategory = EnginePhysicsCollision::Enemy;
	enemyBodyDefinition.collisionMask = EnginePhysicsCollision::PlayerProjectile;
	enemy.AddComponent<PhysicsBodyComponent>(
		PhysicsBodyFactory::Create(
			physicsWorldSystem,
			enemy.GetComponent<TransformComponent>(),
			enemyBodyDefinition));

	Entity projectile = registry->CreateEntity();
	projectile.Group("projectiles");
	projectile.AddComponent<TransformComponent>(glm::vec2(108.0f, 106.0f), glm::vec2(1.0f, 1.0f), 0.0);
	projectile.AddComponent<RigidBodyComponent>(glm::vec2(0.0f, 0.0f));
	projectile.AddComponent<BoxCollisionComponent>(0, 0);
	projectile.AddComponent<ProjectileComponent>(true, 25, 1000, static_cast<int>(SDL_GetTicks()));

	PhysicsBodyDefinition projectileBodyDefinition;
	projectileBodyDefinition.mass = 1.0f;
	projectileBodyDefinition.width = 4.0f;
	projectileBodyDefinition.height = 4.0f;
	projectileBodyDefinition.collisionCategory = EnginePhysicsCollision::PlayerProjectile;
	projectileBodyDefinition.collisionMask = EnginePhysicsCollision::Enemy;
	projectileBodyDefinition.isSensor = true;
	projectile.AddComponent<PhysicsBodyComponent>(
		PhysicsBodyFactory::Create(
			physicsWorldSystem,
			projectile.GetComponent<TransformComponent>(),
			projectileBodyDefinition));

	eventBus->Reset();
	registry->GetSystem<DamageSystem>().SubscribeToEvents(eventBus);
	registry->Update();

	registry->GetSystem<PhysicsSyncSystem>().PushToPhysics();
	registry->GetSystem<PhysicsVelocitySystem>().Update();
	physicsWorldSystem.Update(1.0 / FPS);

	const bool hadPhysicsContact = !physicsWorldSystem.GetWorld().GetContacts().empty();

	registry->GetSystem<PhysicsContactEventSystem>().Update(eventBus, physicsWorldSystem);

	const int enemyHealth = enemy.GetComponent<HealthComponent>().healthPercentage;
	const bool damageApplied = enemyHealth == 75;
	const bool projectileScheduledForKill =
		registry->GetEntitiesToBeKilled().find(projectile) != registry->GetEntitiesToBeKilled().end();

	Entity aabbEnemy = registry->CreateEntity();
	aabbEnemy.Group("enemies");
	aabbEnemy.AddComponent<BoxCollisionComponent>(4, 4);
	aabbEnemy.AddComponent<HealthComponent>(100);

	Entity aabbProjectile = registry->CreateEntity();
	aabbProjectile.Group("projectiles");
	aabbProjectile.AddComponent<BoxCollisionComponent>(4, 4);
	aabbProjectile.AddComponent<ProjectileComponent>(true, 25, 1000, static_cast<int>(SDL_GetTicks()));

	eventBus->EmitEvent<CollisionEvent>(aabbProjectile, aabbEnemy, false);

	const int aabbEnemyHealth = aabbEnemy.GetComponent<HealthComponent>().healthPercentage;
	const bool aabbProjectileDamageIgnored = aabbEnemyHealth == 100 &&
		registry->GetEntitiesToBeKilled().find(aabbProjectile) == registry->GetEntitiesToBeKilled().end();

	if (!hadPhysicsContact)
	{
		AE::Logger::Err("Physics contact smoke test failed: no physics contact was generated.");
	}
	if (!damageApplied)
	{
		AE::Logger::Err("Physics contact smoke test failed: enemy health is " + std::to_string(enemyHealth) + ", expected 75.");
	}
	if (!projectileScheduledForKill)
	{
		AE::Logger::Err("Physics contact smoke test failed: projectile was not scheduled for removal.");
	}
	if (!aabbProjectileDamageIgnored)
	{
		AE::Logger::Err("Physics contact smoke test failed: AABB projectile damage was not ignored.");
	}

	const bool passed = hadPhysicsContact && damageApplied && projectileScheduledForKill && aabbProjectileDamageIgnored;
	AE::Logger::Log(std::string("Physics contact smoke test ") + (passed ? "passed." : "failed."));
	return passed;
}

bool Game::RunPhysicsObstacleSmokeTest()
{
	InitializeSystems();

	MapWidth = 1000;
	MapHeight = 1000;

	auto& physicsWorldSystem = registry->GetSystem<PhysicsWorldSystem>();
	auto& contactResponseSystem = registry->GetSystem<PhysicsContactResponseSystem>();

	PhysicsContactPolicy enemyObstacleBouncePolicy;
	enemyObstacleBouncePolicy.firstLayerMask = EnginePhysicsCollision::Enemy;
	enemyObstacleBouncePolicy.secondLayerMask = EnginePhysicsCollision::Obstacle;
	enemyObstacleBouncePolicy.action = PhysicsContactAction::Bounce;
	enemyObstacleBouncePolicy.responder = PhysicsContactResponder::First;
	enemyObstacleBouncePolicy.bidirectional = true;
	enemyObstacleBouncePolicy.name = "smoke_enemy_obstacle_bounce";
	contactResponseSystem.ClearPolicies();
	contactResponseSystem.AddPolicy(enemyObstacleBouncePolicy);

	Entity player = registry->CreateEntity();
	player.Tag("player");
	player.AddComponent<TransformComponent>(glm::vec2(100.0f, 100.0f), glm::vec2(1.0f, 1.0f), 0.0);
	player.AddComponent<RigidBodyComponent>(glm::vec2(120.0f, 0.0f));
	player.AddComponent<BoxCollisionComponent>(32, 32);

	PhysicsBodyDefinition playerBodyDefinition;
	playerBodyDefinition.mass = 1.0f;
	playerBodyDefinition.width = 32.0f;
	playerBodyDefinition.height = 32.0f;
	playerBodyDefinition.collisionCategory = EnginePhysicsCollision::Player;
	playerBodyDefinition.collisionMask = EnginePhysicsCollision::Obstacle;
	playerBodyDefinition.isSensor = false;
	player.AddComponent<PhysicsBodyComponent>(
		PhysicsBodyFactory::Create(
			physicsWorldSystem,
			player.GetComponent<TransformComponent>(),
			playerBodyDefinition));

	Entity obstacle = registry->CreateEntity();
	obstacle.Group("obstacles");
	obstacle.AddComponent<TransformComponent>(glm::vec2(126.0f, 100.0f), glm::vec2(1.0f, 1.0f), 0.0);
	obstacle.AddComponent<BoxCollisionComponent>(32, 32);

	PhysicsBodyDefinition obstacleBodyDefinition;
	obstacleBodyDefinition.mass = 0.0f;
	obstacleBodyDefinition.width = 32.0f;
	obstacleBodyDefinition.height = 32.0f;
	obstacleBodyDefinition.pullPositionFromPhysics = false;
	obstacleBodyDefinition.pullRotationFromPhysics = false;
	obstacleBodyDefinition.collisionCategory = EnginePhysicsCollision::Obstacle;
	obstacleBodyDefinition.collisionMask = EnginePhysicsCollision::Player;
	obstacleBodyDefinition.isSensor = false;
	obstacle.AddComponent<PhysicsBodyComponent>(
		PhysicsBodyFactory::Create(
			physicsWorldSystem,
			obstacle.GetComponent<TransformComponent>(),
			obstacleBodyDefinition));

	Entity enemy = registry->CreateEntity();
	enemy.Group("enemies");
	enemy.AddComponent<TransformComponent>(glm::vec2(300.0f, 100.0f), glm::vec2(1.0f, 1.0f), 0.0);
	enemy.AddComponent<RigidBodyComponent>(glm::vec2(60.0f, 0.0f));
	enemy.AddComponent<SpriteComponent>("policy-smoke-texture", 32, 32);
	enemy.AddComponent<BoxCollisionComponent>(32, 32);

	PhysicsBodyDefinition enemyBodyDefinition;
	enemyBodyDefinition.mass = 1.0f;
	enemyBodyDefinition.width = 32.0f;
	enemyBodyDefinition.height = 32.0f;
	enemyBodyDefinition.collisionCategory = EnginePhysicsCollision::Enemy;
	enemyBodyDefinition.collisionMask = EnginePhysicsCollision::Obstacle;
	enemyBodyDefinition.isSensor = true;
	enemy.AddComponent<PhysicsBodyComponent>(
		PhysicsBodyFactory::Create(
			physicsWorldSystem,
			enemy.GetComponent<TransformComponent>(),
			enemyBodyDefinition));

	Entity enemyObstacle = registry->CreateEntity();
	enemyObstacle.Group("obstacles");
	enemyObstacle.AddComponent<TransformComponent>(glm::vec2(326.0f, 100.0f), glm::vec2(1.0f, 1.0f), 0.0);
	enemyObstacle.AddComponent<BoxCollisionComponent>(32, 32);

	PhysicsBodyDefinition enemyObstacleBodyDefinition;
	enemyObstacleBodyDefinition.mass = 0.0f;
	enemyObstacleBodyDefinition.width = 32.0f;
	enemyObstacleBodyDefinition.height = 32.0f;
	enemyObstacleBodyDefinition.pullPositionFromPhysics = false;
	enemyObstacleBodyDefinition.pullRotationFromPhysics = false;
	enemyObstacleBodyDefinition.collisionCategory = EnginePhysicsCollision::Obstacle;
	enemyObstacleBodyDefinition.collisionMask = EnginePhysicsCollision::Enemy;
	enemyObstacleBodyDefinition.isSensor = false;
	enemyObstacle.AddComponent<PhysicsBodyComponent>(
		PhysicsBodyFactory::Create(
			physicsWorldSystem,
			enemyObstacle.GetComponent<TransformComponent>(),
			enemyObstacleBodyDefinition));

	eventBus->Reset();
	contactResponseSystem.SubscribeToEvents(eventBus);
	registry->Update();

	const float playerStartX = player.GetComponent<TransformComponent>().position.x;

	registry->GetSystem<MovementSystem>().Update(1.0 / FPS);
	registry->GetSystem<PhysicsSyncSystem>().PushToPhysics();
	registry->GetSystem<PhysicsVelocitySystem>().Update();
	physicsWorldSystem.Update(1.0 / FPS);
	registry->GetSystem<PhysicsSyncSystem>().PullFromPhysics();
	registry->GetSystem<PhysicsContactEventSystem>().Update(eventBus, physicsWorldSystem);

	const bool hadPhysicsContact = !physicsWorldSystem.GetWorld().GetContacts().empty();
	const auto& playerPhysicsBody = player.GetComponent<PhysicsBodyComponent>();
	const auto& playerTransform = player.GetComponent<TransformComponent>();
	const bool playerVelocityStopped =
		playerPhysicsBody.body && playerPhysicsBody.body->velocity.x <= 0.01f;
	const bool playerDidNotMoveDeeper = playerTransform.position.x <= playerStartX + 0.01f;
	const auto& enemyRigidBody = enemy.GetComponent<RigidBodyComponent>();
	const auto& enemyPhysicsBody = enemy.GetComponent<PhysicsBodyComponent>();
	const auto& enemySprite = enemy.GetComponent<SpriteComponent>();
	const bool enemyBounceApplied = enemyRigidBody.velocity.x < 0.0f;
	const bool enemyPhysicsVelocitySynced =
		enemyPhysicsBody.body && enemyPhysicsBody.body->velocity.x < 0.0f;
	const bool enemySpriteFlipped = enemySprite.flip == SDL_FLIP_HORIZONTAL;

	if (!hadPhysicsContact)
	{
		AE::Logger::Err("Physics obstacle smoke test failed: no player/obstacle contact was generated.");
	}
	if (!playerVelocityStopped)
	{
		const float velocityX = playerPhysicsBody.body ? playerPhysicsBody.body->velocity.x : 0.0f;
		AE::Logger::Err("Physics obstacle smoke test failed: player velocity x is " + std::to_string(velocityX) + ", expected <= 0.01.");
	}
	if (!playerDidNotMoveDeeper)
	{
		AE::Logger::Err("Physics obstacle smoke test failed: player moved deeper into the obstacle.");
	}
	if (!enemyBounceApplied)
	{
		AE::Logger::Err("Physics obstacle smoke test failed: enemy obstacle bounce policy did not reverse rigid body velocity.");
	}
	if (!enemyPhysicsVelocitySynced)
	{
		AE::Logger::Err("Physics obstacle smoke test failed: enemy obstacle bounce policy did not sync physics body velocity.");
	}
	if (!enemySpriteFlipped)
	{
		AE::Logger::Err("Physics obstacle smoke test failed: enemy obstacle bounce policy did not flip the sprite.");
	}

	const bool passed =
		hadPhysicsContact &&
		playerVelocityStopped &&
		playerDidNotMoveDeeper &&
		enemyBounceApplied &&
		enemyPhysicsVelocitySynced &&
		enemySpriteFlipped;
	AE::Logger::Log(std::string("Physics obstacle smoke test ") + (passed ? "passed." : "failed."));
	return passed;
}

/**
 * 
 */
void Game::ProcessInput()
{
	SDL_Event event;
	
	while(SDL_PollEvent(&event))
	{
		ImGui_ImplSDL2_ProcessEvent(&event);
		ImGuiIO& io = ImGui::GetIO();

		int mouseX, mouseY;
		const int buttons = SDL_GetMouseState(&mouseX, &mouseY);
		io.MousePos = ImVec2(mouseX, mouseY);
		io.MouseDown[0] = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
		io.MouseDown[1] = buttons & SDL_BUTTON(SDL_BUTTON_RIGHT);

		switch(event.type)
		{
			case SDL_QUIT:
			{
				isRunning = false;
				break;
			}

			case SDL_WINDOWEVENT:
			{
				if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
					event.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					UpdateWindowDimensions();
				}
				break;
			}
		
			case SDL_KEYDOWN:
			{
				if (!event.key.repeat && IsFullscreenToggleKey(event.key))
				{
					ToggleFullscreen();
					break;
				}
				if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					isRunning = false;
				}
				if (event.key.keysym.sym == SDLK_F1)
				{
					showDiagnostics = !showDiagnostics;
					break;
				}
				if (event.key.keysym.sym == SDLK_F3)
				{
					showOutputLog = !showOutputLog;
					break;
				}
				if (io.WantCaptureKeyboard)
				{
					break;
				}
				if (event.key.keysym.sym == SDLK_d)
				{
					isDebug = !isDebug;
				}
				if (event.key.keysym.sym == SDLK_p)
				{
					isEditMode = !isEditMode;
				}

				eventBus->EmitEvent<KeyPressedEvent>(event.key.keysym.sym);
				break;
			}
					
		}
	}
}

void Game::Update()
{
    // If we are too fast, waste some time until we reach the MILLISECS_PER_FRAME
    int timeToWait = MILLISECS_PER_FRAME - (SDL_GetTicks() - millisecsPreviousFrame);

    if (timeToWait > 0 && timeToWait <= MILLISECS_PER_FRAME) 
	{
        SDL_Delay(timeToWait);
    }

    // The difference in ticks since the last frame, converted to seconds
    double deltaTime = (SDL_GetTicks() - millisecsPreviousFrame) / 1000.0;

    // Store the "previous" frame time
    millisecsPreviousFrame = SDL_GetTicks();

	const Uint64 updateStart = SDL_GetPerformanceCounter();
	eventBus->Reset();

	// update subscribe to event system (event bus)
	registry->GetSystem<DamageSystem>().SubscribeToEvents(eventBus);
	registry->GetSystem<PhysicsContactResponseSystem>().SubscribeToEvents(eventBus);
	registry->GetSystem<KeyboardControlSystem>().SubscribeToEvents(eventBus);
	registry->GetSystem<ProjectileEmitterSystem>().SubscribeToEvents(eventBus);

	registry->GetSystem<PhysicsBodyLifecycleSystem>().Update(
		registry,
		registry->GetSystem<PhysicsWorldSystem>());
	registry->Update();

	// update allways system 
	registry->GetSystem<MovementSystem>().Update(deltaTime);
	registry->GetSystem<PhysicsSyncSystem>().PushToPhysics();
	registry->GetSystem<PhysicsVelocitySystem>().Update();
	registry->GetSystem<PhysicsWorldSystem>().Update(deltaTime);
	registry->GetSystem<PhysicsSyncSystem>().PullFromPhysics();
	registry->GetSystem<PhysicsContactEventSystem>().Update(
		eventBus,
		registry->GetSystem<PhysicsWorldSystem>());
	registry->GetSystem<AnimationSystem>().Update();
	registry->GetSystem<CollisionSystem>().Update(eventBus);
	registry->GetSystem<CameraMovementSystem>().Update(camera);
	registry->GetSystem<ProjectileEmitterSystem>().Update(registry);
	registry->GetSystem<ProjectileLifeCycleSystem>().Update();
	lastUpdateMs = ElapsedMs(updateStart, SDL_GetPerformanceCounter());

}

/**
 * Render frame function 
 */
void Game::Render()
{
	const Uint64 renderStart = SDL_GetPerformanceCounter();

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);

	registry->GetSystem<RenderSystem>().Update(renderer,assetManager, camera);
	registry->GetSystem<RenderTextSystem>().Update(renderer, assetManager, camera);
	registry->GetSystem<RenderHealthBarSystem>().Update(renderer, assetManager, camera);
	
	/** debug box collision from entity */
	if (isDebug)
	{
		registry->GetSystem<RenderColliderSystem>().Update(renderer, camera);
		registry->GetSystem<PhysicsDebugRenderSystem>().Update(
			renderer,
			camera,
			registry->GetSystem<PhysicsWorldSystem>());
	}

	if (isEditMode)
	{
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();
		registry->GetSystem<RenderGUISystem>().Update(registry, false);
	}
	else
	{
		RenderDiagnosticsUi();
	}

	lastRenderMs = ElapsedMs(renderStart, SDL_GetPerformanceCounter());
	SDL_RenderPresent(renderer);

}

void Game::RenderDiagnosticsUi()
{
	if (!ImGui::GetCurrentContext())
	{
		return;
	}

	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();

	if (showDiagnostics)
	{
		ImGui::SetNextWindowPos(ImVec2(14.0f, 14.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(340.0f, 300.0f), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Performance", &showDiagnostics))
		{
			const ImGuiIO& io = ImGui::GetIO();
			ImGui::TextUnformatted("GameEngineApp");
			ImGui::Separator();
			ImGui::Text("FPS: %.1f", io.Framerate);
			ImGui::Text("Frame: %.3f ms", io.Framerate > 0.0f ? 1000.0f / io.Framerate : 0.0f);
			ImGui::Text("Update: %.3f ms", lastUpdateMs);
			ImGui::Text("Render: %.3f ms", lastRenderMs);
			ImGui::Text("Level: %d", levelNumber);
			ImGui::Text("Debug draw: %s", isDebug ? "on" : "off");
			ImGui::Text("Fullscreen: %s", isFullscreen ? "yes" : "no");
			ImGui::Text("Camera: %d, %d, %d, %d", camera.x, camera.y, camera.w, camera.h);

			if (registry && registry->HasSystem<PhysicsWorldSystem>())
			{
				AE::Physics::World& world = registry->GetSystem<PhysicsWorldSystem>().GetWorld();
				const AE::Physics::WorldStats& stats = world.GetLastStats();
				ImGui::Separator();
				ImGui::Text("Bodies: %zu", world.GetBodies().size());
				ImGui::Text("Contacts: %zu", world.GetContacts().size());
				ImGui::Text("Pairs: %zu -> %zu", stats.bruteForcePairs, stats.broadPhasePairs);
				ImGui::Text("Physics: %.3f ms", stats.totalStepMs);
				ImGui::Text("Broad/Narrow: %.3f / %.3f", stats.broadPhaseMs, stats.narrowPhaseMs);
				ImGui::Text("Solver: %.3f", stats.solverPhaseMs);
				ImGui::Text(
					"Islands: %zu (max %zu bodies / %zu constraints)",
					stats.solverIslandCount,
					stats.largestSolverIslandBodyCount,
					stats.largestSolverIslandConstraintCount);
				ImGui::Text("Parallel narrow: %s (%zu jobs)", stats.parallelNarrowPhaseUsed ? "on" : "off", stats.parallelNarrowPhaseJobs);
				ImGui::Text("Parallel solver: %s (%zu jobs)", stats.parallelSolverUsed ? "on" : "off", stats.parallelSolverJobs);
			}
		}
		ImGui::End();
	}

	ImGui::SetNextWindowPos(ImVec2(14.0f, 330.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(280.0f, 130.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Controls"))
	{
		ImGui::Checkbox("Performance", &showDiagnostics);
		ImGui::Checkbox("Output Log", &showOutputLog);
		ImGui::Checkbox("Debug draw", &isDebug);
		if (ImGui::Button(isFullscreen ? "Windowed" : "Fullscreen"))
		{
			ToggleFullscreen();
		}
		ImGui::SameLine();
		if (ImGui::Button(isEditMode ? "Play" : "Edit"))
		{
			isEditMode = !isEditMode;
		}
	}
	ImGui::End();

	if (showOutputLog)
	{
		ImGui::SetNextWindowSize(ImVec2(620.0f, 260.0f), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Output Log", &showOutputLog))
		{
			if (ImGui::Button("Clear"))
			{
				AE::LogBus::Clear();
			}
			const std::vector<AE::LogBusEntry> entries = AE::LogBus::GetEntriesSnapshot();
			ImGui::SameLine();
			ImGui::Text("Entries: %zu / %zu", entries.size(), AE::LogBus::GetMaxEntries());
			ImGui::Separator();
			ImGui::BeginChild("GameOutputLogScroll", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
			for (const AE::LogBusEntry& entry : entries)
			{
				ImGui::Text("[%s] %s", entry.category.c_str(), entry.message.c_str());
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

	ImGui::Render();
	ImGuiSDL::Render(ImGui::GetDrawData());
}

void Game::ApplyFullscreenMode()
{
	if (!window)
	{
		return;
	}

	if (isFullscreen)
	{
		if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0)
		{
			AE::Logger::Warn(std::string("Could not enter fullscreen: ") + SDL_GetError());
		}
	}
	else
	{
		if (SDL_SetWindowFullscreen(window, 0) != 0)
		{
			AE::Logger::Warn(std::string("Could not leave fullscreen: ") + SDL_GetError());
		}
		SDL_SetWindowSize(window, DefaultWindowedWidth, DefaultWindowedHeight);
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}

	UpdateWindowDimensions();
	AE::Logger::Log(std::string("Window mode changed to ") + (isFullscreen ? "fullscreen" : "windowed"));
}

void Game::ToggleFullscreen()
{
	SetFullscreenEnabled(!isFullscreen);
}

void Game::UpdateWindowDimensions()
{
	if (!window)
	{
		return;
	}

	int width = 0;
	int height = 0;
	SDL_GetWindowSize(window, &width, &height);
	if ((width <= 0 || height <= 0) && renderer)
	{
		SDL_GetRendererOutputSize(renderer, &width, &height);
	}

	if (width > 0 && height > 0)
	{
		WindowWidth = width;
		WindowHeight = height;
		camera.w = WindowWidth;
		camera.h = WindowHeight;

		if (ImGui::GetCurrentContext())
		{
			ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(WindowWidth), static_cast<float>(WindowHeight));
		}
	}
}

/**
 * Destroy method
 */
void Game::Destroy()
{
	AE::Logger::SaveLogToFile();

	assetManager.reset();
	registry.reset();
	eventBus.reset();

	if (ImGui::GetCurrentContext())
	{
		ImGuiSDL::Deinitialize();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
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

	TTF_Quit();
	SDL_Quit();
}
