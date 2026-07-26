#include "Editor/Shell/EditorViewport.h"

#include "Editor/Shell/AssetRegistry.h"
#include "Editor/Shell/SelectionService.h"
#include "Editor/Shell/TextureCache.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace AE::Editor
{
namespace
{
    bool Contains(float x, float y, const EditorViewport::ObjectBounds& bounds)
    {
        return x >= bounds.x && x <= bounds.x + bounds.w &&
            y >= bounds.y && y <= bounds.y + bounds.h;
    }

    ImTextureID ToImTextureId(SDL_Texture* texture)
    {
        return reinterpret_cast<ImTextureID>(texture);
    }

    bool IsShapeObject(const SceneObject& object)
    {
        return object.kind == SceneObjectKind::Box || object.kind == SceneObjectKind::Circle;
    }

    ImU32 ShapeFillColor(const SceneObject& object, bool playing)
    {
        if (object.className == "PlayerSpawnObject")
        {
            return IM_COL32(74, 178, 116, playing ? 95 : 72);
        }
        if (object.className == "GoalObject")
        {
            return IM_COL32(228, 83, 86, playing ? 95 : 72);
        }
        if (object.className == "CoinObject")
        {
            return IM_COL32(232, 186, 68, playing ? 120 : 96);
        }
        if (object.className == "SolidPlatformObject")
        {
            return IM_COL32(95, 142, 78, playing ? 105 : 82);
        }
        return object.kind == SceneObjectKind::Circle ?
            IM_COL32(225, 142, 72, playing ? 95 : 72) :
            IM_COL32(78, 150, 204, playing ? 90 : 68);
    }

    ImU32 ShapeOutlineColor(const SceneObject& object, bool selected, bool playing)
    {
        if (selected)
        {
            return IM_COL32(255, 211, 91, 255);
        }
        if (object.className == "PlayerSpawnObject")
        {
            return IM_COL32(104, 224, 152, 235);
        }
        if (object.className == "GoalObject")
        {
            return IM_COL32(255, 116, 118, 235);
        }
        if (object.className == "CoinObject")
        {
            return IM_COL32(255, 214, 86, 245);
        }
        if (object.className == "SolidPlatformObject")
        {
            return IM_COL32(128, 184, 96, 235);
        }
        return playing ? IM_COL32(102, 206, 138, 230) : IM_COL32(104, 184, 238, 230);
    }
}

std::optional<EditorViewport::AssetDropRequest> EditorViewport::Draw(
    SceneDocument& sceneDocument,
    SelectionService& selection,
    const AssetRegistry& assetRegistry,
    TextureCache& textureCache,
    EditorTool activeTool,
    bool playing,
    bool paused)
{
    std::optional<AssetDropRequest> dropRequest;

    if (ImGui::Button("Focus Origin"))
    {
        FocusOrigin();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Grid", &showGrid);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160.0f);
    ImGui::SliderFloat("Zoom", &zoom, 0.25f, 2.0f, "%.2fx");

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.x = std::max(1.0f, canvasSize.x);
    canvasSize.y = std::max(1.0f, canvasSize.y);
    ImGui::InvisibleButton("SceneCanvas", canvasSize);
    const bool canvasHovered = ImGui::IsItemHovered();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 canvasEnd(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);
    const ImVec2 canvasCenter(canvasPos.x + canvasSize.x * 0.5f, canvasPos.y + canvasSize.y * 0.5f);
    auto worldToScreen = [&](EditorVec2 world) {
        return ImVec2(
            canvasCenter.x + (world.x - cameraX) * zoom,
            canvasCenter.y + (world.y - cameraY) * zoom);
    };
    auto screenToWorld = [&](ImVec2 screen) {
        return EditorVec2{
            cameraX + (screen.x - canvasCenter.x) / zoom,
            cameraY + (screen.y - canvasCenter.y) / zoom
        };
    };
    auto boundsToScreen = [&](const ObjectBounds& bounds) {
        const ImVec2 topLeft = worldToScreen(EditorVec2{bounds.x, bounds.y});
        return ObjectBounds{
            topLeft.x,
            topLeft.y,
            bounds.w * zoom,
            bounds.h * zoom
        };
    };

    bool canvasClickConsumed = false;
    const ImGuiIO& io = ImGui::GetIO();
    const bool editEnabled = !playing;

    if (canvasHovered && io.MouseWheel != 0.0f)
    {
        const EditorVec2 mouseWorldBeforeZoom = screenToWorld(io.MousePos);
        SetZoom(zoom * std::pow(1.12f, io.MouseWheel));
        const EditorVec2 mouseWorldAfterZoom = screenToWorld(io.MousePos);
        cameraX += mouseWorldBeforeZoom.x - mouseWorldAfterZoom.x;
        cameraY += mouseWorldBeforeZoom.y - mouseWorldAfterZoom.y;
    }

    if (canvasHovered && ImGui::IsMouseClicked(2))
    {
        panning = true;
        panStartMouseScreen = EditorVec2{io.MousePos.x, io.MousePos.y};
        panStartCamera = EditorVec2{cameraX, cameraY};
        activeGizmoAxis = GizmoAxis::None;
        activeGizmoObjectId = 0;
    }

    if (panning)
    {
        if (!ImGui::IsMouseDown(2))
        {
            panning = false;
        }
        else
        {
            cameraX = panStartCamera.x - (io.MousePos.x - panStartMouseScreen.x) / zoom;
            cameraY = panStartCamera.y - (io.MousePos.y - panStartMouseScreen.y) / zoom;
            canvasClickConsumed = true;
        }
    }

    drawList->PushClipRect(canvasPos, canvasEnd, true);
    drawList->AddRectFilled(canvasPos, canvasEnd, IM_COL32(16, 18, 20, 255));

    if (showGrid)
    {
        const float gridStep = 32.0f * zoom;
        const float originX = canvasCenter.x - cameraX * zoom;
        const float originY = canvasCenter.y - cameraY * zoom;
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

    const ImVec2 origin = worldToScreen(EditorVec2{0.0f, 0.0f});
    drawList->AddLine(ImVec2(canvasPos.x, origin.y), ImVec2(canvasEnd.x, origin.y), IM_COL32(132, 74, 74, 170), 2.0f);
    drawList->AddLine(ImVec2(origin.x, canvasPos.y), ImVec2(origin.x, canvasEnd.y), IM_COL32(75, 126, 86, 170), 2.0f);

    for (const SceneObject& object : sceneDocument.GetObjects())
    {
        if (!object.visible || object.kind == SceneObjectKind::Grid)
        {
            continue;
        }

        const ObjectBounds screenBounds = boundsToScreen(GetObjectBounds(object));
        const bool selected = selection.IsSceneObjectSelected(object.id);
        const ImU32 color = IsShapeObject(object) ?
            ShapeOutlineColor(object, selected, playing) :
            (selected ? IM_COL32(255, 211, 91, 255) :
                (playing ? IM_COL32(102, 206, 138, 220) : IM_COL32(92, 153, 214, 220)));
        const float thickness = selected ? 3.0f : 2.0f;

        const AssetRecord* asset = object.assetId.empty() ? nullptr : assetRegistry.FindAssetById(object.assetId);
        TexturePreview* preview = asset ? textureCache.GetTexture(*asset) : nullptr;

        if (preview && preview->texture)
        {
            drawList->AddImage(
                ToImTextureId(preview->texture),
                ImVec2(screenBounds.x, screenBounds.y),
                ImVec2(screenBounds.x + screenBounds.w, screenBounds.y + screenBounds.h));
            drawList->AddRect(
                ImVec2(screenBounds.x, screenBounds.y),
                ImVec2(screenBounds.x + screenBounds.w, screenBounds.y + screenBounds.h),
                color,
                2.0f,
                0,
                thickness);
        }
        else if (object.kind == SceneObjectKind::Camera)
        {
            drawList->AddCircle(
                ImVec2(screenBounds.x + screenBounds.w * 0.5f, screenBounds.y + screenBounds.h * 0.5f),
                std::max(8.0f, 20.0f * zoom),
                color,
                24,
                thickness);
        }
        else if (object.kind == SceneObjectKind::Circle)
        {
            const ImVec2 center(
                screenBounds.x + screenBounds.w * 0.5f,
                screenBounds.y + screenBounds.h * 0.5f);
            const float radius = std::max(4.0f, std::min(screenBounds.w, screenBounds.h) * 0.5f);
            drawList->AddCircleFilled(center, radius, ShapeFillColor(object, playing), 32);
            drawList->AddCircle(center, radius, color, 32, thickness);
        }
        else if (object.kind == SceneObjectKind::Box)
        {
            drawList->AddRectFilled(
                ImVec2(screenBounds.x, screenBounds.y),
                ImVec2(screenBounds.x + screenBounds.w, screenBounds.y + screenBounds.h),
                ShapeFillColor(object, playing),
                2.0f);
            drawList->AddRect(
                ImVec2(screenBounds.x, screenBounds.y),
                ImVec2(screenBounds.x + screenBounds.w, screenBounds.y + screenBounds.h),
                color,
                2.0f,
                0,
                thickness);
        }
        else
        {
            drawList->AddRect(
                ImVec2(screenBounds.x, screenBounds.y),
                ImVec2(screenBounds.x + screenBounds.w, screenBounds.y + screenBounds.h),
                color,
                2.0f,
                0,
                thickness);
        }

        drawList->AddText(
            ImVec2(screenBounds.x, screenBounds.y - 18.0f),
            IM_COL32(210, 216, 222, 230),
            object.name.c_str());
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

        auto hitGizmo = [&]() {
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
                    mouseWorld.y - dragStartMouseWorld.y
                };
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
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AMBER_ASSET"))
        {
            if (payload->IsDelivery() && payload->Data && payload->DataSize > 0)
            {
                std::string assetId(static_cast<const char*>(payload->Data), static_cast<std::size_t>(payload->DataSize));
                if (!assetId.empty() && assetId.back() == '\0')
                {
                    assetId.pop_back();
                }

                const ImVec2 mouse = ImGui::GetIO().MousePos;
                dropRequest = AssetDropRequest{
                    assetId,
                    EditorVec2{
                        cameraX + (mouse.x - canvasCenter.x) / zoom,
                        cameraY + (mouse.y - canvasCenter.y) / zoom
                    }
                };
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (editEnabled && !canvasClickConsumed && canvasHovered && ImGui::IsMouseClicked(0))
    {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        std::uint32_t selectedObjectId = 0;
        const std::vector<SceneObject>& objects = sceneDocument.GetObjects();
        for (auto it = objects.rbegin(); it != objects.rend(); ++it)
        {
            if (!it->visible || it->kind == SceneObjectKind::Grid)
            {
                continue;
            }

            const ObjectBounds screenBounds = boundsToScreen(GetObjectBounds(*it));
            if (Contains(mouse.x, mouse.y, screenBounds))
            {
                selectedObjectId = it->id;
                break;
            }
        }

        if (selectedObjectId != 0)
        {
            selection.SelectSceneObject(selectedObjectId);
        }
        else
        {
            selection.Clear();
        }
    }

    return dropRequest;
}

float EditorViewport::GetZoom() const
{
    return zoom;
}

void EditorViewport::SetZoom(float value)
{
    zoom = std::max(0.25f, std::min(2.0f, value));
}

void EditorViewport::FocusOrigin()
{
    cameraX = 0.0f;
    cameraY = 0.0f;
}

EditorVec2 EditorViewport::GetViewCenter() const
{
    return EditorVec2{cameraX, cameraY};
}

EditorViewport::ObjectBounds EditorViewport::GetObjectBounds(const SceneObject& object) const
{
    if (object.kind == SceneObjectKind::Camera)
    {
        return ObjectBounds{
            object.transform.position.x - 44.0f,
            object.transform.position.y - 30.0f,
            88.0f,
            60.0f
        };
    }

    if (object.kind == SceneObjectKind::RuntimeWorld)
    {
        return ObjectBounds{
            object.transform.position.x - 112.0f,
            object.transform.position.y - 72.0f,
            224.0f,
            144.0f
        };
    }

    if (object.kind == SceneObjectKind::AssetInstance ||
        object.kind == SceneObjectKind::Box ||
        object.kind == SceneObjectKind::Circle)
    {
        const float width = object.size.x * object.transform.scale.x;
        const float height = object.size.y * object.transform.scale.y;
        return ObjectBounds{
            object.transform.position.x - width * 0.5f,
            object.transform.position.y - height * 0.5f,
            width,
            height
        };
    }

    return ObjectBounds{
        object.transform.position.x - 32.0f,
        object.transform.position.y - 32.0f,
        64.0f,
        64.0f
    };
}

}
