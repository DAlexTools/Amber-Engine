#include "EditorPlaySession.h"

#include "Game/GameModuleInterface.h"
#include "Logging/LogBus.h"
#include "Scene/Object.h"
#include "Scene/ObjectFactory.h"

namespace AE::Editor
{

EditorPlaySession::~EditorPlaySession()
{
    Stop();
}

void EditorPlaySession::Update()
{
    if (!playing || paused)
    {
        return;
    }

    if (runtimeRegistry)
    {
        if (activeGameModule)
        {
            AE::GameModuleTickContext tickContext{*runtimeRegistry, 1.0f / 60.0f, frameCount};
            activeGameModule->Get()->Tick(tickContext);
        }
        runtimeRegistry->Update();
    }
    ++frameCount;
}

void EditorPlaySession::Render(void* nativeRenderContext)
{
    if (!playing || !runtimeRegistry || !activeGameModule)
    {
        return;
    }

    AE::GameModuleRenderContext renderContext{*runtimeRegistry, frameCount, nativeRenderContext};
    activeGameModule->Get()->Render(renderContext);
    ++renderCount;
}

bool EditorPlaySession::PlayInPIE(const PlayInPIERequest& request, const SceneDocument& editScene)
{
    if (playing)
    {
        LogBus::Add(LogLevel::Warning, "Editor", "PIE session is already running.");
        return true;
    }

    activeRequest = request;
    runtimeSceneDocument = editScene;
    runtimeSceneDocument.SetDirty(false);
    runtimeRegistry = std::make_unique<Registry>();
    runtimeDocument = editScene.ToRuntimeDocument();
    std::string resolverWarning;
    activeGameModule = gameModuleResolver.Resolve(request.gameModuleTarget, request.projectRoot, &resolverWarning);
    if (!resolverWarning.empty())
    {
        LogBus::Add(LogLevel::Warning, "Editor", resolverWarning);
    }
    frameCount = 0;
    renderCount = 0;
    gameModuleStarted = false;

    AE::Scene::ObjectFactory factory;
    if (activeGameModule && !activeGameModule->IsDynamic())
    {
        activeGameModule->Get()->RegisterSceneObjects(factory);
    }
    else if (activeGameModule && activeGameModule->IsDynamic())
    {
        LogBus::Add(
            LogLevel::Info,
            "Editor",
            "Dynamic PIE module loaded; scene object registration stays editor-side until the runtime ABI is shared.");
    }

    runtimeObjects = factory.CreateObjects(runtimeDocument, runtimeRegistry.get());
    if (runtimeRegistry)
    {
        runtimeRegistry->Update();
    }

    if (activeGameModule)
    {
        std::string error;
        AE::GameModuleStartContext startContext{
            activeRequest.projectName,
            activeRequest.projectRoot,
            activeRequest.scenePath,
            runtimeDocument,
            *runtimeRegistry,
            factory,
            runtimeObjects
        };
        if (!activeGameModule->Get()->StartPlay(startContext, &error))
        {
            DestroyRuntimeWorld();
            activeRequest = {};
            runtimeDocument = {};
            runtimeSceneDocument.NewScene();
            runtimeSceneDocument.SetDirty(false);
            LogBus::Add(LogLevel::Error, "Editor", "PIE game module start failed: " + error);
            return false;
        }
        gameModuleStarted = true;
    }

    playing = true;
    paused = false;

    LogBus::Add(
        LogLevel::Info,
        "Editor",
        "PIE session started for " + activeRequest.projectName +
            " using game module " + std::string(GetRuntimeModuleName()) +
            ". Runtime objects: " + std::to_string(runtimeObjects.size()));
    return true;
}

void EditorPlaySession::Stop()
{
    if (!playing && runtimeObjects.empty() && !runtimeRegistry)
    {
        paused = false;
        frameCount = 0;
        return;
    }

    DestroyRuntimeWorld();
    activeRequest = {};
    runtimeDocument = {};
    runtimeSceneDocument.NewScene();
    runtimeSceneDocument.SetDirty(false);
    playing = false;
    paused = false;
    frameCount = 0;
    renderCount = 0;
    gameModuleStarted = false;
    LogBus::Add(LogLevel::Info, "Editor", "PIE session stopped.");
}

void EditorPlaySession::SetPaused(bool isPaused)
{
    if (!playing)
    {
        paused = false;
        return;
    }

    paused = isPaused;
    LogBus::Add(LogLevel::Info, "Editor", paused ? "PIE session paused." : "PIE session resumed.");
}

bool EditorPlaySession::IsPlaying() const
{
    return playing;
}

bool EditorPlaySession::IsPaused() const
{
    return paused;
}

SceneDocument* EditorPlaySession::GetRuntimeSceneDocument()
{
    return playing ? &runtimeSceneDocument : nullptr;
}

const SceneDocument* EditorPlaySession::GetRuntimeSceneDocument() const
{
    return playing ? &runtimeSceneDocument : nullptr;
}

std::size_t EditorPlaySession::GetRuntimeObjectCount() const
{
    return runtimeObjects.size();
}

unsigned long EditorPlaySession::GetFrameCount() const
{
    return frameCount;
}

unsigned long EditorPlaySession::GetRenderCount() const
{
    return renderCount;
}

const char* EditorPlaySession::GetRuntimeModuleName() const
{
    return activeGameModule && activeGameModule->Get() ? activeGameModule->Get()->GetName() : "";
}

bool EditorPlaySession::IsRuntimeModuleDynamic() const
{
    return activeGameModule && activeGameModule->IsDynamic();
}

const std::string& EditorPlaySession::GetRequestedGameModuleTarget() const
{
    return activeRequest.gameModuleTarget;
}

void EditorPlaySession::DestroyRuntimeWorld()
{
    if (activeGameModule && gameModuleStarted)
    {
        activeGameModule->Get()->StopPlay();
    }

    for (std::unique_ptr<AE::Scene::Object>& object : runtimeObjects)
    {
        if (object)
        {
            object->OnDestroy();
        }
    }

    runtimeObjects.clear();
    runtimeRegistry.reset();
    activeGameModule.reset();
    gameModuleStarted = false;
}

}
