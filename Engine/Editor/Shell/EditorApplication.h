#ifndef AMBER_EDITOR_SHELL_EDITOR_APPLICATION_H
#define AMBER_EDITOR_SHELL_EDITOR_APPLICATION_H

#include "ActorTypeRegistry.h"
#include "AssetRegistry.h"
#include "EditorViewport.h"
#include "EditorPlaySession.h"
#include "ProjectDescriptor.h"
#include "SceneDocument.h"
#include "SelectionService.h"
#include "TextureCache.h"
#include "OutputLogWidget.h"

#include <SDL2/SDL.h>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace AE::Editor
{

class EditorApplication
{
public:
    int Run(const std::filesystem::path& startupProjectFile = {});
    bool RunSmokeTest(const std::filesystem::path& startupProjectFile = {});

private:
    enum class EditorDockLayoutPreset
    {
        Default,
        FocusScene,
        ContentEditing
    };

    struct AssetEntry
    {
        std::filesystem::path path;
        bool directory = false;
        const AssetRecord* asset = nullptr;
    };

    bool Initialize(bool hiddenWindow);
    void Shutdown();
    void PollEvents();
    void BeginFrame();
    void RenderFrame();
    void ApplyStyle();

    void DrawLayout();
    void DrawEditorDockspace(float x, float y, float width, float height);
    void QueueDockLayout(EditorDockLayoutPreset preset);
    void RebuildDockLayout(ImGuiID dockspaceId, const ImVec2& dockspaceSize);
    void DrawMainMenuBar();
    void DrawToolbar(float menuHeight, float toolbarHeight);
    void DrawSceneView(float x, float y, float width, float height);
    void DrawActorPalette(float x, float y, float width, float height);
    void DrawAssetBrowser(float x, float y, float width, float height);
    void DrawSceneOutliner(float x, float y, float width, float height);
    void DrawDetailsPanel(float x, float y, float width, float height);
    void DrawOutputLog(float x, float y, float width, float height);
    void DrawProjectBrowser();

    void RefreshAssets();
    void OpenAssetDirectory(const std::filesystem::path& path);
    void SwitchContentRoot(const std::filesystem::path& path);
    void RebuildActorTypeRegistry();
    bool CreateProjectFromBrowser(bool generateSolutionAfterCreate = false);
    bool GenerateSolutionForActiveProject();
    bool OpenProjectFile(const std::filesystem::path& path);
    bool OpenLegacyWorkspaceProject();
    bool ApplyProjectDescriptor(const ProjectDescriptor& descriptor);
    bool DeleteSelectedSceneObject();
    bool DeleteSceneObject(uint32 ObjectId);
    bool DuplicateSceneObject(uint32 ObjectId);
    SceneObject& AddActorAt(const FActorTypeDefinition& ActorType, EditorVec2 WorldPosition);
    SceneObject& AddBoxAt(EditorVec2 WorldPosition);
    SceneObject& AddCircleAt(EditorVec2 WorldPosition);
    bool PlaceAssetAt(const AssetRecord& Asset, EditorVec2 WorldPosition);
    bool PlaceAssetAtViewCenter(const AssetRecord& Asset);
    void DrawActorPlacementMenu(EditorVec2 WorldPosition, bool CanEditScene);
    void DrawSceneContextMenu(EditorVec2 WorldPosition);
    void DrawSceneObjectContextMenu(uint32 ObjectId);
    bool SaveCurrentScene();
    bool OpenSceneFile(const std::filesystem::path& path);
    void FocusSelectedSceneObject();
    bool StartPlaySession(AE::EGameModuleRunMode RunMode);
    bool PlayInPIE();
    bool SimulateInEditor();
    std::filesystem::path FindEngineRoot() const;
    std::filesystem::path DefaultScenePath() const;
    std::vector<AssetRoot> BuildAssetRoots() const;
    std::string RelativeAssetLabel(const std::filesystem::path& path) const;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool imguiReady = false;
    bool imageSystemInitialized = false;
    bool running = false;
    int windowWidth = 1280;
    int windowHeight = 720;

    bool showAssetBrowser = true;
    bool showSceneOutliner = true;
    bool showDetails = true;
    bool showOutputLog = true;
    bool ShowActorPalette = true;
    bool showProjectBrowser = true;
    bool activeProjectLoaded = false;
    bool dockLayoutInitialized = false;
    bool rebuildDockLayout = false;
    EditorDockLayoutPreset pendingDockLayoutPreset = EditorDockLayoutPreset::Default;
    std::string imguiIniFilename;

    std::array<char, 128> newProjectName{};
    std::array<char, 512> newProjectLocation{};
    std::array<char, 512> openProjectPath{};
    std::array<char, 128> ActorPaletteSearch{};
    int newProjectTemplateIndex = 0;
    bool projectBrowserStatusIsError = false;
    std::string projectBrowserStatus;

    ProjectDescriptor activeProject;
    std::filesystem::path projectContentRoot;
    std::filesystem::path engineContentRoot;
    std::filesystem::path contentRoot;
    std::filesystem::path currentAssetPath;
    std::filesystem::path currentScenePath;
    std::vector<AssetEntry> assetEntries;
    EditorViewport::EContextMenuTarget SceneContextTarget = EditorViewport::EContextMenuTarget::Scene;
    uint32 SceneContextObjectId = 0;
    EditorVec2 SceneContextWorldPosition;

    AssetRegistry assetRegistry;
    TextureCache textureCache;
    FActorTypeRegistry actorTypeRegistry;
    SceneDocument sceneDocument;
    SelectionService selectionService;
    EditorViewport editorViewport;
    EditorPlaySession playSession;
    EditorTool activeTool = EditorTool::Move;
    OutputLogWidget outputLog;
};

}

#endif
