#include "Editor/Shell/EditorViewport.h"

#include "Editor/Shell/SelectionService.h"
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
}

void EditorViewport::Draw(SceneDocument& sceneDocument, SelectionService& selection, bool playing, bool paused)
{
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

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 canvasEnd(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);
    const ImVec2 canvasCenter(canvasPos.x + canvasSize.x * 0.5f, canvasPos.y + canvasSize.y * 0.5f);
    auto worldToScreen = [&](EditorVec2 world) {
        return ImVec2(
            canvasCenter.x + (world.x - cameraX) * zoom,
            canvasCenter.y + (world.y - cameraY) * zoom);
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
        const ImU32 color = selected ? IM_COL32(255, 211, 91, 255) :
            (playing ? IM_COL32(102, 206, 138, 220) : IM_COL32(92, 153, 214, 220));
        const float thickness = selected ? 3.0f : 2.0f;

        if (object.kind == SceneObjectKind::Camera)
        {
            drawList->AddCircle(
                ImVec2(screenBounds.x + screenBounds.w * 0.5f, screenBounds.y + screenBounds.h * 0.5f),
                std::max(8.0f, 20.0f * zoom),
                color,
                24,
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

    if (paused)
    {
        drawList->AddText(ImVec2(canvasPos.x + 12.0f, canvasPos.y + 12.0f), IM_COL32(255, 216, 120, 255), "Paused");
    }
    else if (playing)
    {
        drawList->AddText(ImVec2(canvasPos.x + 12.0f, canvasPos.y + 12.0f), IM_COL32(132, 230, 156, 255), "Playing");
    }

    drawList->PopClipRect();

    if (ImGui::IsItemClicked())
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

    return ObjectBounds{
        object.transform.position.x - 32.0f,
        object.transform.position.y - 32.0f,
        64.0f,
        64.0f
    };
}

}
