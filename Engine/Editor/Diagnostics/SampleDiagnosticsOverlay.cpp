#include "SampleDiagnosticsOverlay.h"

#include <algorithm>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_sdl.h"

namespace AE::Editor
{

bool SampleDiagnosticsOverlay::Initialize(
    SDL_Window* appWindow,
    SDL_Renderer* appRenderer,
    int logicalWidth,
    int logicalHeight)
{
    if (!appWindow || !appRenderer)
    {
        return false;
    }

    window = appWindow;
    renderer = appRenderer;
    this->logicalWidth = logicalWidth;
    this->logicalHeight = logicalHeight;

    if (!ImGui::GetCurrentContext())
    {
        ImGui::CreateContext();
    }

    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForD3D(window);
    ImGuiSDL::Initialize(renderer, logicalWidth, logicalHeight);
    ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(logicalWidth), static_cast<float>(logicalHeight));

    previousCounter = SDL_GetPerformanceCounter();
    ready = true;
    return true;
}

void SampleDiagnosticsOverlay::Shutdown()
{
    if (!ready)
    {
        return;
    }

    ImGuiSDL::Deinitialize();
    ImGui_ImplSDL2_Shutdown();
    if (ImGui::GetCurrentContext())
    {
        ImGui::DestroyContext();
    }

    ready = false;
    window = nullptr;
    renderer = nullptr;
    logicalWidth = 0;
    logicalHeight = 0;
    previousCounter = 0;
}

bool SampleDiagnosticsOverlay::IsReady() const
{
    return ready;
}

void SampleDiagnosticsOverlay::ProcessEvent(const SDL_Event& event)
{
    if (!ready)
    {
        return;
    }

    ImGui_ImplSDL2_ProcessEvent(&event);
}

bool SampleDiagnosticsOverlay::WantsKeyboard() const
{
    return ready && ImGui::GetIO().WantCaptureKeyboard;
}

bool SampleDiagnosticsOverlay::WantsMouse() const
{
    return ready && ImGui::GetIO().WantCaptureMouse;
}

void SampleDiagnosticsOverlay::BeginFrame()
{
    if (!ready)
    {
        return;
    }

    const Uint64 currentCounter = SDL_GetPerformanceCounter();
    if (previousCounter != 0)
    {
        frameMs = static_cast<double>(currentCounter - previousCounter) * 1000.0 /
            static_cast<double>(SDL_GetPerformanceFrequency());
        smoothedFrameMs = smoothedFrameMs <= 0.0 ? frameMs : smoothedFrameMs * 0.9 + frameMs * 0.1;
        fps = smoothedFrameMs > 0.0 ? 1000.0 / smoothedFrameMs : 0.0;
    }
    previousCounter = currentCounter;

    ImGui_ImplSDL2_NewFrame(window);
    ImGuiSDL::ApplyLogicalDisplaySize(window, renderer, logicalWidth, logicalHeight);
    ImGui::NewFrame();
}

void SampleDiagnosticsOverlay::Draw(
    const SampleDiagnosticsData& data,
    const std::function<void()>& drawControls)
{
    if (!ready)
    {
        return;
    }

    if (showDiagnostics)
    {
        ImGui::SetNextWindowPos(ImVec2(14.0f, 14.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(330.0f, 280.0f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Performance", &showDiagnostics))
        {
            ImGui::TextUnformatted(data.sampleName);
            if (!data.statusText.empty())
            {
                ImGui::TextWrapped("%s", data.statusText.c_str());
            }
            ImGui::Separator();
            ImGui::Text("FPS: %.1f", fps);
            ImGui::Text("Frame: %.3f ms", frameMs);
            ImGui::Text("Update: %.3f ms", data.updateMs);
            ImGui::Text("Render: %.3f ms", data.renderMs);
            ImGui::Text("Fixed steps: %d", data.fixedSteps);
            ImGui::Text("Paused: %s", data.paused ? "yes" : "no");

            if (data.hasPhysicsStats)
            {
                const SamplePhysicsStats& stats = data.physics;
                ImGui::Separator();
                ImGui::Text("Bodies: %zu", stats.bodies);
                ImGui::Text("Contacts: %zu", stats.contacts);
                ImGui::Text("Constraints: %zu", stats.constraints);
                ImGui::Text("Pairs: %zu -> %zu", stats.bruteForcePairs, stats.broadPhasePairs);
                ImGui::Text("Narrow tests: %zu", stats.narrowPhaseTests);
                ImGui::Text("Physics: %.3f ms", stats.physicsStepMs);
                ImGui::Text("Broad/Narrow: %.3f / %.3f", stats.broadPhaseMs, stats.narrowPhaseMs);
                ImGui::Text("Solver: %.3f", stats.solverMs);
                ImGui::Text(
                    "Islands: %zu (max %zu bodies / %zu constraints)",
                    stats.solverIslandCount,
                    stats.largestSolverIslandBodyCount,
                    stats.largestSolverIslandConstraintCount);
                ImGui::Text(
                    "Parallel narrow: %s (%zu jobs)",
                    stats.parallelNarrowPhaseUsed ? "on" : "off",
                    stats.parallelNarrowPhaseJobs);
                ImGui::Text(
                    "Parallel solver: %s (%zu jobs)",
                    stats.parallelSolverUsed ? "on" : "off",
                    stats.parallelSolverJobs);
            }
        }
        ImGui::End();
    }

    if (showControls && drawControls)
    {
        ImGui::SetNextWindowPos(ImVec2(14.0f, 310.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(330.0f, 220.0f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Sample Controls", &showControls))
        {
            drawControls();
        }
        ImGui::End();
    }

    if (showOutputLog)
    {
        outputLog.Draw(&showOutputLog);
    }
}

void SampleDiagnosticsOverlay::Render()
{
    if (!ready)
    {
        return;
    }

    ImGui::Render();
    ImGuiSDL::Render(ImGui::GetDrawData());
}

bool& SampleDiagnosticsOverlay::ShowDiagnostics()
{
    return showDiagnostics;
}

bool& SampleDiagnosticsOverlay::ShowControls()
{
    return showControls;
}

bool& SampleDiagnosticsOverlay::ShowOutputLog()
{
    return showOutputLog;
}

double SampleDiagnosticsOverlay::GetFrameMs() const
{
    return frameMs;
}

double SampleDiagnosticsOverlay::GetFps() const
{
    return fps;
}

}
