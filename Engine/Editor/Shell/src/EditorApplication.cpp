#include "Editor/Shell/EditorApplication.h"

#include "Logging/LogBus.h"
#include <SDL2/SDL_image.h>
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

    std::filesystem::path FindContentRoot(const std::filesystem::path& relativeContentPath)
    {
        namespace fs = std::filesystem;

        std::error_code error;
        fs::path current = fs::weakly_canonical(fs::current_path(), error);
        if (current.empty())
        {
            current = fs::current_path();
        }

        for (int depth = 0; depth < 10; ++depth)
        {
            const std::array<fs::path, 2> candidates = {
                current / relativeContentPath,
                current / "AmberEngine" / relativeContentPath
            };

            for (const fs::path& candidate : candidates)
            {
                if (fs::exists(candidate, error) && fs::is_directory(candidate, error))
                {
                    return fs::weakly_canonical(candidate, error);
                }
            }

            if (!current.has_parent_path() || current.parent_path() == current)
            {
                break;
            }

            current = current.parent_path();
        }

        return {};
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

    ImTextureID ToImTextureId(SDL_Texture* texture)
    {
        return reinterpret_cast<ImTextureID>(texture);
    }

    ImVec2 FitPreviewSize(int width, int height, float maxWidth, float maxHeight)
    {
        if (width <= 0 || height <= 0)
        {
            return ImVec2(maxWidth, maxHeight);
        }

        const float scale = std::min(maxWidth / static_cast<float>(width), maxHeight / static_cast<float>(height));
        return ImVec2(
            std::max(1.0f, width * scale),
            std::max(1.0f, height * scale));
    }

    EditorVec2 FitSceneTextureSize(int width, int height)
    {
        if (width <= 0 || height <= 0)
        {
            return EditorVec2{96.0f, 96.0f};
        }

        const float maxDimension = 160.0f;
        const float scale = maxDimension / static_cast<float>(std::max(width, height));
        return EditorVec2{
            std::max(24.0f, width * scale),
            std::max(24.0f, height * scale)
        };
    }

    bool ToolButton(const char* label, EditorTool& activeTool, EditorTool tool)
    {
        const bool active = activeTool == tool;
        if (active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.275f, 0.360f, 0.430f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.335f, 0.430f, 0.510f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.400f, 0.500f, 0.590f, 1.0f));
        }

        const bool clicked = ImGui::Button(label);
        if (active)
        {
            ImGui::PopStyleColor(3);
        }

        if (clicked)
        {
            activeTool = tool;
        }
        return clicked;
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

    if ((IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & IMG_INIT_PNG) == 0)
    {
        std::cerr << "IMG_Init failed: " << IMG_GetError() << std::endl;
        Shutdown();
        return false;
    }
    imageSystemInitialized = true;

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
    textureCache.Initialize(renderer);

    projectContentRoot = FindContentRoot("Content");
    if (projectContentRoot.empty())
    {
        projectContentRoot = std::filesystem::current_path() / "Content";
    }
    engineContentRoot = FindContentRoot(std::filesystem::path("Engine") / "Content");

    contentRoot = projectContentRoot;
    assetRegistry.ScanRoots(BuildAssetRoots());
    currentAssetPath = contentRoot;
    currentScenePath = DefaultScenePath();

    std::error_code scenePathError;
    if (!currentScenePath.empty() && std::filesystem::exists(currentScenePath, scenePathError))
    {
        OpenSceneFile(currentScenePath);
    }
    else
    {
        RefreshAssets();
        if (!sceneDocument.GetObjects().empty())
        {
            selectionService.SelectSceneObject(sceneDocument.GetObjects().front().id);
        }
    }

    LogBus::Add(LogLevel::Info, "Editor", "Amber Editor shell initialized.");
    LogBus::Add(LogLevel::Info, "Editor", "Project content: " + projectContentRoot.string());
    if (!engineContentRoot.empty())
    {
        LogBus::Add(LogLevel::Info, "Editor", "Engine content: " + engineContentRoot.string());
    }
    return true;
}

