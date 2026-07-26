#include "Editor/Shell/EditorApplication.h"

#include "Logging/LogBus.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_sdl.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <iostream>

namespace AE::Editor
{
namespace
{
    constexpr int DefaultWindowWidth = 1280;
    constexpr int DefaultWindowHeight = 720;

    std::filesystem::path FindContentRoot()
    {
        namespace fs = std::filesystem;

        const std::array<fs::path, 8> candidateRoots = {
            fs::current_path(),
            fs::current_path() / "AmberEngine",
            fs::current_path() / "..",
            fs::current_path() / ".." / "..",
            fs::current_path() / ".." / ".." / "..",
            fs::current_path() / ".." / "AmberEngine",
            fs::current_path() / ".." / ".." / "AmberEngine",
            fs::current_path() / ".." / ".." / ".." / "AmberEngine"
        };

        for (const fs::path& root : candidateRoots)
        {
            const fs::path candidate = root / "Content";
            std::error_code error;
            if (fs::exists(candidate, error) && fs::is_directory(candidate, error))
            {
                return fs::weakly_canonical(candidate, error);
            }
        }

        return fs::current_path() / "Content";
    }

    void SetPanelBounds(float x, float y, float width, float height)
    {
        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(std::max(1.0f, width), std::max(1.0f, height)), ImGuiCond_Always);
    }

    ImGuiWindowFlags FixedPanelFlags()
    {
        return ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse;
    }
}

int EditorApplication::Run()
{
    if (!Initialize(false))
    {
        return 1;
    }

    running = true;
    while (running)
    {
        PollEvents();
        BeginFrame();
        DrawLayout();
        RenderFrame();
    }

    Shutdown();
    return 0;
}

bool EditorApplication::RunSmokeTest()
{
    if (!Initialize(true))
    {
        return false;
    }

    BeginFrame();
    DrawLayout();
    RenderFrame();
    Shutdown();
    return true;
}

bool EditorApplication::Initialize(bool hiddenWindow)
{
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    const Uint32 windowFlags = SDL_WINDOW_RESIZABLE | (hiddenWindow ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN);
    window = SDL_CreateWindow(
        "Amber Editor",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        DefaultWindowWidth,
        DefaultWindowHeight,
        windowFlags);
    if (!window)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
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
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        Shutdown();
        return false;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    windowWidth = std::max(1, windowWidth);
    windowHeight = std::max(1, windowHeight);
    SDL_RenderSetLogicalSize(renderer, windowWidth, windowHeight);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ApplyStyle();
    ImGui_ImplSDL2_InitForD3D(window);
    ImGuiSDL::Initialize(renderer, windowWidth, windowHeight);
    imguiReady = true;

    contentRoot = FindContentRoot();
    currentAssetPath = contentRoot;
    RefreshAssets();
    if (!sceneDocument.GetObjects().empty())
    {
        selectionService.SelectSceneObject(sceneDocument.GetObjects().front().id);
    }

    LogBus::Add(LogLevel::Info, "Editor", "Amber Editor shell initialized.");
    return true;
}

void EditorApplication::Shutdown()
{
    if (imguiReady)
    {
        ImGuiSDL::Deinitialize();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        imguiReady = false;
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

    SDL_Quit();
}

void EditorApplication::PollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (imguiReady)
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
        }

        if (event.type == SDL_QUIT)
        {
            running = false;
        }
        else if (event.type == SDL_KEYDOWN && !event.key.repeat)
        {
            const SDL_Keycode key = event.key.keysym.sym;
            if (key == SDLK_ESCAPE)
            {
                running = false;
            }
            else if (key == SDLK_F5)
            {
                playing = true;
                paused = false;
                LogBus::Add(LogLevel::Info, "Editor", "Play requested.");
            }
            else if (key == SDLK_F6)
            {
                paused = !paused;
                LogBus::Add(LogLevel::Info, "Editor", paused ? "Editor paused." : "Editor resumed.");
            }
            else if (key == SDLK_F7)
            {
                playing = false;
                paused = false;
                LogBus::Add(LogLevel::Info, "Editor", "Play stopped.");
            }
        }
    }
}

