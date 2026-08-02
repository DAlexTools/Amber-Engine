#include "Viewport/EditorViewport.h"

#include "Actors/ActorTypeRegistry.h"
#include "Assets/AssetRegistry.h"
#include "Selection/SelectionService.h"
#include "Assets/TextureCache.h"
#include "imgui.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace AE::Editor
{
namespace
{
constexpr float Pi = 3.14159265358979323846f;

bool Contains(float x, float y, const EditorViewport::ObjectBounds& bounds)
{
	return x >= bounds.X && x <= bounds.X + bounds.W &&
		   y >= bounds.Y && y <= bounds.Y + bounds.H;
}

float DegreesToRadians(float Degrees)
{
	return Degrees * Pi / 180.0f;
}

float MouseAngleDegrees(const ImVec2& Center, const ImVec2& Mouse)
{
	return std::atan2(Mouse.y - Center.y, Mouse.x - Center.x) * 180.0f / Pi;
}

float NormalizeDegrees(float Degrees)
{
	float Result = std::fmod(Degrees, 360.0f);
	if (Result < 0.0f)
	{
		Result += 360.0f;
	}
	return Result;
}

ImVec2 RotatePoint(const ImVec2& Point, const ImVec2& Center, float RotationDegrees)
{
	const float Radians = DegreesToRadians(RotationDegrees);
	const float SinValue = std::sin(Radians);
	const float CosValue = std::cos(Radians);
	const float LocalX = Point.x - Center.x;
	const float LocalY = Point.y - Center.y;
	return ImVec2{
		Center.x + LocalX * CosValue - LocalY * SinValue,
		Center.y + LocalX * SinValue + LocalY * CosValue};
}

std::array<ImVec2, 4> RotatedQuad(const EditorViewport::ObjectBounds& Bounds, float RotationDegrees)
{
	const ImVec2 Center(Bounds.X + Bounds.W * 0.5f, Bounds.Y + Bounds.H * 0.5f);
	std::array<ImVec2, 4> Points = {
		ImVec2(Bounds.X, Bounds.Y),
		ImVec2(Bounds.X + Bounds.W, Bounds.Y),
		ImVec2(Bounds.X + Bounds.W, Bounds.Y + Bounds.H),
		ImVec2(Bounds.X, Bounds.Y + Bounds.H)};

	for (ImVec2& Point : Points)
	{
		Point = RotatePoint(Point, Center, RotationDegrees);
	}
	return Points;
}

bool ContainsRotatedBounds(const ImVec2& Mouse, const EditorViewport::ObjectBounds& Bounds, float RotationDegrees)
{
	const ImVec2 Center(Bounds.X + Bounds.W * 0.5f, Bounds.Y + Bounds.H * 0.5f);
	const ImVec2 UnrotatedMouse = RotatePoint(Mouse, Center, -RotationDegrees);
	return Contains(UnrotatedMouse.x, UnrotatedMouse.y, Bounds);
}

ImTextureID ToImTextureId(SDL_Texture* texture)
{
	return reinterpret_cast<ImTextureID>(texture);
}

bool IsShapeObject(const SceneObject& object)
{
	return object.kind == SceneObjectKind::Box || object.kind == SceneObjectKind::Circle;
}

constexpr float MinimumObjectScale = 0.05f;

float ClampObjectScale(float Value)
{
	return std::max(MinimumObjectScale, Value);
}

EditorVec2 ClampObjectScale(EditorVec2 Value)
{
	Value.x = ClampObjectScale(Value.x);
	Value.y = ClampObjectScale(Value.y);
	return Value;
}

bool CanScaleObject(const SceneObject& Object)
{
	return Object.kind == SceneObjectKind::AssetInstance ||
		   Object.kind == SceneObjectKind::Box ||
		   Object.kind == SceneObjectKind::Circle ||
		   Object.kind == SceneObjectKind::Empty;
}

ImVec2 Midpoint(const ImVec2& First, const ImVec2& Second)
{
	return ImVec2((First.x + Second.x) * 0.5f, (First.y + Second.y) * 0.5f);
}

bool ContainsHandle(const ImVec2& Mouse, const ImVec2& HandleCenter, float Radius)
{
	return std::fabs(Mouse.x - HandleCenter.x) <= Radius &&
		   std::fabs(Mouse.y - HandleCenter.y) <= Radius;
}

EditorVec2 RotateDeltaToLocal(EditorVec2 Delta, float RotationDegrees)
{
	const float Radians = DegreesToRadians(RotationDegrees);
	const float SinValue = std::sin(Radians);
	const float CosValue = std::cos(Radians);
	return EditorVec2{
		Delta.x * CosValue + Delta.y * SinValue,
		-Delta.x * SinValue + Delta.y * CosValue};
}

ImU32 PreviewColor(const FActorPreviewColor& Color)
{
	return IM_COL32(Color.R, Color.G, Color.B, Color.A);
}

ImU32 ShapeFillColor(const SceneObject& Object, bool Playing, const FActorTypeRegistry* ActorTypeRegistry)
{
	if (ActorTypeRegistry)
	{
		if (const FActorTypeDefinition* ActorType = ActorTypeRegistry->FindByClassName(Object.className))
		{
			FActorPreviewColor Color = ActorType->FillColor;
			if (Playing)
			{
				Color.A = static_cast<uint8>(std::min<int>(Color.A, 120));
			}
			return PreviewColor(Color);
		}
	}

	return Object.kind == SceneObjectKind::Circle ? IM_COL32(225, 142, 72, Playing ? 95 : 72) : IM_COL32(78, 150, 204, Playing ? 90 : 68);
}

ImU32 ShapeOutlineColor(
	const SceneObject& Object,
	bool Selected,
	bool Playing,
	const FActorTypeRegistry* ActorTypeRegistry)
{
	if (Selected)
	{
		return IM_COL32(255, 211, 91, 255);
	}

	if (ActorTypeRegistry)
	{
		if (const FActorTypeDefinition* ActorType = ActorTypeRegistry->FindByClassName(Object.className))
		{
			return PreviewColor(ActorType->OutlineColor);
		}
	}

	return Playing ? IM_COL32(102, 206, 138, 230) : IM_COL32(104, 184, 238, 230);
}

std::string PayloadString(const ImGuiPayload& Payload)
{
	if (!Payload.Data || Payload.DataSize <= 0)
	{
		return {};
	}

	std::string Value(static_cast<const char*>(Payload.Data), static_cast<SizeT>(Payload.DataSize));
	if (!Value.empty() && Value.back() == '\0')
	{
		Value.pop_back();
	}
	return Value;
}
} // namespace

std::optional<EditorViewport::FDropRequest> EditorViewport::Draw(
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
	Registry* runtimeRegistry,
	const RuntimeRenderCallback& runtimeRenderCallback)
{
	(void)assetRegistry;
	(void)textureCache;

	std::optional<FDropRequest> DropRequest;
	PendingContextMenuRequest.reset();
	const bool playing = mode == ViewportMode::PlayOutput;
	const bool editEnabled = mode == ViewportMode::EditPreview;

	if (editEnabled)
	{
		if (ImGui::Button("Focus Origin"))
		{
			FocusOrigin();
		}
		ImGui::SameLine();
		ImGui::Checkbox("Grid", &ShowGrid);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(160.0f);
		ImGui::SliderFloat("Zoom", &Zoom, 0.25f, 2.0f, "%.2fx");
	}

	ImVec2 canvasPos = ImGui::GetCursorScreenPos();
	ImVec2 canvasSize = ImGui::GetContentRegionAvail();
	canvasSize.x = std::max(1.0f, canvasSize.x);
	canvasSize.y = std::max(1.0f, canvasSize.y);
	ImGui::InvisibleButton("SceneCanvas", canvasSize);
	const bool canvasHovered = ImGui::IsItemHovered();

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 canvasEnd(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);
	const ImVec2 canvasCenter(canvasPos.x + canvasSize.x * 0.5f, canvasPos.y + canvasSize.y * 0.5f);
	auto worldToScreen = [&](EditorVec2 world)
	{
		return ImVec2(
			canvasCenter.x + (world.x - CameraX) * Zoom,
			canvasCenter.y + (world.y - CameraY) * Zoom);
	};
	auto screenToWorld = [&](ImVec2 screen)
	{
		return EditorVec2{ CameraX + (screen.x - canvasCenter.x) / Zoom, CameraY + (screen.y - canvasCenter.y) / Zoom};
	};
	auto boundsToScreen = [&](const ObjectBounds& bounds)
	{
		const ImVec2 topLeft = worldToScreen(EditorVec2{bounds.X, bounds.Y});
		return ObjectBounds{
			topLeft.x,
			topLeft.y,
			bounds.W * Zoom,
			bounds.H * Zoom};
	};

	bool canvasClickConsumed = false;
	const ImGuiIO& io = ImGui::GetIO();

	if (canvasHovered && io.MouseWheel != 0.0f)
	{
		const EditorVec2 mouseWorldBeforeZoom = screenToWorld(io.MousePos);
		SetZoom(Zoom * std::pow(1.12f, io.MouseWheel));
		const EditorVec2 mouseWorldAfterZoom = screenToWorld(io.MousePos);
		CameraX += mouseWorldBeforeZoom.x - mouseWorldAfterZoom.x;
		CameraY += mouseWorldBeforeZoom.y - mouseWorldAfterZoom.y;
	}

	if (canvasHovered && ImGui::IsMouseClicked(2))
	{
		Panning = true;
		PanStartMouseScreen = EditorVec2{io.MousePos.x, io.MousePos.y};
		PanStartCamera = EditorVec2{CameraX, CameraY};
		activeGizmoAxis = GizmoAxis::None;
		activeGizmoObjectId = 0;
		ActiveScaleGizmoAxis = GizmoAxis::None;
		ActiveScaleGizmoObjectId = 0;
	}

	if (Panning)
	{
		if (!ImGui::IsMouseDown(2))
		{
			Panning = false;
		}
		else
		{
			CameraX = PanStartCamera.x - (io.MousePos.x - PanStartMouseScreen.x) / Zoom;
			CameraY = PanStartCamera.y - (io.MousePos.y - PanStartMouseScreen.y) / Zoom;
			canvasClickConsumed = true;
		}
	}

	const int previewWidth = std::max(1, static_cast<int>(std::round(canvasSize.x)));
	const int previewHeight = std::max(1, static_cast<int>(std::round(canvasSize.y)));
	bool runtimeSceneRendered = false;
	if (renderer && activeProject && EnsureRuntimePreviewTexture(renderer, previewWidth, previewHeight))
	{
		SDL_Texture* previousTarget = SDL_GetRenderTarget(renderer);
		SDL_Rect previousViewport{};
		SDL_RenderGetViewport(renderer, &previousViewport);

		if (SDL_SetRenderTarget(renderer, runtimePreviewTexture) == 0)
		{
			SDL_RenderSetViewport(renderer, nullptr);
			RuntimeSceneRendererConfig rendererConfig;
			rendererConfig.projectRoot = activeProject->projectRoot;
			rendererConfig.engineRoot = activeProject->engineRoot;
			rendererConfig.contentRoot = activeProject->ResolveProjectPath(activeProject->contentRoot);
			rendererConfig.assetRoots = assetRegistry.GetRoots();
			rendererConfig.cameraPolicy = playing ? RuntimeCameraPolicy::SceneCamera : RuntimeCameraPolicy::Explicit;
			rendererConfig.cameraX = playing ? 0.0f : CameraX;
			rendererConfig.cameraY = playing ? 0.0f : CameraY;
			rendererConfig.zoom = playing ? 1.0f : Zoom;
			rendererConfig.showGrid = ShowGrid;

			const AE::Scene::Document runtimeDocument = sceneDocument.ToRuntimeDocument();
			if (runtimeRegistry)
			{
				runtimeSceneRenderer.RenderWorld(
					*runtimeRegistry,
					runtimeDocument,
					rendererConfig,
					SDL_Rect{0, 0, previewWidth, previewHeight});
			}
			else
			{
				runtimeSceneRenderer.RenderScene(
					runtimeDocument,
					rendererConfig,
					SDL_Rect{0, 0, previewWidth, previewHeight});
			}
			if (playing && runtimeRenderCallback)
			{
				RuntimeRenderContextSDL renderContext{
					window,
					renderer,
					SDL_Rect{0, 0, previewWidth, previewHeight},
					activeProject,
					&runtimeDocument};
				runtimeRenderCallback(renderContext);
			}
			runtimeSceneRendered = true;
		}

		SDL_SetRenderTarget(renderer, previousTarget);
		SDL_RenderSetViewport(renderer, &previousViewport);
	}

	drawList->PushClipRect(canvasPos, canvasEnd, true);
	if (runtimeSceneRendered)
	{
		drawList->AddImage(
			ToImTextureId(runtimePreviewTexture),
			canvasPos,
			canvasEnd);
	}
	else
	{
		drawList->AddRectFilled(canvasPos, canvasEnd, IM_COL32(16, 18, 20, 255));
	}

	if (!runtimeSceneRendered && ShowGrid)
	{
		const float gridStep = 32.0f * Zoom;
		const float originX = canvasCenter.x - CameraX * Zoom;
		const float originY = canvasCenter.y - CameraY * Zoom;
		const ImU32 gridColor = IM_COL32(58, 65, 72, 120);

		for (float x = std::fmod(originX - canvasPos.x, gridStep); x < canvasSize.x; x += gridStep)
		{
			const float lineX = canvasPos.x + x;
			drawList->AddLine(ImVec2(lineX, canvasPos.y), ImVec2(lineX, canvasEnd.y), gridColor);
		}
		for (float y = std::fmod(originY - canvasPos.y, gridStep); y < canvasSize.y; y += gridStep)
		{
			const float lineY = canvasPos.y + y;
			drawList->AddLine(ImVec2(canvasPos.x, lineY), ImVec2(canvasEnd.x, lineY), gridColor);
		}
	}

	if (!runtimeSceneRendered)
	{
		const ImVec2 origin = worldToScreen(EditorVec2{0.0f, 0.0f});
		drawList->AddLine(ImVec2(canvasPos.x, origin.y), ImVec2(canvasEnd.x, origin.y), IM_COL32(132, 74, 74, 170), 2.0f);
		drawList->AddLine(ImVec2(origin.x, canvasPos.y), ImVec2(origin.x, canvasEnd.y), IM_COL32(75, 126, 86, 170), 2.0f);
	}

	const bool drawSceneObjects = editEnabled || !runtimeSceneRendered;
	if (drawSceneObjects)
	{
		for (const SceneObject& object : sceneDocument.GetObjects())
		{
			if (!object.visible || object.kind == SceneObjectKind::Grid)
			{
				continue;
			}

			const ObjectBounds screenBounds = boundsToScreen(GetObjectBounds(object));
			const bool selected = editEnabled && selection.IsSceneObjectSelected(object.id);
			const ImU32 color = IsShapeObject(object) ? ShapeOutlineColor(object, selected, playing, actorTypeRegistry) : (selected ? IM_COL32(255, 211, 91, 255) : (playing ? IM_COL32(102, 206, 138, 220) : IM_COL32(92, 153, 214, 220)));
			const float thickness = selected ? 3.0f : 2.0f;
			const std::array<ImVec2, 4> ScreenQuad = RotatedQuad(screenBounds, object.transform.rotationDegrees);

			const AssetRecord* asset = object.assetId.empty() ? nullptr : assetRegistry.FindAssetById(object.assetId);
			TexturePreview* preview = asset ? textureCache.GetTexture(*asset) : nullptr;

			if (runtimeSceneRendered)
			{
				if (selected)
				{
					if (object.kind == SceneObjectKind::Circle)
					{
						drawList->AddCircle(
							ImVec2(screenBounds.X + screenBounds.W * 0.5f, screenBounds.Y + screenBounds.H * 0.5f),
							std::max(4.0f, std::min(screenBounds.W, screenBounds.H) * 0.5f),
							color,
							32,
							thickness);
					}
					else
					{
						drawList->AddQuad(
							ScreenQuad[0],
							ScreenQuad[1],
							ScreenQuad[2],
							ScreenQuad[3],
							color,
							thickness);
					}
				}
			}
			else if (preview && preview->texture)
			{
				drawList->AddImageQuad(
					ToImTextureId(preview->texture),
					ScreenQuad[0],
					ScreenQuad[1],
					ScreenQuad[2],
					ScreenQuad[3]);
				drawList->AddQuad(
					ScreenQuad[0],
					ScreenQuad[1],
					ScreenQuad[2],
					ScreenQuad[3],
					color,
					thickness);
			}
			else if (object.kind == SceneObjectKind::Camera)
			{
				drawList->AddCircle(
					ImVec2(screenBounds.X + screenBounds.W * 0.5f, screenBounds.Y + screenBounds.H * 0.5f),
					std::max(8.0f, 20.0f * Zoom),
					color,
					24,
					thickness);
			}
			else if (object.kind == SceneObjectKind::Circle)
			{
				const ImVec2 center(
					screenBounds.X + screenBounds.W * 0.5f,
					screenBounds.Y + screenBounds.H * 0.5f);
				const float radius = std::max(4.0f, std::min(screenBounds.W, screenBounds.H) * 0.5f);
				drawList->AddCircleFilled(center, radius, ShapeFillColor(object, playing, actorTypeRegistry), 32);
				drawList->AddCircle(center, radius, color, 32, thickness);
			}
			else if (object.kind == SceneObjectKind::Box)
			{
				drawList->AddQuadFilled(
					ScreenQuad[0],
					ScreenQuad[1],
					ScreenQuad[2],
					ScreenQuad[3],
					ShapeFillColor(object, playing, actorTypeRegistry));
				drawList->AddQuad(
					ScreenQuad[0],
					ScreenQuad[1],
					ScreenQuad[2],
					ScreenQuad[3],
					color,
					thickness);
			}
			else
			{
				drawList->AddQuad(
					ScreenQuad[0],
					ScreenQuad[1],
					ScreenQuad[2],
					ScreenQuad[3],
					color,
					thickness);
			}

			if (editEnabled)
			{
				drawList->AddText(
					ImVec2(screenBounds.X, screenBounds.Y - 18.0f),
					IM_COL32(210, 216, 222, 230),
					object.name.c_str());
			}
		}
	}

	const EditorSelection& currentSelection = selection.GetSelection();
	SceneObject* selectedObject = nullptr;
	if (currentSelection.type == EditorSelectionType::SceneObject)
	{
		selectedObject = sceneDocument.FindObject(currentSelection.objectId);
	}

	if (!editEnabled || activeTool != EditorTool::Move || !selectedObject || !selectedObject->visible || selectedObject->locked)
	{
		activeGizmoAxis = GizmoAxis::None;
		activeGizmoObjectId = 0;
	}
	else
	{
		const float axisLength = 82.0f;
		const float axisHitWidth = 9.0f;
		const ImVec2 mouse = ImGui::GetIO().MousePos;
		const ImVec2 center = worldToScreen(selectedObject->transform.position);

		auto hitGizmo = [&]()
		{
			if (std::fabs(mouse.x - center.x) <= 10.0f && std::fabs(mouse.y - center.y) <= 10.0f)
			{
				return GizmoAxis::XY;
			}
			if (mouse.x >= center.x + 8.0f && mouse.x <= center.x + axisLength + 16.0f &&
				std::fabs(mouse.y - center.y) <= axisHitWidth)
			{
				return GizmoAxis::X;
			}
			if (mouse.y >= center.y + 8.0f && mouse.y <= center.y + axisLength + 16.0f &&
				std::fabs(mouse.x - center.x) <= axisHitWidth)
			{
				return GizmoAxis::Y;
			}
			return GizmoAxis::None;
		};

		if (activeGizmoAxis == GizmoAxis::None && canvasHovered && ImGui::IsMouseClicked(0))
		{
			const GizmoAxis hitAxis = hitGizmo();
			if (hitAxis != GizmoAxis::None)
			{
				activeGizmoAxis = hitAxis;
				activeGizmoObjectId = selectedObject->id;
				dragStartMouseWorld = screenToWorld(mouse);
				dragStartObjectPosition = selectedObject->transform.position;
				canvasClickConsumed = true;
			}
		}

		if (activeGizmoAxis != GizmoAxis::None)
		{
			if (!ImGui::IsMouseDown(0))
			{
				activeGizmoAxis = GizmoAxis::None;
				activeGizmoObjectId = 0;
			}
			else if (activeGizmoObjectId == selectedObject->id)
			{
				const EditorVec2 mouseWorld = screenToWorld(mouse);
				const EditorVec2 delta{
					mouseWorld.x - dragStartMouseWorld.x,
					mouseWorld.y - dragStartMouseWorld.y};
				EditorVec2 position = dragStartObjectPosition;

				if (activeGizmoAxis == GizmoAxis::X || activeGizmoAxis == GizmoAxis::XY)
				{
					position.x += delta.x;
				}
				if (activeGizmoAxis == GizmoAxis::Y || activeGizmoAxis == GizmoAxis::XY)
				{
					position.y += delta.y;
				}

				selectedObject->transform.position = position;
				sceneDocument.SetDirty(true);
				canvasClickConsumed = true;
			}
			else
			{
				activeGizmoAxis = GizmoAxis::None;
				activeGizmoObjectId = 0;
			}
		}

		const ImVec2 updatedCenter = worldToScreen(selectedObject->transform.position);
		const ImVec2 xEnd(updatedCenter.x + axisLength, updatedCenter.y);
		const ImVec2 yEnd(updatedCenter.x, updatedCenter.y + axisLength);
		const bool xActive = activeGizmoAxis == GizmoAxis::X || activeGizmoAxis == GizmoAxis::XY;
		const bool yActive = activeGizmoAxis == GizmoAxis::Y || activeGizmoAxis == GizmoAxis::XY;
		const ImU32 xColor = xActive ? IM_COL32(255, 102, 102, 255) : IM_COL32(222, 72, 72, 255);
		const ImU32 yColor = yActive ? IM_COL32(102, 235, 142, 255) : IM_COL32(72, 190, 104, 255);
		const ImU32 centerColor = activeGizmoAxis == GizmoAxis::XY ? IM_COL32(255, 226, 96, 255) : IM_COL32(236, 196, 76, 255);

		drawList->AddLine(updatedCenter, xEnd, IM_COL32(24, 24, 24, 180), 6.0f);
		drawList->AddLine(updatedCenter, yEnd, IM_COL32(24, 24, 24, 180), 6.0f);
		drawList->AddLine(updatedCenter, xEnd, xColor, 3.0f);
		drawList->AddLine(updatedCenter, yEnd, yColor, 3.0f);
		drawList->AddTriangleFilled(
			ImVec2(xEnd.x + 12.0f, xEnd.y),
			ImVec2(xEnd.x, xEnd.y - 7.0f),
			ImVec2(xEnd.x, xEnd.y + 7.0f),
			xColor);
		drawList->AddTriangleFilled(
			ImVec2(yEnd.x, yEnd.y + 12.0f),
			ImVec2(yEnd.x - 7.0f, yEnd.y),
			ImVec2(yEnd.x + 7.0f, yEnd.y),
			yColor);
		drawList->AddRectFilled(
			ImVec2(updatedCenter.x - 7.0f, updatedCenter.y - 7.0f),
			ImVec2(updatedCenter.x + 7.0f, updatedCenter.y + 7.0f),
			centerColor,
			2.0f);
		drawList->AddRect(
			ImVec2(updatedCenter.x - 7.0f, updatedCenter.y - 7.0f),
			ImVec2(updatedCenter.x + 7.0f, updatedCenter.y + 7.0f),
			IM_COL32(20, 20, 20, 220),
			2.0f,
			0,
			1.0f);
		drawList->AddText(ImVec2(xEnd.x + 15.0f, xEnd.y - 9.0f), xColor, "X");
		drawList->AddText(ImVec2(yEnd.x - 4.0f, yEnd.y + 14.0f), yColor, "Y");
	}

	if (!editEnabled || activeTool != EditorTool::Rotate || !selectedObject || !selectedObject->visible || selectedObject->locked)
	{
		RotatingObject = false;
		RotatingObjectId = 0;
	}
	else
	{
		activeGizmoAxis = GizmoAxis::None;
		activeGizmoObjectId = 0;

		const ObjectBounds SelectedScreenBounds = boundsToScreen(GetObjectBounds(*selectedObject));
		const ImVec2 Center = worldToScreen(selectedObject->transform.position);
		const ImVec2 Mouse = ImGui::GetIO().MousePos;
		const float Radius = std::max(34.0f, std::max(SelectedScreenBounds.W, SelectedScreenBounds.H) * 0.62f);
		const float DistanceX = Mouse.x - Center.x;
		const float DistanceY = Mouse.y - Center.y;
		const float Distance = std::sqrt(DistanceX * DistanceX + DistanceY * DistanceY);
		const bool HitRotationRing = Distance >= Radius - 16.0f && Distance <= Radius + 16.0f;
		const bool HitSelectedObject = ContainsRotatedBounds(
			Mouse,
			SelectedScreenBounds,
			selectedObject->transform.rotationDegrees);

		if (!RotatingObject && canvasHovered && ImGui::IsMouseClicked(0) && (HitRotationRing || HitSelectedObject))
		{
			RotatingObject = true;
			RotatingObjectId = selectedObject->id;
			DragStartRotationDegrees = selectedObject->transform.rotationDegrees;
			DragStartMouseAngleDegrees = MouseAngleDegrees(Center, Mouse);
			canvasClickConsumed = true;
		}

		if (RotatingObject)
		{
			if (!ImGui::IsMouseDown(0))
			{
				RotatingObject = false;
				RotatingObjectId = 0;
			}
			else if (RotatingObjectId == selectedObject->id)
			{
				const float CurrentMouseAngleDegrees = MouseAngleDegrees(Center, Mouse);
				const float DeltaDegrees = CurrentMouseAngleDegrees - DragStartMouseAngleDegrees;
				selectedObject->transform.rotationDegrees = NormalizeDegrees(DragStartRotationDegrees + DeltaDegrees);
				sceneDocument.SetDirty(true);
				canvasClickConsumed = true;
			}
			else
			{
				RotatingObject = false;
				RotatingObjectId = 0;
			}
		}

		const float RotationRadians = DegreesToRadians(selectedObject->transform.rotationDegrees);
		const ImVec2 Handle(
			Center.x + std::cos(RotationRadians) * Radius,
			Center.y + std::sin(RotationRadians) * Radius);
		const ImU32 RingColor = RotatingObject ? IM_COL32(255, 211, 91, 255) : IM_COL32(134, 185, 236, 230);
		drawList->AddCircle(Center, Radius, IM_COL32(24, 24, 24, 180), 48, 5.0f);
		drawList->AddCircle(Center, Radius, RingColor, 48, 2.0f);
		drawList->AddLine(Center, Handle, RingColor, 2.0f);
		drawList->AddCircleFilled(Handle, 6.0f, RingColor, 24);
		drawList->AddCircle(Handle, 6.0f, IM_COL32(20, 20, 20, 220), 24, 1.0f);
	}

	if (!editEnabled || activeTool != EditorTool::Scale || !selectedObject || !selectedObject->visible || selectedObject->locked || !CanScaleObject(*selectedObject))
	{
		ActiveScaleGizmoAxis = GizmoAxis::None;
		ActiveScaleGizmoObjectId = 0;
	}
	else
	{
		activeGizmoAxis = GizmoAxis::None;
		activeGizmoObjectId = 0;
		RotatingObject = false;
		RotatingObjectId = 0;

		const ObjectBounds SelectedScreenBounds = boundsToScreen(GetObjectBounds(*selectedObject));
		const std::array<ImVec2, 4> SelectedQuad = RotatedQuad(SelectedScreenBounds, selectedObject->transform.rotationDegrees);
		const ImVec2 Mouse = ImGui::GetIO().MousePos;
		const ImVec2 RightHandle = Midpoint(SelectedQuad[1], SelectedQuad[2]);
		const ImVec2 BottomHandle = Midpoint(SelectedQuad[2], SelectedQuad[3]);
		const ImVec2 CornerHandle = SelectedQuad[2];
		const float HandleRadius = 9.0f;

		auto HitScaleGizmo = [&]() -> GizmoAxis
		{
			if (ContainsHandle(Mouse, CornerHandle, HandleRadius + 2.0f))
			{
				return GizmoAxis::XY;
			}
			if (ContainsHandle(Mouse, RightHandle, HandleRadius))
			{
				return GizmoAxis::X;
			}
			if (ContainsHandle(Mouse, BottomHandle, HandleRadius))
			{
				return GizmoAxis::Y;
			}
			return GizmoAxis::None;
		};

		if (ActiveScaleGizmoAxis == GizmoAxis::None && canvasHovered && ImGui::IsMouseClicked(0))
		{
			const GizmoAxis HitAxis = HitScaleGizmo();
			if (HitAxis != GizmoAxis::None)
			{
				ActiveScaleGizmoAxis = HitAxis;
				ActiveScaleGizmoObjectId = selectedObject->id;
				DragStartScaleMouseWorld = screenToWorld(Mouse);
				DragStartObjectScale = ClampObjectScale(selectedObject->transform.scale);
				canvasClickConsumed = true;
			}
		}

		if (ActiveScaleGizmoAxis != GizmoAxis::None)
		{
			if (!ImGui::IsMouseDown(0))
			{
				ActiveScaleGizmoAxis = GizmoAxis::None;
				ActiveScaleGizmoObjectId = 0;
			}
			else if (ActiveScaleGizmoObjectId == selectedObject->id)
			{
				const EditorVec2 MouseWorld = screenToWorld(Mouse);
				const EditorVec2 WorldDelta{
					MouseWorld.x - DragStartScaleMouseWorld.x,
					MouseWorld.y - DragStartScaleMouseWorld.y};
				const EditorVec2 LocalDelta = RotateDeltaToLocal(WorldDelta, selectedObject->transform.rotationDegrees);
				const float BaseWidth = std::max(1.0f, std::fabs(selectedObject->size.x));
				const float BaseHeight = std::max(1.0f, std::fabs(selectedObject->size.y));
				EditorVec2 Scale = DragStartObjectScale;

				if (ActiveScaleGizmoAxis == GizmoAxis::X || ActiveScaleGizmoAxis == GizmoAxis::XY)
				{
					Scale.x = ClampObjectScale(DragStartObjectScale.x + (LocalDelta.x * 2.0f) / BaseWidth);
				}
				if (ActiveScaleGizmoAxis == GizmoAxis::Y || ActiveScaleGizmoAxis == GizmoAxis::XY)
				{
					Scale.y = ClampObjectScale(DragStartObjectScale.y + (LocalDelta.y * 2.0f) / BaseHeight);
				}

				selectedObject->transform.scale = Scale;
				sceneDocument.SetDirty(true);
				canvasClickConsumed = true;
			}
			else
			{
				ActiveScaleGizmoAxis = GizmoAxis::None;
				ActiveScaleGizmoObjectId = 0;
			}
		}

		const ObjectBounds UpdatedScreenBounds = boundsToScreen(GetObjectBounds(*selectedObject));
		const std::array<ImVec2, 4> UpdatedQuad = RotatedQuad(UpdatedScreenBounds, selectedObject->transform.rotationDegrees);
		const ImVec2 UpdatedRightHandle = Midpoint(UpdatedQuad[1], UpdatedQuad[2]);
		const ImVec2 UpdatedBottomHandle = Midpoint(UpdatedQuad[2], UpdatedQuad[3]);
		const ImVec2 UpdatedCornerHandle = UpdatedQuad[2];
		const bool XActive = ActiveScaleGizmoAxis == GizmoAxis::X || ActiveScaleGizmoAxis == GizmoAxis::XY;
		const bool YActive = ActiveScaleGizmoAxis == GizmoAxis::Y || ActiveScaleGizmoAxis == GizmoAxis::XY;
		const ImU32 XColor = XActive ? IM_COL32(255, 102, 102, 255) : IM_COL32(222, 72, 72, 255);
		const ImU32 YColor = YActive ? IM_COL32(102, 235, 142, 255) : IM_COL32(72, 190, 104, 255);
		const ImU32 CornerColor = ActiveScaleGizmoAxis == GizmoAxis::XY ? IM_COL32(255, 226, 96, 255) : IM_COL32(236, 196, 76, 255);
		const ImVec2 Center = worldToScreen(selectedObject->transform.position);

		drawList->AddQuad(
			UpdatedQuad[0],
			UpdatedQuad[1],
			UpdatedQuad[2],
			UpdatedQuad[3],
			IM_COL32(24, 24, 24, 190),
			5.0f);
		drawList->AddQuad(
			UpdatedQuad[0],
			UpdatedQuad[1],
			UpdatedQuad[2],
			UpdatedQuad[3],
			IM_COL32(255, 211, 91, 235),
			2.0f);
		drawList->AddLine(Center, UpdatedRightHandle, XColor, 2.0f);
		drawList->AddLine(Center, UpdatedBottomHandle, YColor, 2.0f);
		drawList->AddLine(Center, UpdatedCornerHandle, CornerColor, 2.0f);

		auto DrawScaleHandle = [&](const ImVec2& Position, ImU32 Color, float Radius)
		{
			drawList->AddRectFilled(
				ImVec2(Position.x - Radius, Position.y - Radius),
				ImVec2(Position.x + Radius, Position.y + Radius),
				Color,
				2.0f);
			drawList->AddRect(
				ImVec2(Position.x - Radius, Position.y - Radius),
				ImVec2(Position.x + Radius, Position.y + Radius),
				IM_COL32(20, 20, 20, 220),
				2.0f,
				0,
				1.0f);
		};

		DrawScaleHandle(UpdatedRightHandle, XColor, 6.0f);
		DrawScaleHandle(UpdatedBottomHandle, YColor, 6.0f);
		DrawScaleHandle(UpdatedCornerHandle, CornerColor, 7.0f);
	}

	if (paused)
	{
		drawList->AddText(ImVec2(canvasPos.x + 12.0f, canvasPos.y + 12.0f), IM_COL32(255, 216, 120, 255), "Paused");
	}
	else if (playing)
	{
		drawList->AddText(ImVec2(canvasPos.x + 12.0f, canvasPos.y + 12.0f), IM_COL32(132, 230, 156, 255), "Playing");
	}

	drawList->PopClipRect();

	if (editEnabled && ImGui::BeginDragDropTarget())
	{
		auto BuildWorldPosition = [&]() -> EditorVec2
		{
			const ImVec2 Mouse = ImGui::GetIO().MousePos;
			return EditorVec2{
				CameraX + (Mouse.x - canvasCenter.x) / Zoom,
				CameraY + (Mouse.y - canvasCenter.y) / Zoom};
		};

		if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("AMBER_ASSET"))
		{
			if (Payload->IsDelivery())
			{
				const std::string PayloadId = PayloadString(*Payload);
				if (!PayloadId.empty())
				{
					DropRequest = FDropRequest{
						EDropPayloadType::Asset,
						PayloadId,
						BuildWorldPosition()};
				}
			}
		}
		if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("AMBER_ACTOR_TYPE"))
		{
			if (Payload->IsDelivery())
			{
				const std::string PayloadId = PayloadString(*Payload);
				if (!PayloadId.empty())
				{
					DropRequest = FDropRequest{
						EDropPayloadType::ActorType,
						PayloadId,
						BuildWorldPosition()};
				}
			}
		}
		ImGui::EndDragDropTarget();
	}

	auto HitSceneObject = [&]() -> uint32
	{
		const ImVec2 Mouse = ImGui::GetIO().MousePos;
		uint32 SelectedObjectId = 0;
		const std::vector<SceneObject>& Objects = sceneDocument.GetObjects();
		for (auto It = Objects.rbegin(); It != Objects.rend(); ++It)
		{
			if (!It->visible || It->kind == SceneObjectKind::Grid)
			{
				continue;
			}

			const ObjectBounds ScreenBounds = boundsToScreen(GetObjectBounds(*It));
			if (It->kind == SceneObjectKind::Circle ? Contains(Mouse.x, Mouse.y, ScreenBounds) : ContainsRotatedBounds(Mouse, ScreenBounds, It->transform.rotationDegrees))
			{
				SelectedObjectId = It->id;
				break;
			}
		}
		return SelectedObjectId;
	};

	if (editEnabled && canvasHovered && ImGui::IsMouseClicked(1))
	{
		const ImVec2 Mouse = ImGui::GetIO().MousePos;
		const uint32 SelectedObjectId = HitSceneObject();
		if (SelectedObjectId != 0)
		{
			selection.SelectSceneObject(SelectedObjectId);
			PendingContextMenuRequest = FContextMenuRequest{
				EContextMenuTarget::SceneObject,
				SelectedObjectId,
				screenToWorld(Mouse)};
		}
		else
		{
			selection.Clear();
			PendingContextMenuRequest = FContextMenuRequest{
				EContextMenuTarget::Scene,
				0u,
				screenToWorld(Mouse)};
		}
	}

	if (editEnabled && !canvasClickConsumed && canvasHovered && ImGui::IsMouseClicked(0))
	{
		const uint32 SelectedObjectId = HitSceneObject();
		if (SelectedObjectId != 0)
		{
			selection.SelectSceneObject(SelectedObjectId);
		}
		else
		{
			selection.Clear();
		}
	}

	return DropRequest;
}

