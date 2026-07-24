#ifndef ENGINE_RUNTIME_CLASSES_ENGINE_H
#define ENGINE_RUNTIME_CLASSES_ENGINE_H

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif

#include <memory>
#include <SDL2/SDL.h>

#include "../AssetManager/AssetManager.h"
#include "../EntityComponentSystem/ECS.h"
#include "../EventBus/EventBus.h"

namespace AE
{
	struct EngineConfig
	{
		const char* windowTitle = "AmberEngine";
		int windowedWidth = 1280;
		int windowedHeight = 720;
		bool startFullscreen = true;
	};

	class Engine
	{
	public:
		Engine();
		~Engine();

		bool Initialize(const EngineConfig& config = EngineConfig{});
		void InitializeSystems();
		void Shutdown();

		bool IsRunning() const;
		void RequestShutdown();

		void SetFullscreenEnabled(bool enabled);
		bool IsFullscreenEnabled() const;
		void ToggleFullscreen();
		void UpdateWindowDimensions();

		Registry& GetRegistry();
		const Registry& GetRegistry() const;
		std::unique_ptr<Registry>& GetRegistryHandle();
		const std::unique_ptr<Registry>& GetRegistryHandle() const;

		AssetManager& GetAssetManager();
		const AssetManager& GetAssetManager() const;
		std::unique_ptr<AssetManager>& GetAssetManagerHandle();
		const std::unique_ptr<AssetManager>& GetAssetManagerHandle() const;

		EventBus& GetEventBus();
		const EventBus& GetEventBus() const;
		std::unique_ptr<EventBus>& GetEventBusHandle();
		const std::unique_ptr<EventBus>& GetEventBusHandle() const;

		SDL_Window* GetWindow() const;
		SDL_Renderer* GetRenderer() const;
		SDL_Rect& GetCamera();
		const SDL_Rect& GetCamera() const;

		static int MapWidth;
		static int MapHeight;
		static int WindowWidth;
		static int WindowHeight;

	private:
		void ApplyFullscreenMode();

		bool isRunning = false;
		bool isFullscreen = true;
		bool sdlInitialized = false;
		bool ttfInitialized = false;
		bool imguiInitialized = false;
		bool systemsInitialized = false;

		EngineConfig config;
		SDL_Window* window = nullptr;
		SDL_Renderer* renderer = nullptr;
		SDL_Rect camera{};

		std::unique_ptr<Registry> registry = nullptr;
		std::unique_ptr<AssetManager> assetManager = nullptr;
		std::unique_ptr<EventBus> eventBus = nullptr;
	};
}

#endif
