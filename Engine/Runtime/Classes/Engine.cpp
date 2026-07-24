#include "Engine.h"

#include <string>
#include <SDL2/SDL_ttf.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_sdl.h>

#include "../Logging/Logger.h"
#include "../Systems/AnimationSystem.h"
#include "../Systems/CameraMovementSystem.h"
#include "../Systems/CollisionSystem.h"
#include "../Systems/DamageSystem.h"
#include "../Systems/KeyboardControlSystem.h"
#include "../Systems/MovementSystem.h"
#include "../Systems/ProjectileEmitterSystem.h"
#include "../Systems/ProjectileLifeCycleSystem.h"
#include "../Systems/RenderColliderSystem.h"
#include "../Systems/RenderGUISystem.h"
#include "../Systems/RenderHealthBarSystem.h"
#include "../Systems/RenderSystem.h"
#include "../Systems/RenderTextSystem.h"
#include "../EnginePhysicsBridge/EnginePhysicsBridge.h"
#include "../Systems/PhysicsContactResponseSystem.h"
#include "../Systems/PhysicsDebugRenderSystem.h"

namespace AE
{
	int Engine::MapWidth = 0;
	int Engine::MapHeight = 0;
	int Engine::WindowWidth = 0;
	int Engine::WindowHeight = 0;

	Engine::Engine()
	{
		registry = std::make_unique<Registry>();
		assetManager = std::make_unique<AssetManager>();
		eventBus = std::make_unique<EventBus>();
		AE::Logger::Log("Engine constructor called");
	}

	Engine::~Engine()
	{
		Shutdown();
		AE::Logger::Log("Engine destructor called");
	}

	bool Engine::Initialize(const EngineConfig& engineConfig)
	{
		config = engineConfig;
		isFullscreen = config.startFullscreen;
		AE::Logger::Log("Initialize AmberEngine");

		if (!registry)
		{
			registry = std::make_unique<Registry>();
		}
		if (!assetManager)
		{
			assetManager = std::make_unique<AssetManager>();
		}
		if (!eventBus)
		{
			eventBus = std::make_unique<EventBus>();
		}

		if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
		{
			AE::Logger::Err(std::string("Error initialize SDL: ") + SDL_GetError());
			return false;
		}
		sdlInitialized = true;

		if (TTF_Init() != 0)
		{
			AE::Logger::Err(std::string("Error initialize SDL TTF: ") + TTF_GetError());
			Shutdown();
			return false;
		}
		ttfInitialized = true;

		WindowWidth = config.windowedWidth;
		WindowHeight = config.windowedHeight;

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
			config.windowTitle,
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			WindowWidth,
			WindowHeight,
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

		if (!window)
		{
			AE::Logger::Err(std::string("Error creating SDL Window: ") + SDL_GetError());
			Shutdown();
			return false;
		}

		renderer = SDL_CreateRenderer(window, -1, 0);
		if (!renderer)
		{
			AE::Logger::Err(std::string("Error creating SDL renderer: ") + SDL_GetError());
			Shutdown();
			return false;
		}

		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGui_ImplSDL2_InitForD3D(window);
		ImGuiSDL::Initialize(renderer, WindowWidth, WindowHeight);
		imguiInitialized = true;

		camera.x = 0;
		camera.y = 0;
		camera.w = WindowWidth;
		camera.h = WindowHeight;

		InitializeSystems();
		ApplyFullscreenMode();
		isRunning = true;
		return true;
	}

	void Engine::InitializeSystems()
	{
		if (systemsInitialized || !registry)
		{
			return;
		}

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
		systemsInitialized = true;
	}

	void Engine::Shutdown()
	{
		if (!isRunning && !window && !renderer && !registry && !assetManager && !eventBus)
		{
			return;
		}

		AE::Logger::SaveLogToFile();
		isRunning = false;
		systemsInitialized = false;

		assetManager.reset();
		registry.reset();
		eventBus.reset();

		if (imguiInitialized && ImGui::GetCurrentContext())
		{
			ImGuiSDL::Deinitialize();
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext();
		}
		imguiInitialized = false;

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

		if (ttfInitialized)
		{
			TTF_Quit();
			ttfInitialized = false;
		}
		if (sdlInitialized)
		{
			SDL_Quit();
			sdlInitialized = false;
		}
	}

	bool Engine::IsRunning() const
	{
		return isRunning;
	}

	void Engine::RequestShutdown()
	{
		isRunning = false;
	}

	void Engine::SetFullscreenEnabled(bool enabled)
	{
		isFullscreen = enabled;
		if (window)
		{
			ApplyFullscreenMode();
		}
	}

	bool Engine::IsFullscreenEnabled() const
	{
		return isFullscreen;
	}

	void Engine::ToggleFullscreen()
	{
		SetFullscreenEnabled(!isFullscreen);
	}

	void Engine::UpdateWindowDimensions()
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

	Registry& Engine::GetRegistry()
	{
		return *registry;
	}

	const Registry& Engine::GetRegistry() const
	{
		return *registry;
	}

	std::unique_ptr<Registry>& Engine::GetRegistryHandle()
	{
		return registry;
	}

	const std::unique_ptr<Registry>& Engine::GetRegistryHandle() const
	{
		return registry;
	}

	AssetManager& Engine::GetAssetManager()
	{
		return *assetManager;
	}

	const AssetManager& Engine::GetAssetManager() const
	{
		return *assetManager;
	}

	std::unique_ptr<AssetManager>& Engine::GetAssetManagerHandle()
	{
		return assetManager;
	}

	const std::unique_ptr<AssetManager>& Engine::GetAssetManagerHandle() const
	{
		return assetManager;
	}

	EventBus& Engine::GetEventBus()
	{
		return *eventBus;
	}

	const EventBus& Engine::GetEventBus() const
	{
		return *eventBus;
	}

	std::unique_ptr<EventBus>& Engine::GetEventBusHandle()
	{
		return eventBus;
	}

	const std::unique_ptr<EventBus>& Engine::GetEventBusHandle() const
	{
		return eventBus;
	}

	SDL_Window* Engine::GetWindow() const
	{
		return window;
	}

	SDL_Renderer* Engine::GetRenderer() const
	{
		return renderer;
	}

	SDL_Rect& Engine::GetCamera()
	{
		return camera;
	}

	const SDL_Rect& Engine::GetCamera() const
	{
		return camera;
	}

	void Engine::ApplyFullscreenMode()
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
			SDL_SetWindowSize(window, config.windowedWidth, config.windowedHeight);
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		}

		UpdateWindowDimensions();
		AE::Logger::Log(std::string("Window mode changed to ") + (isFullscreen ? "fullscreen" : "windowed"));
	}
}