float EditorViewport::GetZoom() const
{
	return Zoom;
}

void EditorViewport::SetZoom(float value)
{
	Zoom = std::max(0.25f, std::min(2.0f, value));
}

void EditorViewport::FocusOrigin()
{
	CameraX = 0.0f;
	CameraY = 0.0f;
}

void EditorViewport::FocusObject(const SceneObject& Object)
{
	CameraX = Object.transform.position.x;
	CameraY = Object.transform.position.y;
}

EditorVec2 EditorViewport::GetViewCenter() const
{
	return EditorVec2{CameraX, CameraY};
}

std::optional<EditorViewport::FContextMenuRequest> EditorViewport::ConsumeContextMenuRequest()
{
	std::optional<FContextMenuRequest> Request = PendingContextMenuRequest;
	PendingContextMenuRequest.reset();
	return Request;
}

void EditorViewport::ReleaseRenderResources()
{
	DestroyRuntimePreviewTexture();
	runtimeSceneRenderer.ClearTextureCache();
	runtimeSceneRenderer.SetRenderer(nullptr);
}

EditorViewport::ObjectBounds EditorViewport::GetObjectBounds(const SceneObject& object) const
{
	if (object.kind == SceneObjectKind::Camera)
	{
		return ObjectBounds{
			object.transform.position.x - 44.0f,
			object.transform.position.y - 30.0f,
			88.0f,
			60.0f};
	}

	if (object.kind == SceneObjectKind::RuntimeWorld)
	{
		return ObjectBounds{
			object.transform.position.x - 112.0f,
			object.transform.position.y - 72.0f,
			224.0f,
			144.0f};
	}

	if (object.kind == SceneObjectKind::AssetInstance ||
		object.kind == SceneObjectKind::Box ||
		object.kind == SceneObjectKind::Circle)
	{
		const EditorVec2 Scale = ClampObjectScale(object.transform.scale);
		const float width = object.size.x * Scale.x;
		const float height = object.size.y * Scale.y;
		return ObjectBounds{
			object.transform.position.x - width * 0.5f,
			object.transform.position.y - height * 0.5f,
			width,
			height};
	}

	const EditorVec2 Scale = ClampObjectScale(object.transform.scale);
	return ObjectBounds{
		object.transform.position.x - 32.0f * Scale.x,
		object.transform.position.y - 32.0f * Scale.y,
		64.0f * Scale.x,
		64.0f * Scale.y};
}

EditorViewport::~EditorViewport()
{
	ReleaseRenderResources();
}

bool EditorViewport::EnsureRuntimePreviewTexture(SDL_Renderer* renderer, int width, int height)
{
	if (!renderer || width <= 0 || height <= 0)
	{
		return false;
	}

	runtimeSceneRenderer.SetRenderer(renderer);

	if (runtimePreviewTexture && runtimePreviewWidth == width && runtimePreviewHeight == height)
	{
		return true;
	}

	DestroyRuntimePreviewTexture();
	runtimePreviewTexture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET,
		width,
		height);
	if (!runtimePreviewTexture)
	{
		return false;
	}

	SDL_SetTextureBlendMode(runtimePreviewTexture, SDL_BLENDMODE_BLEND);
	runtimePreviewWidth = width;
	runtimePreviewHeight = height;
	return true;
}

void EditorViewport::DestroyRuntimePreviewTexture()
{
	if (runtimePreviewTexture)
	{
		SDL_DestroyTexture(runtimePreviewTexture);
		runtimePreviewTexture = nullptr;
	}
	runtimePreviewWidth = 0;
	runtimePreviewHeight = 0;
}

} // namespace AE::Editor