void EditorApplication::BeginFrame()
{
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    windowWidth = std::max(1, windowWidth);
    windowHeight = std::max(1, windowHeight);
    SDL_RenderSetLogicalSize(renderer, windowWidth, windowHeight);

    ImGui_ImplSDL2_NewFrame(window);
    ImGuiSDL::ApplyLogicalDisplaySize(window, renderer, windowWidth, windowHeight);
    ImGui::NewFrame();
}

void EditorApplication::RenderFrame()
{
    ImGui::Render();

    SDL_SetRenderDrawColor(renderer, 18, 20, 22, 255);
    SDL_RenderClear(renderer);
    ImGuiSDL::Render(ImGui::GetDrawData());
    SDL_RenderPresent(renderer);
}

void EditorApplication::ApplyStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 2.0f;
    style.ChildRounding = 2.0f;
    style.FrameRounding = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.WindowPadding = ImVec2(10.0f, 8.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.105f, 0.113f, 0.122f, 1.0f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.075f, 0.082f, 0.090f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.125f, 0.137f, 0.150f, 1.0f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.075f, 0.082f, 0.090f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.180f, 0.220f, 0.255f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.230f, 0.285f, 0.330f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.290f, 0.360f, 0.420f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.170f, 0.190f, 0.210f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.240f, 0.275f, 0.310f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.305f, 0.365f, 0.420f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.090f, 0.100f, 0.110f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.130f, 0.150f, 0.170f, 1.0f);
}

void EditorApplication::DrawLayout()
{
    DrawMainMenuBar();

    const float menuHeight = ImGui::GetFrameHeight();
    const float toolbarHeight = 42.0f;
    const float rightWidth = std::max(280.0f, std::min(360.0f, windowWidth * 0.24f));
    const float bottomHeight = std::max(180.0f, std::min(260.0f, windowHeight * 0.31f));
    const float top = menuHeight + toolbarHeight;
    const float rightX = windowWidth - rightWidth;
    const float bottomY = windowHeight - bottomHeight;
    const float centerWidth = std::max(320.0f, rightX);
    const float sceneHeight = std::max(220.0f, bottomY - top);
    const float assetWidth = showOutputLog ? centerWidth * 0.60f : centerWidth;

    DrawToolbar(menuHeight, toolbarHeight);
    DrawSceneView(0.0f, top, centerWidth, sceneHeight);

    if (showAssetBrowser)
    {
        DrawAssetBrowser(0.0f, bottomY, assetWidth, bottomHeight);
    }
    if (showOutputLog)
    {
        DrawOutputLog(assetWidth, bottomY, centerWidth - assetWidth, bottomHeight);
    }
    if (showSceneOutliner)
    {
        DrawSceneOutliner(rightX, top, rightWidth, sceneHeight * 0.46f);
    }
    if (showDetails)
    {
        DrawDetailsPanel(rightX, top + sceneHeight * 0.46f, rightWidth, windowHeight - (top + sceneHeight * 0.46f));
    }
}

