#ifndef AMBER_EDITOR_SHELL_EDITOR_VIEWPORT_H
#define AMBER_EDITOR_SHELL_EDITOR_VIEWPORT_H

#include "Game/RuntimeViewerSDL.h"
#include "Project/ProjectDescriptor.h"
#include "SceneDocument.h"

#include <SDL2/SDL.h>

#include <functional>
#include <optional>
#include <string>

class Registry;

namespace AE::Editor
{

class AssetRegistry;
class FActorTypeRegistry;
class SelectionService;
class TextureCache;

enum class EditorTool
{
	Select,
	Move,
	Rotate,
	Scale
};

class EditorViewport
{
public:
	~EditorViewport();

	struct ObjectBounds
	{
		float x = 0.0f;
		float y = 0.0f;
		float w = 0.0f;
		float h = 0.0f;
	};

	enum class EDropPayloadType
	{
		Asset,
		ActorType
	};

	struct FDropRequest
	{
		EDropPayloadType Type = EDropPayloadType::Asset;
		std::string PayloadId;
		EditorVec2 WorldPosition;
	};

	enum class EContextMenuTarget
	{
		Scene,
		SceneObject
	};

	struct FContextMenuRequest
	{
		EContextMenuTarget Target = EContextMenuTarget::Scene;
		uint32 ObjectId = 0;
		EditorVec2 WorldPosition;
	};

	enum class ViewportMode
	{
		EditPreview,
		PlayOutput
	};

	using RuntimeRenderCallback = std::function<void(RuntimeRenderContextSDL&)>;

	std::optional<FDropRequest> Draw(
		SceneDocument& sceneDocument,
		SelectionService& selection,
		const AssetRegistry& assetRegistry,
		TextureCache& textureCache,
		EditorTool activeTool,
		SDL_Window* window,
		SDL_Renderer* renderer,
		const ProjectDescriptor* activeProject,
		const FActorTypeRegistry* actorTypeRegistry,
		ViewportMode mode,
		bool paused,
		Registry* runtimeRegistry = nullptr,
		const RuntimeRenderCallback& runtimeRenderCallback = {});

	float GetZoom() const;
	void SetZoom(float value);
	void FocusOrigin();
	void FocusObject(const SceneObject& Object);
	EditorVec2 GetViewCenter() const;
	std::optional<FContextMenuRequest> ConsumeContextMenuRequest();
	void ReleaseRenderResources();

private:
	float cameraX = 0.0f;
	float cameraY = 0.0f;
	float zoom = 1.0f;
	bool showGrid = true;
	bool panning = false;
	EditorVec2 panStartMouseScreen;
	EditorVec2 panStartCamera;

	enum class GizmoAxis
	{
		None,
		X,
		Y,
		XY
	};

	GizmoAxis activeGizmoAxis = GizmoAxis::None;
	uint32 activeGizmoObjectId = 0;
	EditorVec2 dragStartMouseWorld;
	EditorVec2 dragStartObjectPosition;
	GizmoAxis ActiveScaleGizmoAxis = GizmoAxis::None;
	uint32 ActiveScaleGizmoObjectId = 0;
	EditorVec2 DragStartScaleMouseWorld;
	EditorVec2 DragStartObjectScale{1.0f, 1.0f};
	bool RotatingObject = false;
	uint32 RotatingObjectId = 0;
	float DragStartRotationDegrees = 0.0f;
	float DragStartMouseAngleDegrees = 0.0f;
	std::optional<FContextMenuRequest> PendingContextMenuRequest;

	ObjectBounds GetObjectBounds(const SceneObject& object) const;
	bool EnsureRuntimePreviewTexture(SDL_Renderer* renderer, int width, int height);
	void DestroyRuntimePreviewTexture();

	RuntimeSceneRendererSDL runtimeSceneRenderer;
	SDL_Texture* runtimePreviewTexture = nullptr;
	int runtimePreviewWidth = 0;
	int runtimePreviewHeight = 0;
};

} // namespace AE::Editor

#endif
