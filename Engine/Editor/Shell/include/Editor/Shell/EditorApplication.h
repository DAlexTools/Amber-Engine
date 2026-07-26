#ifndef AMBER_EDITOR_SHELL_EDITOR_APPLICATION_H
#define AMBER_EDITOR_SHELL_EDITOR_APPLICATION_H

#include "Editor/Shell/AssetRegistry.h"
#include "Editor/Shell/EditorViewport.h"
#include "Editor/Shell/EditorPlaySession.h"
#include "Editor/Shell/ProjectDescriptor.h"
#include "Editor/Shell/SceneDocument.h"
#include "Editor/Shell/SelectionService.h"
#include "Editor/Shell/TextureCache.h"
#include "Editor/OutputLog/OutputLogWidget.h"

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
    void DrawMainMenuBar();
    void DrawToolbar(float menuHeight, float toolbarHeight);
    void DrawSceneView(float x, float y, float width, float height);
    void DrawAssetBrowser(float x, float y, float width, float height);
    void DrawSceneOutliner(float x, float y, float width, float height);
    void DrawDetailsPanel(float x, float y, float width, float height);
    void DrawOutputLog(float x, float y, float width, float height);
    void DrawProjectBrowser();

    void RefreshAssets();
    void OpenAssetDirectory(const std::filesystem::path& path);
    void SwitchContentRoot(const std::filesystem::path& path);
    bool CreateProjectFromBrowser(bool generateSolutionAfterCreate = false);
    bool GenerateSolutionForActiveProject();
    bool OpenProjectFile(const std::filesystem::path& path);
    bool OpenLegacyWorkspaceProject();
    bool ApplyProjectDescriptor(const ProjectDescriptor& descriptor);
    bool DeleteSelectedSceneObject();
    bool SaveCurrentScene();
    bool OpenSceneFile(const std::filesystem::path& path);
    bool PlayInPIE();
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
    bool showProjectBrowser = true;
    bool activeProjectLoaded = false;

    std::array<char, 128> newProjectName{};
    std::array<char, 512> newProjectLocation{};
    std::array<char, 512> openProjectPath{};
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

    AssetRegistry assetRegistry;
    TextureCache textureCache;
    SceneDocument sceneDocument;
    SelectionService selectionService;
    EditorViewport editorViewport;
    EditorPlaySession playSession;
    EditorTool activeTool = EditorTool::Move;
    OutputLogWidget outputLog;
};

}

#endif
