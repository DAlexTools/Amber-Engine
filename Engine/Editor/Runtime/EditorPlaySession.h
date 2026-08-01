#ifndef AMBER_EDITOR_EDITOR_PLAY_SESSION_H
#define AMBER_EDITOR_EDITOR_PLAY_SESSION_H

#include "Runtime/GameModuleResolver.h"
#include "Scene/SceneDocument.h"
#include "EntityComponentSystem/ECS.h"
#include "Game/GameModuleInterface.h"
#include "Game/RuntimeWorld.h"
#include "Scene/Object.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace AE::Editor
{

struct PlayInPIERequest
{
	std::string projectName;
	std::filesystem::path projectRoot;
	std::filesystem::path scenePath;
	std::string gameModuleTarget;
	std::string playTarget;
	AE::EGameModuleRunMode RunMode = AE::EGameModuleRunMode::Play;
};

class EditorPlaySession
{
public:
	~EditorPlaySession();

	void Update();
	void Render(void* nativeRenderContext = nullptr);
	bool PlayInPIE(const PlayInPIERequest& request, const SceneDocument& editScene);
	void Stop();
	void SetPaused(bool isPaused);

	bool IsPlaying() const;
	bool IsPaused() const;
	SceneDocument* GetRuntimeSceneDocument();
	const SceneDocument* GetRuntimeSceneDocument() const;
	Registry* GetRuntimeRegistry();
	const Registry* GetRuntimeRegistry() const;
	SizeT GetRuntimeObjectCount() const;
	uint64 GetFrameCount() const;
	uint64 GetRenderCount() const;
	const char* GetRuntimeModuleName() const;
	bool IsRuntimeModuleDynamic() const;
	AE::EGameModuleRunMode GetRunMode() const;
	bool IsSimulating() const;
	const char* GetRunModeName() const;
	const std::string& GetRequestedGameModuleTarget() const;

private:
	void DestroyRuntimeWorld();

	PlayInPIERequest activeRequest;
	SceneDocument runtimeSceneDocument;
	AE::Scene::Document runtimeDocument;
	GameModuleResolver gameModuleResolver;
	std::unique_ptr<LoadedGameModule> activeGameModule;
	std::unique_ptr<AE::RuntimeWorld> runtimeWorld;
	uint64 frameCount = 0;
	uint64 renderCount = 0;
	bool gameModuleStarted = false;
	bool playing = false;
	bool paused = false;
	AE::EGameModuleRunMode ActiveRunMode = AE::EGameModuleRunMode::Play;
};

} // namespace AE::Editor

#endif
