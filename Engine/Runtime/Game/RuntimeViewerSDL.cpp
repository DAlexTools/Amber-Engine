#include "Game/RuntimeViewerSDL.h"

#include "Game/RuntimeRenderSystems.h"
#include "Logging/Logger.h"

#include <SDL2/SDL_image.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include <utility>

namespace AE
{
namespace
{
constexpr double Pi = 3.14159265358979323846;

void SetError(std::string* error, const std::string& message)
{
	if (error)
	{
		*error = message;
	}
}

SDL_Color ShapeFillColor(const Scene::ObjectData& object)
{
	return object.kind == Scene::ObjectKind::Circle ? SDL_Color{225, 142, 72, 120} : SDL_Color{78, 150, 204, 110};
}

SDL_Color ShapeOutlineColor(const Scene::ObjectData&)
{
	return SDL_Color{104, 184, 238, 230};
}

void SetDrawColor(SDL_Renderer* renderer, SDL_Color color)
{
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

double DegreesToRadians(double Degrees)
{
	return Degrees * Pi / 180.0;
}

SDL_FPoint RotatePoint(SDL_FPoint Point, SDL_FPoint Center, double RotationDegrees)
{
	const double Radians = DegreesToRadians(RotationDegrees);
	const float SinValue = static_cast<float>(std::sin(Radians));
	const float CosValue = static_cast<float>(std::cos(Radians));
	const float LocalX = Point.x - Center.x;
	const float LocalY = Point.y - Center.y;
	return SDL_FPoint{
		Center.x + LocalX * CosValue - LocalY * SinValue,
		Center.y + LocalX * SinValue + LocalY * CosValue};
}

std::array<SDL_FPoint, 4> RotatedRectPoints(const SDL_Rect& Rect, double RotationDegrees)
{
	const SDL_FPoint Center{
		static_cast<float>(Rect.x) + static_cast<float>(Rect.w) * 0.5f,
		static_cast<float>(Rect.y) + static_cast<float>(Rect.h) * 0.5f};
	std::array<SDL_FPoint, 4> Points = {
		SDL_FPoint{static_cast<float>(Rect.x), static_cast<float>(Rect.y)},
		SDL_FPoint{static_cast<float>(Rect.x + Rect.w), static_cast<float>(Rect.y)},
		SDL_FPoint{static_cast<float>(Rect.x + Rect.w), static_cast<float>(Rect.y + Rect.h)},
		SDL_FPoint{static_cast<float>(Rect.x), static_cast<float>(Rect.y + Rect.h)}};

	for (SDL_FPoint& Point : Points)
	{
		Point = RotatePoint(Point, Center, RotationDegrees);
	}
	return Points;
}

void DrawLine(SDL_Renderer* Renderer, SDL_FPoint First, SDL_FPoint Second)
{
	SDL_RenderDrawLine(
		Renderer,
		static_cast<int>(std::round(First.x)),
		static_cast<int>(std::round(First.y)),
		static_cast<int>(std::round(Second.x)),
		static_cast<int>(std::round(Second.y)));
}

void DrawFilledRotatedRect(
	SDL_Renderer* Renderer,
	const SDL_Rect& Rect,
	double RotationDegrees,
	SDL_Color Fill,
	SDL_Color Outline)
{
	const std::array<SDL_FPoint, 4> Points = RotatedRectPoints(Rect, RotationDegrees);
	SDL_Vertex Vertices[4] = {
		SDL_Vertex{Points[0], Fill, SDL_FPoint{0.0f, 0.0f}},
		SDL_Vertex{Points[1], Fill, SDL_FPoint{1.0f, 0.0f}},
		SDL_Vertex{Points[2], Fill, SDL_FPoint{1.0f, 1.0f}},
		SDL_Vertex{Points[3], Fill, SDL_FPoint{0.0f, 1.0f}}};
	const int Indices[6] = {0, 1, 2, 0, 2, 3};
	SDL_RenderGeometry(Renderer, nullptr, Vertices, 4, Indices, 6);

	SetDrawColor(Renderer, Outline);
	DrawLine(Renderer, Points[0], Points[1]);
	DrawLine(Renderer, Points[1], Points[2]);
	DrawLine(Renderer, Points[2], Points[3]);
	DrawLine(Renderer, Points[3], Points[0]);
}

void DrawFilledCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius, SDL_Color fill, SDL_Color outline)
{
	if (radius <= 0)
	{
		return;
	}

	SetDrawColor(renderer, fill);
	for (int y = -radius; y <= radius; ++y)
	{
		const int x = static_cast<int>(std::sqrt(static_cast<double>(radius * radius - y * y)));
		SDL_RenderDrawLine(renderer, centerX - x, centerY + y, centerX + x, centerY + y);
	}

	SetDrawColor(renderer, outline);
	int x = radius - 1;
	int y = 0;
	int dx = 1;
	int dy = 1;
	int err = dx - (radius << 1);

	while (x >= y)
	{
		SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
		SDL_RenderDrawPoint(renderer, centerX + y, centerY + x);
		SDL_RenderDrawPoint(renderer, centerX - y, centerY + x);
		SDL_RenderDrawPoint(renderer, centerX - x, centerY + y);
		SDL_RenderDrawPoint(renderer, centerX - x, centerY - y);
		SDL_RenderDrawPoint(renderer, centerX - y, centerY - x);
		SDL_RenderDrawPoint(renderer, centerX + y, centerY - x);
		SDL_RenderDrawPoint(renderer, centerX + x, centerY - y);

		if (err <= 0)
		{
			++y;
			err += dy;
			dy += 2;
		}
		if (err > 0)
		{
			--x;
			dx += 2;
			err += dx - (radius << 1);
		}
	}
}

bool SceneHasVisibleGrid(const Scene::Document& scene)
{
	for (const Scene::ObjectData& object : scene.objects)
	{
		if (object.visible && object.kind == Scene::ObjectKind::Grid)
		{
			return true;
		}
	}
	return false;
}
} // namespace

bool ApplyRuntimeSceneCamera(const Scene::Document& scene, RuntimeSceneRendererConfig& config)
{
	for (const Scene::ObjectData& object : scene.objects)
	{
		if (object.visible && object.kind == Scene::ObjectKind::Camera)
		{
			config.cameraX = object.transform.position.x;
			config.cameraY = object.transform.position.y;
			return true;
		}
	}

	return false;
}

RuntimeSceneRendererConfig BuildRuntimeSceneRendererConfig(const ProjectDescriptor& project, const Scene::Document& scene)
{
	RuntimeSceneRendererConfig config;
	config.projectRoot = project.projectRoot;
	config.engineRoot = project.engineRoot;
	config.contentRoot = project.ResolveProjectPath(project.contentRoot);
	config.assetRoots = BuildRuntimeAssetRoots(RuntimeAssetResolverConfig{
		project.projectRoot,
		project.engineRoot,
		project.contentRoot,
		{}});
	config.cameraPolicy = RuntimeCameraPolicy::SceneCamera;
	config.zoom = 1.0f;
	ApplyRuntimeSceneCamera(scene, config);
	return config;
}

RuntimeSceneRendererSDL::RuntimeSceneRendererSDL(SDL_Renderer* value)
	: renderer(value)
	, textureCache(value)
{
}

RuntimeSceneRendererSDL::~RuntimeSceneRendererSDL()
{
	ClearTextureCache();
}

void RuntimeSceneRendererSDL::SetRenderer(SDL_Renderer* value)
{
	if (renderer != value)
	{
		ClearTextureCache();
		renderer = value;
		textureCache.SetRenderer(value);
	}
}

void RuntimeSceneRendererSDL::ClearTextureCache()
{
	textureCache.Clear();
}

void RuntimeSceneRendererSDL::RenderScene(
	const Scene::Document& scene,
	const RuntimeSceneRendererConfig& config,
	const SDL_Rect& viewport)
{
	if (!renderer)
	{
		return;
	}

	const RuntimeSceneRendererConfig effectiveConfig = ResolveEffectiveConfig(scene, config);
	RenderBackgroundAndGrid(scene, effectiveConfig, viewport);
	RenderDocumentObjects(scene, effectiveConfig, viewport);
}

void RuntimeSceneRendererSDL::RenderWorld(
	Registry& registry,
	const Scene::Document& scene,
	const RuntimeSceneRendererConfig& config,
	const SDL_Rect& viewport)
{
	if (!renderer)
	{
		return;
	}

	const RuntimeSceneRendererConfig effectiveConfig = ResolveEffectiveConfig(scene, config);
	RenderBackgroundAndGrid(scene, effectiveConfig, viewport);
	if (!RenderRegistryObjects(registry, effectiveConfig, viewport))
	{
		RenderDocumentObjects(scene, effectiveConfig, viewport);
	}
}

RuntimeSceneRendererConfig RuntimeSceneRendererSDL::ResolveEffectiveConfig(
	const Scene::Document& scene,
	const RuntimeSceneRendererConfig& config) const
{
	RuntimeSceneRendererConfig effectiveConfig = config;
	if (effectiveConfig.cameraPolicy == RuntimeCameraPolicy::SceneCamera)
	{
		ApplyRuntimeSceneCamera(scene, effectiveConfig);
	}
	return effectiveConfig;
}

void RuntimeSceneRendererSDL::RenderBackgroundAndGrid(
	const Scene::Document& scene,
	const RuntimeSceneRendererConfig& config,
	const SDL_Rect& viewport)
{
	SetDrawColor(renderer, SDL_Color{16, 18, 20, 255});
	SDL_RenderFillRect(renderer, &viewport);

	if (config.showGrid && SceneHasVisibleGrid(scene))
	{
		const float gridStep = std::max(4.0f, 32.0f * config.zoom);
		const float centerX = viewport.x + viewport.w * 0.5f;
		const float centerY = viewport.y + viewport.h * 0.5f;
		const float originX = centerX - config.cameraX * config.zoom;
		const float originY = centerY - config.cameraY * config.zoom;

		SetDrawColor(renderer, SDL_Color{58, 65, 72, 120});
		for (float x = std::fmod(originX - viewport.x, gridStep); x < viewport.w; x += gridStep)
		{
			const int lineX = static_cast<int>(std::round(viewport.x + x));
			SDL_RenderDrawLine(renderer, lineX, viewport.y, lineX, viewport.y + viewport.h);
		}
		for (float y = std::fmod(originY - viewport.y, gridStep); y < viewport.h; y += gridStep)
		{
			const int lineY = static_cast<int>(std::round(viewport.y + y));
			SDL_RenderDrawLine(renderer, viewport.x, lineY, viewport.x + viewport.w, lineY);
		}

		SetDrawColor(renderer, SDL_Color{132, 74, 74, 170});
		const int axisY = static_cast<int>(std::round(originY));
		SDL_RenderDrawLine(renderer, viewport.x, axisY, viewport.x + viewport.w, axisY);
		SetDrawColor(renderer, SDL_Color{75, 126, 86, 170});
		const int axisX = static_cast<int>(std::round(originX));
		SDL_RenderDrawLine(renderer, axisX, viewport.y, axisX, viewport.y + viewport.h);
	}
}

void RuntimeSceneRendererSDL::RenderDocumentObjects(
	const Scene::Document& scene,
	const RuntimeSceneRendererConfig& config,
	const SDL_Rect& viewport)
{
	for (const Scene::ObjectData& object : scene.objects)
	{
		if (!object.visible ||
			object.kind == Scene::ObjectKind::Grid ||
			object.kind == Scene::ObjectKind::Camera)
		{
			continue;
		}

		const SDL_Rect rect = GetObjectRect(object, viewport, config);
		if (rect.w <= 0 || rect.h <= 0)
		{
			continue;
		}

		if (object.kind == Scene::ObjectKind::AssetInstance && !object.assetId.empty())
		{
			SDL_Texture* texture = GetTexture(object.assetId, config);
			if (texture)
			{
				const SDL_Point center{rect.w / 2, rect.h / 2};
				SDL_RenderCopyEx(renderer, texture, nullptr, &rect, object.transform.rotationDegrees, &center, SDL_FLIP_NONE);
				continue;
			}
		}

		if (object.kind == Scene::ObjectKind::Circle)
		{
			DrawFilledCircle(
				renderer,
				rect.x + rect.w / 2,
				rect.y + rect.h / 2,
				std::max(1, std::min(rect.w, rect.h) / 2),
				ShapeFillColor(object),
				ShapeOutlineColor(object));
		}
		else
		{
			SDL_Color fill = ShapeFillColor(object);
			SDL_Color outline = ShapeOutlineColor(object);
			DrawFilledRotatedRect(renderer, rect, object.transform.rotationDegrees, fill, outline);
		}
	}
}

bool RuntimeSceneRendererSDL::RenderRegistryObjects(
	Registry& registry,
	const RuntimeSceneRendererConfig& config,
	const SDL_Rect& viewport)
{
	bool renderedAny = false;

	if (registry.HasSystem<RuntimeSceneShapeRenderSystem>())
	{
		RuntimeSceneShapeRenderSystem& shapeSystem = registry.GetSystem<RuntimeSceneShapeRenderSystem>();
		for (const Entity& entity : shapeSystem.GetSystemEntity())
		{
			const AE::Scene::SceneObjectComponent& object = entity.GetComponent<AE::Scene::SceneObjectComponent>();
			if (!object.visible)
			{
				continue;
			}

			const TransformComponent& transform = entity.GetComponent<TransformComponent>();
			const AE::Scene::SceneShapeComponent& shape = entity.GetComponent<AE::Scene::SceneShapeComponent>();
			const SDL_Rect rect = GetSceneEntityRect(transform, shape.width, shape.height, viewport, config);
			if (rect.w <= 0 || rect.h <= 0)
			{
				continue;
			}

			Scene::ObjectData objectData;
			objectData.className = object.className;
			objectData.kind = shape.shapeType == AE::Scene::SceneShapeType::Circle ? Scene::ObjectKind::Circle : Scene::ObjectKind::Box;

			if (shape.shapeType == AE::Scene::SceneShapeType::Circle)
			{
				DrawFilledCircle(
					renderer,
					rect.x + rect.w / 2,
					rect.y + rect.h / 2,
					std::max(1, std::min(rect.w, rect.h) / 2),
					ShapeFillColor(objectData),
					ShapeOutlineColor(objectData));
			}
			else
			{
				const SDL_Color fill = ShapeFillColor(objectData);
				const SDL_Color outline = ShapeOutlineColor(objectData);
				DrawFilledRotatedRect(renderer, rect, transform.rotation, fill, outline);
			}

			renderedAny = true;
		}
	}

	if (registry.HasSystem<RuntimeSceneSpriteRenderSystem>())
	{
		RuntimeSceneSpriteRenderSystem& spriteSystem = registry.GetSystem<RuntimeSceneSpriteRenderSystem>();
		for (const Entity& entity : spriteSystem.GetSystemEntity())
		{
			const AE::Scene::SceneObjectComponent& object = entity.GetComponent<AE::Scene::SceneObjectComponent>();
			const AE::Scene::SceneSpriteComponent& sprite = entity.GetComponent<AE::Scene::SceneSpriteComponent>();
			if (!object.visible || sprite.assetId.empty())
			{
				continue;
			}

			SDL_Texture* texture = GetTexture(sprite.assetId, config);
			if (!texture)
			{
				continue;
			}

			const TransformComponent& transform = entity.GetComponent<TransformComponent>();
			const SDL_Rect rect = GetSceneEntityRect(transform, sprite.width, sprite.height, viewport, config);
			if (rect.w <= 0 || rect.h <= 0)
			{
				continue;
			}

			const SDL_Point center{rect.w / 2, rect.h / 2};
			SDL_RenderCopyEx(renderer, texture, nullptr, &rect, transform.rotation, &center, SDL_FLIP_NONE);
			renderedAny = true;
		}
	}

	if (registry.HasSystem<RuntimeLegacySpriteRenderSystem>())
	{
		struct LegacyRenderable
		{
			TransformComponent transform;
			SpriteComponent sprite;
		};

		std::vector<LegacyRenderable> renderables;
		RuntimeLegacySpriteRenderSystem& legacySpriteSystem = registry.GetSystem<RuntimeLegacySpriteRenderSystem>();
		for (const Entity& entity : legacySpriteSystem.GetSystemEntity())
		{
			renderables.push_back(LegacyRenderable{
				entity.GetComponent<TransformComponent>(),
				entity.GetComponent<SpriteComponent>()});
		}

		std::sort(renderables.begin(), renderables.end(), [](const LegacyRenderable& left, const LegacyRenderable& right)
				  { return left.sprite.zIndex < right.sprite.zIndex; });

		for (const LegacyRenderable& renderable : renderables)
		{
			if (renderable.sprite.assetID.empty())
			{
				continue;
			}

			SDL_Texture* texture = GetTexture(renderable.sprite.assetID, config);
			if (!texture)
			{
				continue;
			}

			const SDL_Rect rect = GetLegacySpriteRect(renderable.transform, renderable.sprite, viewport, config);
			if (rect.w <= 0 || rect.h <= 0)
			{
				continue;
			}

			const SDL_Rect srcRect = renderable.sprite.srcRect;
			SDL_RenderCopyEx(
				renderer,
				texture,
				&srcRect,
				&rect,
				renderable.transform.rotation,
				nullptr,
				renderable.sprite.flip);
			renderedAny = true;
		}
	}

	return renderedAny;
}

SDL_Texture* RuntimeSceneRendererSDL::GetTexture(const std::string& assetId, const RuntimeSceneRendererConfig& config)
{
	RuntimeAssetResolverConfig resolverConfig;
	resolverConfig.ProjectRoot = config.projectRoot;
	resolverConfig.EngineRoot = config.engineRoot;
	resolverConfig.ContentRoot = config.contentRoot;
	resolverConfig.Roots = config.assetRoots;
	textureCache.SetResolverConfig(std::move(resolverConfig));

	RuntimeTextureSDL* texture = textureCache.GetTexture(assetId);
	return texture ? texture->texture : nullptr;
}

SDL_Rect RuntimeSceneRendererSDL::GetObjectRect(
	const Scene::ObjectData& object,
	const SDL_Rect& viewport,
	const RuntimeSceneRendererConfig& config) const
{
	const Scene::Vec2 center = object.transform.position;
	const SDL_Point screen = WorldToScreen(center, viewport, config);
	const int width = std::max(1, static_cast<int>(std::round(object.size.x * object.transform.scale.x * config.zoom)));
	const int height = std::max(1, static_cast<int>(std::round(object.size.y * object.transform.scale.y * config.zoom)));
	return SDL_Rect{screen.x - width / 2, screen.y - height / 2, width, height};
}

SDL_Rect RuntimeSceneRendererSDL::GetSceneEntityRect(
	const TransformComponent& transform,
	float width,
	float height,
	const SDL_Rect& viewport,
	const RuntimeSceneRendererConfig& config) const
{
	const SDL_Point screen = WorldToScreen(
		Scene::Vec2{transform.position.x, transform.position.y},
		viewport,
		config);
	const int scaledWidth = std::max(1, static_cast<int>(std::round(width * transform.scale.x * config.zoom)));
	const int scaledHeight = std::max(1, static_cast<int>(std::round(height * transform.scale.y * config.zoom)));
	return SDL_Rect{screen.x - scaledWidth / 2, screen.y - scaledHeight / 2, scaledWidth, scaledHeight};
}

SDL_Rect RuntimeSceneRendererSDL::GetLegacySpriteRect(
	const TransformComponent& transform,
	const SpriteComponent& sprite,
	const SDL_Rect& viewport,
	const RuntimeSceneRendererConfig& config) const
{
	const int width = std::max(1, static_cast<int>(std::round(sprite.width * transform.scale.x * config.zoom)));
	const int height = std::max(1, static_cast<int>(std::round(sprite.height * transform.scale.y * config.zoom)));
	if (sprite.isFixed)
	{
		return SDL_Rect{
			viewport.x + static_cast<int>(std::round(transform.position.x)),
			viewport.y + static_cast<int>(std::round(transform.position.y)),
			width,
			height};
	}

	const SDL_Point topLeft = WorldToScreen(
		Scene::Vec2{transform.position.x, transform.position.y},
		viewport,
		config);
	return SDL_Rect{topLeft.x, topLeft.y, width, height};
}

SDL_Point RuntimeSceneRendererSDL::WorldToScreen(
	const Scene::Vec2& point,
	const SDL_Rect& viewport,
	const RuntimeSceneRendererConfig& config) const
{
	const float centerX = viewport.x + viewport.w * 0.5f;
	const float centerY = viewport.y + viewport.h * 0.5f;
	return SDL_Point{
		static_cast<int>(std::round(centerX + (point.x - config.cameraX) * config.zoom)),
		static_cast<int>(std::round(centerY + (point.y - config.cameraY) * config.zoom))};
}

RuntimeViewerSDL::~RuntimeViewerSDL()
{
	Shutdown();
}

bool RuntimeViewerSDL::Initialize(const RuntimeViewerSDLConfig& config, std::string* error)
{
	width = std::max(320, config.width);
	height = std::max(240, config.height);
	fullscreen = config.startFullscreen;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
	{
		SetError(error, std::string("SDL_Init failed: ") + SDL_GetError());
		return false;
	}
	sdlInitialized = true;

	const int imageFlags = IMG_INIT_PNG | IMG_INIT_JPG;
	const int initializedImageFlags = IMG_Init(imageFlags);
	imageInitialized = initializedImageFlags != 0;
	if ((initializedImageFlags & IMG_INIT_PNG) == 0)
	{
		AE::Logger::Warn(std::string("SDL_image PNG loader is unavailable: ") + IMG_GetError());
	}

	window = SDL_CreateWindow(
		config.windowTitle.empty() ? "Amber Runtime" : config.windowTitle.c_str(),
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		width,
		height,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window)
	{
		SetError(error, std::string("SDL_CreateWindow failed: ") + SDL_GetError());
		Shutdown();
		return false;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer)
	{
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	}
	if (!renderer)
	{
		SetError(error, std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
		Shutdown();
		return false;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	sceneRenderer.SetRenderer(renderer);

	if (fullscreen)
	{
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}

	return true;
}

void RuntimeViewerSDL::Shutdown()
{
	sceneRenderer.ClearTextureCache();

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
	if (imageInitialized)
	{
		IMG_Quit();
		imageInitialized = false;
	}
	if (sdlInitialized)
	{
		SDL_Quit();
		sdlInitialized = false;
	}
}

bool RuntimeViewerSDL::PollEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT)
		{
			return false;
		}
		if (event.type == SDL_KEYDOWN)
		{
			if (event.key.keysym.sym == SDLK_ESCAPE)
			{
				return false;
			}
			if (event.key.keysym.sym == SDLK_F11)
			{
				ToggleFullscreen();
			}
		}
	}
	return true;
}