void EditorApplication::DrawMainMenuBar()
{
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New Scene"))
        {
            sceneDocument.NewScene();
            if (!sceneDocument.GetObjects().empty())
            {
                selectionService.SelectSceneObject(sceneDocument.GetObjects().front().id);
            }
            else
            {
                selectionService.Clear();
            }
            LogBus::Add(LogLevel::Info, "Editor", "New scene requested.");
        }
        if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
        {
            LogBus::Add(LogLevel::Warning, "Editor", "Scene serialization is not implemented yet.");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Esc"))
        {
            running = false;
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Window"))
    {
        ImGui::MenuItem("Asset Browser", nullptr, &showAssetBrowser);
        ImGui::MenuItem("Scene Outliner", nullptr, &showSceneOutliner);
        ImGui::MenuItem("Details", nullptr, &showDetails);
        ImGui::MenuItem("Output Log", nullptr, &showOutputLog);
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void EditorApplication::DrawToolbar(float menuHeight, float toolbarHeight)
{
    SetPanelBounds(0.0f, menuHeight, static_cast<float>(windowWidth), toolbarHeight);
    ImGui::Begin(
        "Editor Toolbar",
        nullptr,
        FixedPanelFlags() | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);

    if (ImGui::Button("Select"))
    {
        LogBus::Add(LogLevel::Info, "Editor", "Select tool active.");
    }
    ImGui::SameLine();
    ImGui::Button("Move");
    ImGui::SameLine();
    ImGui::Button("Rotate");
    ImGui::SameLine();
    ImGui::Button("Scale");

    ImGui::SameLine(ImGui::GetWindowWidth() * 0.44f);
    if (ImGui::Button("Play"))
    {
        playing = true;
        paused = false;
        LogBus::Add(LogLevel::Info, "Editor", "Play requested.");
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause"))
    {
        if (playing)
        {
            paused = !paused;
            LogBus::Add(LogLevel::Info, "Editor", paused ? "Editor paused." : "Editor resumed.");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        playing = false;
        paused = false;
        LogBus::Add(LogLevel::Info, "Editor", "Play stopped.");
    }

    ImGui::SameLine();
    const char* state = playing ? (paused ? "Paused" : "Playing") : "Editing";
    ImGui::TextDisabled("%s", state);

    ImGui::End();
}

void EditorApplication::DrawSceneView(float x, float y, float width, float height)
{
    SetPanelBounds(x, y, width, height);
    ImGui::Begin("Scene View", nullptr, FixedPanelFlags() | ImGuiWindowFlags_NoScrollbar);
    editorViewport.Draw(sceneDocument, selectionService, playing, paused);
    ImGui::End();
}

void EditorApplication::DrawAssetBrowser(float x, float y, float width, float height)
{
    SetPanelBounds(x, y, width, height);
    ImGui::Begin("Asset Browser", &showAssetBrowser, FixedPanelFlags());

    if (ImGui::Button("Content"))
    {
        OpenAssetDirectory(contentRoot);
    }
    ImGui::SameLine();
    if (ImGui::Button("Up") && currentAssetPath != contentRoot)
    {
        OpenAssetDirectory(currentAssetPath.parent_path());
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh"))
    {
        RefreshAssets();
    }

    ImGui::TextDisabled("%s", RelativeAssetLabel(currentAssetPath).c_str());
    ImGui::Separator();

    ImGui::BeginChild("AssetList", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const AssetEntry& entry : assetEntries)
    {
        const std::string name = entry.path.filename().string();
        const std::string label = entry.directory ? "[Dir] " + name : "[File] " + name;
        const std::string assetPath = entry.path.string();
        const bool selected = selectionService.IsAssetSelected(assetPath);

        if (ImGui::Selectable(label.c_str(), selected))
        {
            if (entry.directory)
            {
                OpenAssetDirectory(entry.path);
            }
            else
            {
                selectionService.SelectAsset(assetPath);
            }
        }
    }
    ImGui::EndChild();
    ImGui::End();
}

void EditorApplication::DrawSceneOutliner(float x, float y, float width, float height)
{
    SetPanelBounds(x, y, width, height);
    ImGui::Begin("Scene Outliner", &showSceneOutliner, FixedPanelFlags());

    ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DefaultOpen;
    if (sceneDocument.IsDirty())
    {
        ImGui::TextDisabled("Modified");
    }

    if (ImGui::TreeNodeEx(sceneDocument.GetName().c_str(), rootFlags))
    {
        for (const SceneObject& object : sceneDocument.GetObjects())
        {
            ImGui::PushID(static_cast<int>(object.id));
            const bool selected = selectionService.IsSceneObjectSelected(object.id);
            if (ImGui::Selectable(object.name.c_str(), selected))
            {
                selectionService.SelectSceneObject(object.id);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("%s", SceneDocument::KindName(object.kind));
            ImGui::PopID();
        }
        ImGui::TreePop();
    }

    ImGui::End();
}

void EditorApplication::DrawDetailsPanel(float x, float y, float width, float height)
{
    SetPanelBounds(x, y, width, height);
    ImGui::Begin("Details", &showDetails, FixedPanelFlags());

    const EditorSelection& selection = selectionService.GetSelection();
    if (selection.type == EditorSelectionType::SceneObject)
    {
        SceneObject* object = sceneDocument.FindObject(selection.objectId);
        if (object)
        {
            ImGui::Text("Selection: %s", object->name.c_str());
            ImGui::TextDisabled("Type: %s", SceneDocument::KindName(object->kind));
            ImGui::Separator();

            bool changed = false;
            char nameBuffer[128] = {};
            std::snprintf(nameBuffer, sizeof(nameBuffer), "%s", object->name.c_str());
            if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
            {
                object->name = nameBuffer;
                changed = true;
            }

            changed |= ImGui::Checkbox("Visible", &object->visible);
            changed |= ImGui::Checkbox("Locked", &object->locked);
            changed |= ImGui::InputFloat2("Position", &object->transform.position.x);
            changed |= ImGui::InputFloat("Rotation", &object->transform.rotationDegrees);
            changed |= ImGui::InputFloat2("Scale", &object->transform.scale.x);

            if (changed)
            {
                sceneDocument.SetDirty(true);
            }
        }
        else
        {
            selectionService.Clear();
            ImGui::TextDisabled("Nothing selected");
        }
    }
    else if (selection.type == EditorSelectionType::Asset)
    {
        ImGui::TextWrapped("Asset: %s", RelativeAssetLabel(selection.assetPath).c_str());
        ImGui::Separator();
        ImGui::Text("Type: File");
    }
    else
    {
        ImGui::TextDisabled("Nothing selected");
    }

    ImGui::End();
}

void EditorApplication::DrawOutputLog(float x, float y, float width, float height)
{
    SetPanelBounds(x, y, width, height);
    outputLog.Draw(&showOutputLog);
}

void EditorApplication::RefreshAssets()
{
    assetEntries.clear();

    std::error_code error;
    if (!std::filesystem::exists(currentAssetPath, error) || !std::filesystem::is_directory(currentAssetPath, error))
    {
        return;
    }

    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(currentAssetPath, error))
    {
        if (error)
        {
            break;
        }

        assetEntries.push_back(AssetEntry{entry.path(), entry.is_directory(error)});
    }

    std::sort(assetEntries.begin(), assetEntries.end(), [](const AssetEntry& left, const AssetEntry& right) {
        if (left.directory != right.directory)
        {
            return left.directory > right.directory;
        }

        return left.path.filename().string() < right.path.filename().string();
    });
}

void EditorApplication::OpenAssetDirectory(const std::filesystem::path& path)
{
    std::error_code error;
    if (!std::filesystem::exists(path, error) || !std::filesystem::is_directory(path, error))
    {
        return;
    }

    currentAssetPath = std::filesystem::weakly_canonical(path, error);
    if (currentAssetPath.empty())
    {
        currentAssetPath = path;
    }
    RefreshAssets();
}

std::string EditorApplication::RelativeAssetLabel(const std::filesystem::path& path) const
{
    std::error_code error;
    const std::filesystem::path relative = std::filesystem::relative(path, contentRoot, error);
    if (!error && !relative.empty())
    {
        return (std::filesystem::path("Content") / relative).generic_string();
    }

    return path.generic_string();
}

}
