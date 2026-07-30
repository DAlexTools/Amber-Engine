#include "EditorPlaySession.h"

#include "Game/GameModuleInterface.h"
#include "Game/RuntimeWorld.h"
#include "Logging/LogBus.h"

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

	if (runtimeWorld)
	{
		Registry& registry = runtimeWorld->GetRegistry();
		if (activeGameModule)
		{
			AE::GameModuleTickContext tickContext{registry, 1.0f / 60.0f, frameCount, ActiveRunMode};
			activeGameModule->Get()->Tick(tickContext);
		}
		registry.Update();
	}
	++frameCount;
}

void EditorPlaySession::Render(void* nativeRenderContext)
{
	if (!playing || !runtimeWorld || !activeGameModule)
	{
		return;
	}

	AE::GameModuleRenderContext renderContext{runtimeWorld->GetRegistry(), frameCount, nativeRenderContext};
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
	ActiveRunMode = request.RunMode;
	runtimeSceneDocument = editScene;
	runtimeSceneDocument.SetDirty(false);
	runtimeWorld = std::make_unique<AE::RuntimeWorld>();
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

	AE::RuntimeWorldBuildOptions buildOptions;
	buildOptions.registerGameModuleSceneObjects = activeGameModule && !activeGameModule->IsDynamic();
	if (activeGameModule && activeGameModule->IsDynamic())
	{
		LogBus::Add(
			LogLevel::Info,
			"Editor",
			"Dynamic PIE module loaded; scene object registration stays editor-side until the runtime ABI is shared.");
		LogBus::Add(
			LogLevel::Info,
			"Editor",
			"PIE module path: " + activeGameModule->GetLibraryPath().string());
	}

	std::string buildError;
	if (!AE::BuildRuntimeWorld(
			runtimeDocument,
			activeGameModule ? activeGameModule->Get() : nullptr,
			*runtimeWorld,
			buildOptions,
			&buildError))
	{
		DestroyRuntimeWorld();
		activeRequest = {};
		runtimeDocument = {};
		runtimeSceneDocument.NewScene();
		runtimeSceneDocument.SetDirty(false);
		ActiveRunMode = AE::EGameModuleRunMode::Play;
		LogBus::Add(LogLevel::Error, "Editor", buildError.empty() ? "PIE runtime world build failed." : buildError);
		return false;
	}

	if (activeGameModule)
	{
		std::string error;
		AE::GameModuleStartContext startContext{
			activeRequest.projectName,
			activeRequest.projectRoot,
			activeRequest.scenePath,
			runtimeDocument,
			runtimeWorld->GetRegistry(),
			runtimeWorld->GetObjectFactory(),
			runtimeWorld->GetSceneObjects(),
			ActiveRunMode};
		if (!activeGameModule->Get()->StartPlay(startContext, &error))
		{
			DestroyRuntimeWorld();
			activeRequest = {};
			runtimeDocument = {};
			runtimeSceneDocument.NewScene();
			runtimeSceneDocument.SetDirty(false);
			ActiveRunMode = AE::EGameModuleRunMode::Play;
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
		std::string(GetRunModeName()) + " session started for " + activeRequest.projectName +
			" using game module " + std::string(GetRuntimeModuleName()) +
			". Runtime objects: " + std::to_string(GetRuntimeObjectCount()));
	return true;
}

void EditorPlaySession::Stop()
{
	if (!playing && (!runtimeWorld || runtimeWorld->GetObjectCount() == 0))
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
	ActiveRunMode = AE::EGameModuleRunMode::Play;
	LogBus::Add(LogLevel::Info, "Editor", "Runtime session stopped.");
}

void EditorPlaySession::SetPaused(bool isPaused)
{
	if (!playing)
	{
		paused = false;
		return;
	}

	paused = isPaused;
	LogBus::Add(LogLevel::Info, "Editor", paused ? "Runtime session paused." : "Runtime session resumed.");
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

Registry* EditorPlaySession::GetRuntimeRegistry()
{
	return playing && runtimeWorld ? &runtimeWorld->GetRegistry() : nullptr;
}

const Registry* EditorPlaySession::GetRuntimeRegistry() const
{
	return playing && runtimeWorld ? &runtimeWorld->GetRegistry() : nullptr;
}

SizeT EditorPlaySession::GetRuntimeObjectCount() const
{
	return runtimeWorld ? runtimeWorld->GetObjectCount() : 0;
}

uint64 EditorPlaySession::GetFrameCount() const
{
	return frameCount;
}

uint64 EditorPlaySession::GetRenderCount() const
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

AE::EGameModuleRunMode EditorPlaySession::GetRunMode() const
{
	return playing ? ActiveRunMode : AE::EGameModuleRunMode::Play;
}

bool EditorPlaySession::IsSimulating() const
{
	return playing && ActiveRunMode == AE::EGameModuleRunMode::Simulate;
}

const char* EditorPlaySession::GetRunModeName() const
{
	return ActiveRunMode == AE::EGameModuleRunMode::Simulate ? "Simulate" : "PIE";
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

	if (runtimeWorld)
	{
		runtimeWorld->DestroyObjects();
	}

	runtimeWorld.reset();
	activeGameModule.reset();
	gameModuleStarted = false;
}

} // namespace AE::Editor