void RuntimeViewerSDL::BeginFrame()
{
	if (!renderer)
	{
		return;
	}

	SetDrawColor(renderer, SDL_Color{16, 18, 20, 255});
	SDL_RenderClear(renderer);
}

void RuntimeViewerSDL::RenderScene(const Scene::Document& scene, const RuntimeSceneRendererConfig& config)
{
	sceneRenderer.RenderScene(scene, config, GetViewport());
}

void RuntimeViewerSDL::RenderWorld(Registry& registry, const Scene::Document& scene, const RuntimeSceneRendererConfig& config)
{
	sceneRenderer.RenderWorld(registry, scene, config, GetViewport());
}

void RuntimeViewerSDL::Present()
{
	if (renderer)
	{
		SDL_RenderPresent(renderer);
	}
}

RuntimeRenderContextSDL RuntimeViewerSDL::MakeRenderContext(
	const ProjectDescriptor& project,
	const Scene::Document& scene) const
{
	return RuntimeRenderContextSDL{window, renderer, GetViewport(), &project, &scene};
}

SDL_Window* RuntimeViewerSDL::GetWindow() const
{
	return window;
}

SDL_Renderer* RuntimeViewerSDL::GetRenderer() const
{
	return renderer;
}

SDL_Rect RuntimeViewerSDL::GetViewport() const
{
	int outputWidth = width;
	int outputHeight = height;
	if (renderer)
	{
		SDL_GetRendererOutputSize(renderer, &outputWidth, &outputHeight);
	}
	return SDL_Rect{0, 0, std::max(1, outputWidth), std::max(1, outputHeight)};
}

void RuntimeViewerSDL::ToggleFullscreen()
{
	if (!window)
	{
		return;
	}

	fullscreen = !fullscreen;
	SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

} // namespace AE