void EditorApplication::Shutdown()
{
    textureCache.Clear();

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
    if (imageSystemInitialized)
    {
        IMG_Quit();
        imageSystemInitialized = false;
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
            const bool ctrlPressed = (event.key.keysym.mod & KMOD_CTRL) != 0;
            const bool textInputActive = imguiReady && ImGui::GetIO().WantTextInput;
            if (key == SDLK_ESCAPE)
            {
                running = false;
            }
            else if (key == SDLK_s && ctrlPressed && !textInputActive)
            {
                SaveCurrentScene();
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
            else if (key == SDLK_DELETE && !textInputActive)
            {
                DeleteSelectedSceneObject();
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
            currentScenePath = DefaultScenePath();
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
        if (ImGui::MenuItem("Open Platformer Test Scene"))
        {
            OpenSceneFile(DefaultScenePath());
        }
        if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
        {
            SaveCurrentScene();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Esc"))
        {
            running = false;
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        const EditorSelection& selection = selectionService.GetSelection();
        const bool canDelete =
            selection.type == EditorSelectionType::SceneObject &&
            sceneDocument.IsObjectRemovable(selection.objectId);
        if (ImGui::MenuItem("Delete Selected", "Del", false, canDelete))
        {
            DeleteSelectedSceneObject();
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

    if (ToolButton("Select", activeTool, EditorTool::Select))
    {
        LogBus::Add(LogLevel::Info, "Editor", "Select tool active.");
    }
    ImGui::SameLine();
    if (ToolButton("Move", activeTool, EditorTool::Move))
    {
        LogBus::Add(LogLevel::Info, "Editor", "Move tool active.");
    }
    ImGui::SameLine();
    if (ToolButton("Rotate", activeTool, EditorTool::Rotate))
    {
        LogBus::Add(LogLevel::Info, "Editor", "Rotate tool active.");
    }
    ImGui::SameLine();
    if (ToolButton("Scale", activeTool, EditorTool::Scale))
    {
        LogBus::Add(LogLevel::Info, "Editor", "Scale tool active.");
    }

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
    const std::optional<EditorViewport::AssetDropRequest> dropRequest =
        editorViewport.Draw(sceneDocument, selectionService, assetRegistry, textureCache, activeTool, playing, paused);
    if (dropRequest)
    {
        const AssetRecord* asset = assetRegistry.FindAssetById(dropRequest->assetId);
        if (asset && AssetRegistry::CanInstantiate(asset->type))
        {
            EditorTransform transform;
            transform.position = dropRequest->worldPosition;
            SceneObject& object = sceneDocument.AddAssetInstance(asset->absolutePath.stem().string(), asset->id, transform);
            if (asset->type == AssetType::Texture)
            {
                if (TexturePreview* preview = textureCache.GetTexture(*asset))
                {
                    object.size = FitSceneTextureSize(preview->width, preview->height);
                }
            }
            else if (asset->type == AssetType::Tilemap)
            {
                object.size = EditorVec2{160.0f, 120.0f};
            }
            selectionService.SelectSceneObject(object.id);
            LogBus::Add(LogLevel::Info, "Editor", "Placed asset: " + asset->id);
        }
        else
        {
            LogBus::Add(LogLevel::Warning, "Editor", "Dropped asset cannot be placed in the scene.");
        }
    }
    ImGui::End();
}

void EditorApplication::DrawAssetBrowser(float x, float y, float width, float height)
{
    SetPanelBounds(x, y, width, height);
    ImGui::Begin("Asset Browser", &showAssetBrowser, FixedPanelFlags());

    if (ImGui::Button("Project"))
    {
        SwitchContentRoot(projectContentRoot);
    }
    if (!engineContentRoot.empty())
    {
        ImGui::SameLine();
        if (ImGui::Button("Engine"))
        {
            SwitchContentRoot(engineContentRoot);
        }
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
    bool openPendingDirectory = false;
    std::filesystem::path pendingDirectory;
    for (const AssetEntry& entry : assetEntries)
    {
        const std::string name = entry.path.filename().string();
        const std::string label = entry.directory ? "[Dir] " + name :
            "[" + std::string(AssetRegistry::TypeName(entry.asset ? entry.asset->type : AssetType::Unknown)) + "] " + name;
        const std::string assetId = entry.asset ? entry.asset->id : std::string{};
        const bool selected = !assetId.empty() && selectionService.IsAssetSelected(assetId);

        if (entry.asset && entry.asset->type == AssetType::Texture)
        {
            TexturePreview* preview = textureCache.GetTexture(*entry.asset);
            if (preview && preview->texture)
            {
                ImGui::Image(ToImTextureId(preview->texture), FitPreviewSize(preview->width, preview->height, 34.0f, 34.0f));
                ImGui::SameLine();
            }
        }

        if (ImGui::Selectable(label.c_str(), selected))
        {
            if (entry.directory)
            {
                pendingDirectory = entry.path;
                openPendingDirectory = true;
            }
            else if (entry.asset)
            {
                selectionService.SelectAsset(entry.asset->id);
            }
        }

        if (entry.asset && AssetRegistry::CanInstantiate(entry.asset->type) && ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("AMBER_ASSET", entry.asset->id.c_str(), entry.asset->id.size() + 1);
            ImGui::Text("%s", entry.asset->name.c_str());
            ImGui::TextDisabled("%s", AssetRegistry::TypeName(entry.asset->type));
            ImGui::EndDragDropSource();
        }
        else if (entry.asset && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", AssetRegistry::TypeName(entry.asset->type));
        }
        if (entry.asset && !AssetRegistry::CanInstantiate(entry.asset->type))
        {
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s assets are indexed but not placeable yet.", AssetRegistry::TypeName(entry.asset->type));
            }
        }
    }
    ImGui::EndChild();
    if (openPendingDirectory)
    {
        OpenAssetDirectory(pendingDirectory);
    }
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
            if (!object->assetId.empty())
            {
                ImGui::TextDisabled("Asset: %s", object->assetId.c_str());
                const AssetRecord* asset = assetRegistry.FindAssetById(object->assetId);
                if (asset && asset->type == AssetType::Texture)
                {
                    TexturePreview* preview = textureCache.GetTexture(*asset);
                    if (preview && preview->texture)
                    {
                        ImGui::Image(ToImTextureId(preview->texture), FitPreviewSize(preview->width, preview->height, 220.0f, 140.0f));
                    }
                }
            }
            ImGui::Separator();

            bool changed = false;
            char nameBuffer[128] = {};
            std::snprintf(nameBuffer, sizeof(nameBuffer), "%s", object->name.c_str());
            if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
            {
                object->name = nameBuffer;
                changed = true;
            }

            char classBuffer[128] = {};
            std::snprintf(classBuffer, sizeof(classBuffer), "%s", object->className.c_str());
            if (ImGui::InputText("Class", classBuffer, sizeof(classBuffer)))
            {
                object->className = classBuffer;
                changed = true;
            }

            changed |= ImGui::Checkbox("Visible", &object->visible);
            changed |= ImGui::Checkbox("Locked", &object->locked);
            changed |= ImGui::InputFloat2("Position", &object->transform.position.x);
            changed |= ImGui::InputFloat("Rotation", &object->transform.rotationDegrees);
            changed |= ImGui::InputFloat2("Scale", &object->transform.scale.x);
            if (object->kind == SceneObjectKind::AssetInstance)
            {
                changed |= ImGui::InputFloat2("Size", &object->size.x);
            }

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
        const AssetRecord* asset = assetRegistry.FindAssetById(selection.assetId);
        if (asset)
        {
            ImGui::TextWrapped("Asset: %s", asset->name.c_str());
            ImGui::Separator();
            ImGui::Text("ID: %s", asset->id.c_str());
            ImGui::Text("Type: %s", AssetRegistry::TypeName(asset->type));
            ImGui::TextWrapped("Path: %s", RelativeAssetLabel(asset->absolutePath).c_str());
            if (asset->type == AssetType::Texture)
            {
                TexturePreview* preview = textureCache.GetTexture(*asset);
                if (preview && preview->texture)
                {
                    ImGui::Image(ToImTextureId(preview->texture), FitPreviewSize(preview->width, preview->height, 220.0f, 160.0f));
                    ImGui::TextDisabled("%d x %d", preview->width, preview->height);
                }
            }
            ImGui::TextDisabled(
                AssetRegistry::CanInstantiate(asset->type) ?
                    "Drag into Scene View to place." :
                    "Indexed, not placeable yet.");
        }
        else
        {
            ImGui::TextWrapped("Missing asset: %s", selection.assetId.c_str());
        }
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
    assetRegistry.ScanRoots(BuildAssetRoots());
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

        if (entry.is_directory(error))
        {
            assetEntries.push_back(AssetEntry{entry.path(), true, nullptr});
        }
    }

    for (const AssetRecord* asset : assetRegistry.GetAssetsInDirectory(currentAssetPath))
    {
        if (asset)
        {
            assetEntries.push_back(AssetEntry{asset->absolutePath, false, asset});
        }
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

void EditorApplication::SwitchContentRoot(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return;
    }

    std::error_code error;
    if (!std::filesystem::exists(path, error) || !std::filesystem::is_directory(path, error))
    {
        return;
    }

    contentRoot = std::filesystem::weakly_canonical(path, error);
    if (contentRoot.empty())
    {
        contentRoot = path;
    }
    currentAssetPath = contentRoot;
    RefreshAssets();
}

bool EditorApplication::DeleteSelectedSceneObject()
{
    const EditorSelection& selection = selectionService.GetSelection();
    if (selection.type != EditorSelectionType::SceneObject)
    {
        return false;
    }

    const SceneObject* object = sceneDocument.FindObject(selection.objectId);
    if (!object)
    {
        selectionService.Clear();
        return false;
    }

    const std::string objectName = object->name;
    if (!sceneDocument.RemoveObject(selection.objectId))
    {
        LogBus::Add(LogLevel::Warning, "Editor", "Selected scene object cannot be deleted: " + objectName);
        return false;
    }

    selectionService.Clear();
    LogBus::Add(LogLevel::Info, "Editor", "Deleted scene object: " + objectName);
    return true;
}

bool EditorApplication::SaveCurrentScene()
{
    if (currentScenePath.empty())
    {
        currentScenePath = DefaultScenePath();
    }

    std::string error;
    if (!sceneDocument.SaveToFile(currentScenePath, &error))
    {
        LogBus::Add(LogLevel::Error, "Editor", "Scene save failed: " + error);
        return false;
    }

    RefreshAssets();
    LogBus::Add(LogLevel::Info, "Editor", "Scene saved: " + RelativeAssetLabel(currentScenePath));
    return true;
}

bool EditorApplication::OpenSceneFile(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return false;
    }

    std::string error;
    if (!sceneDocument.LoadFromFile(path, &error))
    {
        LogBus::Add(LogLevel::Error, "Editor", "Scene open failed: " + error);
        return false;
    }

    currentScenePath = path;
    selectionService.Clear();
    for (const SceneObject& object : sceneDocument.GetObjects())
    {
        if (object.kind == SceneObjectKind::AssetInstance)
        {
            selectionService.SelectSceneObject(object.id);
            break;
        }
    }
    if (selectionService.GetSelection().type == EditorSelectionType::None && !sceneDocument.GetObjects().empty())
    {
        selectionService.SelectSceneObject(sceneDocument.GetObjects().front().id);
    }

    RefreshAssets();
    LogBus::Add(LogLevel::Info, "Editor", "Scene opened: " + RelativeAssetLabel(path));
    return true;
}

std::filesystem::path EditorApplication::DefaultScenePath() const
{
    if (projectContentRoot.empty())
    {
        return {};
    }

    return projectContentRoot / "Scenes" / "PlatformerTest.amber.scene";
}

std::vector<AssetRoot> EditorApplication::BuildAssetRoots() const
{
    std::vector<AssetRoot> roots;
    if (!projectContentRoot.empty())
    {
        roots.push_back(AssetRoot{"Project", projectContentRoot});
    }
    if (!engineContentRoot.empty())
    {
        roots.push_back(AssetRoot{"Engine", engineContentRoot});
    }

    return roots;
}

std::string EditorApplication::RelativeAssetLabel(const std::filesystem::path& path) const
{
    for (const AssetRoot& root : assetRegistry.GetRoots())
    {
        std::error_code error;
        const std::filesystem::path relative = std::filesystem::relative(path, root.path, error);
        if (error || relative.empty())
        {
            continue;
        }

        const std::string generic = relative.generic_string();
        if (generic == ".")
        {
            return root.name;
        }
        if (generic.rfind("../", 0) == 0 || generic == "..")
        {
            continue;
        }

        return root.name + "/" + generic;
    }

    return path.generic_string();
}

}
