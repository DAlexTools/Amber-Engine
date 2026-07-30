#include "EditorApplication.h"

#include "Core/Platform/PlatformTypes.h"
#include "ProjectGenerator.h"
#include "Logging/LogBus.h"
#include <SDL2/SDL_image.h>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_sdl.h"
#include "imgui_internal.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <system_error>

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
			current / "AmberEngine" / relativeContentPath};

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

bool HasFixedBounds(float width, float height)
{
	return width > 0.0f && height > 0.0f;
}

ImGuiWindowFlags PanelFlags(bool fixedBounds)
{
	if (fixedBounds)
	{
		return FixedPanelFlags();
	}

	return ImGuiWindowFlags_NoCollapse;
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
		std::max(24.0f, height * scale)};
}

bool ClampTransformScale(EditorVec2& Scale)
{
	constexpr float MinimumScale = 0.05f;
	bool Changed = false;
	if (Scale.x < MinimumScale)
	{
		Scale.x = MinimumScale;
		Changed = true;
	}
	if (Scale.y < MinimumScale)
	{
		Scale.y = MinimumScale;
		Changed = true;
	}
	return Changed;
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

std::string FormatFloatProperty(float Value);

bool ContainsText(const std::string& Text, const std::string& Needle)
{
	if (Needle.empty())
	{
		return true;
	}

	std::string LowerText = Text;
	std::string LowerNeedle = Needle;
	std::transform(LowerText.begin(), LowerText.end(), LowerText.begin(), [](unsigned char Character)
				   { return static_cast<char>(std::tolower(Character)); });
	std::transform(LowerNeedle.begin(), LowerNeedle.end(), LowerNeedle.begin(), [](unsigned char Character)
				   { return static_cast<char>(std::tolower(Character)); });
	return LowerText.find(LowerNeedle) != std::string::npos;
}

bool ActorTypeMatchesFilter(const FActorTypeDefinition& ActorType, const std::string& Filter)
{
	return ContainsText(ActorType.DisplayName, Filter) ||
		   ContainsText(ActorType.ClassName, Filter) ||
		   ContainsText(ActorType.Category, Filter);
}

ImU32 ActorTypeFillColor(const FActorTypeDefinition& ActorType)
{
	return IM_COL32(ActorType.FillColor.R, ActorType.FillColor.G, ActorType.FillColor.B, ActorType.FillColor.A);
}

ImU32 ActorTypeOutlineColor(const FActorTypeDefinition& ActorType)
{
	return IM_COL32(ActorType.OutlineColor.R, ActorType.OutlineColor.G, ActorType.OutlineColor.B, ActorType.OutlineColor.A);
}

void DrawActorTypeSwatch(const FActorTypeDefinition& ActorType)
{
	const ImVec2 Position = ImGui::GetCursorScreenPos();
	const ImVec2 Size(18.0f, 18.0f);
	ImGui::Dummy(Size);

	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	const ImU32 FillColor = ActorTypeFillColor(ActorType);
	const ImU32 OutlineColor = ActorTypeOutlineColor(ActorType);
	if (ActorType.Kind == SceneObjectKind::Circle)
	{
		const ImVec2 Center(Position.x + Size.x * 0.5f, Position.y + Size.y * 0.5f);
		DrawList->AddCircleFilled(Center, 7.0f, FillColor, 20);
		DrawList->AddCircle(Center, 7.0f, OutlineColor, 20, 1.0f);
	}
	else
	{
		DrawList->AddRectFilled(
			ImVec2(Position.x + 2.0f, Position.y + 4.0f),
			ImVec2(Position.x + Size.x - 2.0f, Position.y + Size.y - 4.0f),
			FillColor,
			2.0f);
		DrawList->AddRect(
			ImVec2(Position.x + 2.0f, Position.y + 4.0f),
			ImVec2(Position.x + Size.x - 2.0f, Position.y + Size.y - 4.0f),
			OutlineColor,
			2.0f,
			0,
			1.0f);
	}
}

std::string DefaultPropertyValue(
	const SceneObject& Object,
	const FActorComponentSchema& ComponentSchema,
	const FActorComponentPropertySchema& PropertySchema)
{
	(void)Object;
	(void)ComponentSchema;
	return PropertySchema.DefaultValue;
}

AE::Scene::ComponentData& FindOrAddComponent(SceneObject& Object, const FActorComponentSchema& Schema)
{
	for (AE::Scene::ComponentData& Component : Object.components)
	{
		if (Component.name == Schema.Name)
		{
			return Component;
		}
	}

	Object.components.push_back(AE::Scene::ComponentData{Schema.Name, {}});
	return Object.components.back();
}

AE::Scene::ComponentPropertyData& FindOrAddProperty(
	AE::Scene::ComponentData& Component,
	const FActorComponentPropertySchema& Schema,
	const std::string& DefaultValue)
{
	for (AE::Scene::ComponentPropertyData& Property : Component.properties)
	{
		if (Property.name == Schema.Name)
		{
			Property.type = Schema.Type;
			if (Property.value.empty())
			{
				Property.value = DefaultValue;
			}
			return Property;
		}
	}

	Component.properties.push_back(AE::Scene::ComponentPropertyData{Schema.Name, Schema.Type, DefaultValue});
	return Component.properties.back();
}

void SynchronizeSceneObjectComponents(SceneObject& Object, const FActorTypeRegistry& ActorTypeRegistry)
{
	Object.components.erase(
		std::remove_if(
			Object.components.begin(),
			Object.components.end(),
			[&Object, &ActorTypeRegistry](const AE::Scene::ComponentData& Component)
			{
				return ActorTypeRegistry.IsManagedComponentName(Component.name) &&
					   !ActorTypeRegistry.IsComponentExpectedForClass(Object.className, Component.name);
			}),
		Object.components.end());

	const FActorTypeDefinition* ActorType = ActorTypeRegistry.FindByClassName(Object.className);
	if (!ActorType)
	{
		return;
	}

	for (const FActorComponentSchema& Schema : ActorType->Components)
	{
		AE::Scene::ComponentData& Component = FindOrAddComponent(Object, Schema);
		for (const FActorComponentPropertySchema& PropertySchema : Schema.Properties)
		{
			FindOrAddProperty(Component, PropertySchema, DefaultPropertyValue(Object, Schema, PropertySchema));
		}
	}
}

bool ParseBoolProperty(const std::string& Value, bool DefaultValue)
{
	if (Value == "1" || Value == "true" || Value == "True")
	{
		return true;
	}
	if (Value == "0" || Value == "false" || Value == "False")
	{
		return false;
	}

	return DefaultValue;
}

int ParseIntProperty(const std::string& Value, int DefaultValue)
{
	std::istringstream Stream(Value);
	int Result = DefaultValue;
	return (Stream >> Result) ? Result : DefaultValue;
}

float ParseFloatProperty(const std::string& Value, float DefaultValue)
{
	std::istringstream Stream(Value);
	float Result = DefaultValue;
	return (Stream >> Result) ? Result : DefaultValue;
}

std::string FormatFloatProperty(float Value)
{
	char Buffer[64] = {};
	std::snprintf(Buffer, sizeof(Buffer), "%.3f", Value);
	return Buffer;
}

bool DrawComponentProperty(AE::Scene::ComponentPropertyData& Property)
{
	switch (Property.type)
	{
	case AE::Scene::ComponentPropertyType::Bool:
	{
		bool Value = ParseBoolProperty(Property.value, false);
		if (ImGui::Checkbox(Property.name.c_str(), &Value))
		{
			Property.value = Value ? "true" : "false";
			return true;
		}
		return false;
	}
	case AE::Scene::ComponentPropertyType::Int:
	{
		int Value = ParseIntProperty(Property.value, 0);
		if (ImGui::InputInt(Property.name.c_str(), &Value))
		{
			Property.value = std::to_string(Value);
			return true;
		}
		return false;
	}
	case AE::Scene::ComponentPropertyType::Float:
	{
		float Value = ParseFloatProperty(Property.value, 0.0f);
		if (ImGui::InputFloat(Property.name.c_str(), &Value))
		{
			Property.value = FormatFloatProperty(Value);
			return true;
		}
		return false;
	}
	default:
	{
		char Buffer[256] = {};
		std::snprintf(Buffer, sizeof(Buffer), "%s", Property.value.c_str());
		if (ImGui::InputText(Property.name.c_str(), Buffer, sizeof(Buffer)))
		{
			Property.value = Buffer;
			return true;
		}
		return false;
	}
	}
}

bool DrawSceneObjectComponents(SceneObject& Object)
{
	if (Object.components.empty())
	{
		return false;
	}

	bool Changed = false;
	ImGui::Separator();
	if (ImGui::CollapsingHeader("ECS Components", ImGuiTreeNodeFlags_DefaultOpen))
	{
		for (AE::Scene::ComponentData& Component : Object.components)
		{
			ImGui::PushID(Component.name.c_str());
			if (ImGui::TreeNodeEx(Component.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				for (AE::Scene::ComponentPropertyData& Property : Component.properties)
				{
					Changed |= DrawComponentProperty(Property);
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}

	return Changed;
}

template <SizeT Size>
void CopyToBuffer(std::array<char, Size>& buffer, const std::string& value)
{
	buffer.fill('\0');
	const SizeT count = std::min(buffer.size() - 1, value.size());
	std::memcpy(buffer.data(), value.data(), count);
}

bool LooksLikeEngineRoot(const std::filesystem::path& path)
{
	if (path.empty())
	{
		return false;
	}
	std::error_code error;
	return std::filesystem::exists(path / "CMakeLists.txt", error) &&
		   std::filesystem::exists(path / "Engine" / "Runtime", error);
}

bool RunProjectGeneratorSmoke(const std::filesystem::path& engineRoot)
{
	const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
	const std::filesystem::path parent =
		std::filesystem::temp_directory_path() /
		("AmberEditorProjectSmoke_" + std::to_string(stamp));

	std::error_code cleanupError;
	std::filesystem::remove_all(parent, cleanupError);

	ProjectGenerationRequest request;
	request.projectName = "SmokeBlank";
	request.parentDirectory = parent;
	request.engineRoot = engineRoot;
	request.projectTemplate = ProjectTemplate::BlankCppGame;

	ProjectGenerationResult result;
	std::string error;
	if (!ProjectGenerator::CreateProject(request, result, &error))
	{
		std::cerr << "Project generator smoke failed: " << error << std::endl;
		return false;
	}

	ProjectDescriptor descriptor;
	if (!LoadProjectDescriptor(result.projectFilePath, descriptor, &error))
	{
		std::cerr << "Project descriptor smoke failed: " << error << std::endl;
		std::filesystem::remove_all(parent, cleanupError);
		return false;
	}

	const bool valid =
		descriptor.name == "SmokeBlank" &&
		descriptor.gameModuleTarget == "SmokeBlankModule" &&
		descriptor.playTarget == "SmokeBlankLauncher" &&
		descriptor.actorTypes == (std::filesystem::path("Config") / "ActorTypes.amberactors") &&
		descriptor.solutionPath == (std::filesystem::path("Builds") / "Editor" / "SmokeBlank.sln") &&
		ProjectGenerator::GetConfigureCommand(descriptor) == "cmake --preset editor" &&
		ProjectGenerator::GetExpectedSolutionPath(descriptor) == (result.projectRoot / "Builds" / "Editor" / "SmokeBlank.sln") &&
		std::filesystem::exists(result.projectRoot / "CMakeLists.txt") &&
		std::filesystem::exists(result.projectRoot / "CMakePresets.json") &&
		std::filesystem::exists(result.projectRoot / "Source" / "SmokeBlank" / "SmokeBlankModule.cpp") &&
		std::filesystem::exists(result.projectRoot / "Source" / "SmokeBlank" / "SmokeBlankModulePlugin.cpp") &&
		std::filesystem::exists(result.projectRoot / "Config" / "ActorTypes.amberactors") &&
		std::filesystem::exists(result.projectRoot / "Content" / "Scenes" / "Startup.amber.scene");

	std::filesystem::remove_all(parent, cleanupError);
	if (!valid)
	{
		std::cerr << "Project generator smoke failed: generated project is incomplete." << std::endl;
	}
	return valid;
}
} // namespace

int EditorApplication::Run(const std::filesystem::path& startupProjectFile)
{
	if (!Initialize(false))
	{
		return 1;
	}

	if (!startupProjectFile.empty())
	{
		CopyToBuffer(openProjectPath, startupProjectFile.string());
		if (!OpenProjectFile(startupProjectFile))
		{
			std::cerr << "Project open failed: " << startupProjectFile.string() << std::endl;
		}
	}

	running = true;
	while (running)
	{
		PollEvents();
		playSession.Update();
		BeginFrame();
		DrawLayout();
		RenderFrame();
	}

	Shutdown();
	return 0;
}

bool EditorApplication::RunSmokeTest(const std::filesystem::path& startupProjectFile)
{
	if (!Initialize(true))
	{
		return false;
	}

	if (!RunProjectGeneratorSmoke(FindEngineRoot()))
	{
		Shutdown();
		return false;
	}

	if (!startupProjectFile.empty())
	{
		if (!OpenProjectFile(startupProjectFile))
		{
			std::cerr << "Editor smoke failed: project file did not open: "
					  << startupProjectFile.string() << std::endl;
			Shutdown();
			return false;
		}
	}
	else if (!OpenLegacyWorkspaceProject())
	{
		Shutdown();
		return false;
	}

	const SizeT editObjectCount = sceneDocument.GetObjects().size();
	const bool editSceneDirty = sceneDocument.IsDirty();
	if (!PlayInPIE() || !playSession.IsPlaying() || playSession.GetRuntimeObjectCount() != editObjectCount)
	{
		std::cerr << "Editor PIE smoke failed: runtime world did not start." << std::endl;
		Shutdown();
		return false;
	}

	playSession.Update();
	playSession.Update();
	playSession.Render();
	if (playSession.GetFrameCount() < 2)
	{
		std::cerr << "Editor PIE smoke failed: runtime world did not tick." << std::endl;
		Shutdown();
		return false;
	}
	if (playSession.GetRenderCount() < 1 || std::string(playSession.GetRuntimeModuleName()).empty())
	{
		std::cerr << "Editor PIE smoke failed: game module render hook did not run." << std::endl;
		Shutdown();
		return false;
	}
	const std::string expectedModuleName = activeProject.gameModuleTarget;
	if (!playSession.IsRuntimeModuleDynamic() ||
		std::string(playSession.GetRuntimeModuleName()) != expectedModuleName)
	{
		std::cerr << "Editor PIE smoke failed: expected game module plugin was not loaded." << std::endl;
		Shutdown();
		return false;
	}

	playSession.SetPaused(true);
	const uint64 pausedFrameCount = playSession.GetFrameCount();
	playSession.Update();
	if (playSession.GetFrameCount() != pausedFrameCount)
	{
		std::cerr << "Editor PIE smoke failed: paused runtime world ticked." << std::endl;
		Shutdown();
		return false;
	}

	playSession.SetPaused(false);
	playSession.Stop();
	if (playSession.IsPlaying() ||
		playSession.GetRuntimeObjectCount() != 0 ||
		sceneDocument.GetObjects().size() != editObjectCount ||
		sceneDocument.IsDirty() != editSceneDirty)
	{
		std::cerr << "Editor PIE smoke failed: stop did not restore editor state." << std::endl;
		Shutdown();
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
	RebuildActorTypeRegistry();

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

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	if (!renderer)
	{
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
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
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigDockingWithShift = false;
	io.ConfigWindowsMoveFromTitleBarOnly = true;
	imguiIniFilename = (FindEngineRoot() / "imgui.ini").string();
	io.IniFilename = imguiIniFilename.c_str();
	ImGui::StyleColorsDark();
	ApplyStyle();
	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGuiSDL::Initialize(renderer, windowWidth, windowHeight);
	imguiReady = true;
	textureCache.Initialize(renderer);

	projectContentRoot = FindContentRoot("Content");
	if (projectContentRoot.empty())
	{
		projectContentRoot = std::filesystem::current_path() / "Content";
	}
	engineContentRoot = FindContentRoot(std::filesystem::path("Engine") / "Content");

	const std::filesystem::path defaultProjectsRoot = FindEngineRoot() / "Projects";
	CopyToBuffer(newProjectName, std::string("MyGame"));
	CopyToBuffer(newProjectLocation, defaultProjectsRoot.string());

	contentRoot = projectContentRoot;
	assetRegistry.ScanRoots(BuildAssetRoots());
	textureCache.SetAssetRoots(assetRegistry.GetRoots());
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
	playSession.Stop();
	editorViewport.ReleaseRenderResources();
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
			const bool altPressed = (event.key.keysym.mod & KMOD_ALT) != 0;
			const bool shiftPressed = (event.key.keysym.mod & KMOD_SHIFT) != 0;
			const bool textInputActive = imguiReady && ImGui::GetIO().WantTextInput;
			const bool editorShortcutAvailable = !textInputActive && !playSession.IsPlaying() && !ctrlPressed && !altPressed && !shiftPressed;
			if (key == SDLK_ESCAPE)
			{
				running = false;
			}
			else if (key == SDLK_s && ctrlPressed && !textInputActive)
			{
				SaveCurrentScene();
			}
			else if (key == SDLK_d && ctrlPressed && !textInputActive)
			{
				const EditorSelection& Selection = selectionService.GetSelection();
				if (Selection.type == EditorSelectionType::SceneObject)
				{
					DuplicateSceneObject(Selection.objectId);
				}
			}
			else if (key == SDLK_F5)
			{
				PlayInPIE();
			}
			else if (key == SDLK_F6)
			{
				playSession.SetPaused(!playSession.IsPaused());
			}
			else if (key == SDLK_F7)
			{
				playSession.Stop();
			}
			else if (key == SDLK_DELETE && !textInputActive)
			{
				DeleteSelectedSceneObject();
			}
			else if (key == SDLK_w && editorShortcutAvailable)
			{
				activeTool = EditorTool::Move;
				LogBus::Add(LogLevel::Info, "Editor", "Location tool active.");
			}
			else if (key == SDLK_e && editorShortcutAvailable)
			{
				activeTool = EditorTool::Rotate;
				LogBus::Add(LogLevel::Info, "Editor", "Rotate tool active.");
			}
			else if (key == SDLK_r && editorShortcutAvailable)
			{
				activeTool = EditorTool::Scale;
				LogBus::Add(LogLevel::Info, "Editor", "Scale tool active.");
			}
			else if (key == SDLK_f && editorShortcutAvailable)
			{
				FocusSelectedSceneObject();
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

	ImGui_ImplSDL2_NewFrame();
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
	if (showProjectBrowser && !activeProjectLoaded)
	{
		DrawProjectBrowser();
		return;
	}

	const float toolbarHeight = 42.0f;
	const float dockspaceTop = menuHeight + toolbarHeight;

	DrawToolbar(menuHeight, toolbarHeight);
	DrawEditorDockspace(
		0.0f,
		dockspaceTop,
		static_cast<float>(windowWidth),
		std::max(1.0f, static_cast<float>(windowHeight) - dockspaceTop));

	DrawSceneView(0.0f, 0.0f, 0.0f, 0.0f);

	if (ShowActorPalette)
	{
		DrawActorPalette(0.0f, 0.0f, 0.0f, 0.0f);
	}
	if (showAssetBrowser)
	{
		DrawAssetBrowser(0.0f, 0.0f, 0.0f, 0.0f);
	}
	if (showOutputLog)
	{
		DrawOutputLog(0.0f, 0.0f, 0.0f, 0.0f);
	}
	if (showSceneOutliner)
	{
		DrawSceneOutliner(0.0f, 0.0f, 0.0f, 0.0f);
	}
	if (showDetails)
	{
		DrawDetailsPanel(0.0f, 0.0f, 0.0f, 0.0f);
	}
}

void EditorApplication::DrawEditorDockspace(float x, float y, float width, float height)
{
	SetPanelBounds(x, y, width, height);

	const ImGuiWindowFlags hostFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::Begin("Amber Editor Dockspace", nullptr, hostFlags);
	ImGui::PopStyleVar(2);

	const ImGuiID dockspaceId = ImGui::GetID("AmberEditorDockspace");
	if (!dockLayoutInitialized)
	{
		dockLayoutInitialized = true;
		if (!ImGui::DockBuilderGetNode(dockspaceId))
		{
			QueueDockLayout(EditorDockLayoutPreset::Default);
		}
	}

	if (rebuildDockLayout)
	{
		RebuildDockLayout(dockspaceId, ImGui::GetContentRegionAvail());
		rebuildDockLayout = false;
	}

	ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
	ImGui::End();
}

void EditorApplication::QueueDockLayout(EditorDockLayoutPreset preset)
{
	pendingDockLayoutPreset = preset;
	rebuildDockLayout = true;

	showAssetBrowser = true;
	showSceneOutliner = true;
	showDetails = true;
	showOutputLog = true;
	ShowActorPalette = true;
}

void EditorApplication::RebuildDockLayout(ImGuiID dockspaceId, const ImVec2& dockspaceSize)
{
	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspaceId, dockspaceSize);

	ImGuiID mainDock = dockspaceId;
	ImGuiID rightDock = 0;
	ImGuiID bottomDock = 0;
	ImGuiID sceneDock = 0;
	ImGuiID assetDock = 0;
	ImGuiID outputDock = 0;
	ImGuiID outlinerDock = 0;
	ImGuiID detailsDock = 0;

	float rightRatio = 0.24f;
	float bottomRatio = 0.30f;
	float outputRatio = 0.40f;
	float detailsRatio = 0.54f;

	if (pendingDockLayoutPreset == EditorDockLayoutPreset::FocusScene)
	{
		rightRatio = 0.20f;
		bottomRatio = 0.22f;
		outputRatio = 0.34f;
		detailsRatio = 0.50f;
	}
	else if (pendingDockLayoutPreset == EditorDockLayoutPreset::ContentEditing)
	{
		rightRatio = 0.24f;
		bottomRatio = 0.42f;
		outputRatio = 0.30f;
		detailsRatio = 0.58f;
	}

	ImGui::DockBuilderSplitNode(mainDock, ImGuiDir_Right, rightRatio, &rightDock, &mainDock);
	ImGui::DockBuilderSplitNode(mainDock, ImGuiDir_Down, bottomRatio, &bottomDock, &sceneDock);
	ImGui::DockBuilderSplitNode(bottomDock, ImGuiDir_Right, outputRatio, &outputDock, &assetDock);
	ImGui::DockBuilderSplitNode(rightDock, ImGuiDir_Down, detailsRatio, &detailsDock, &outlinerDock);

	ImGui::DockBuilderDockWindow("Scene View", sceneDock);
	ImGui::DockBuilderDockWindow("Actor Palette", assetDock);
	ImGui::DockBuilderDockWindow("Asset Browser", assetDock);
	ImGui::DockBuilderDockWindow("Output Log", outputDock);
	ImGui::DockBuilderDockWindow("Scene Outliner", outlinerDock);
	ImGui::DockBuilderDockWindow("Details", detailsDock);
	ImGui::DockBuilderFinish(dockspaceId);
}

void EditorApplication::DrawMainMenuBar()
{
	if (!ImGui::BeginMainMenuBar())
	{
		return;
	}

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Project Browser"))
		{
			showProjectBrowser = true;
			activeProjectLoaded = false;
		}
		const bool canGenerateSolution = activeProjectLoaded && !activeProject.projectRoot.empty();
		if (ImGui::MenuItem("Generate Solution", nullptr, false, canGenerateSolution))
		{
			GenerateSolutionForActiveProject();
		}
		ImGui::Separator();
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
		const EditorSelection& Selection = selectionService.GetSelection();
		const bool CanModifyObject =
			Selection.type == EditorSelectionType::SceneObject &&
			!playSession.IsPlaying() &&
			sceneDocument.IsObjectRemovable(Selection.objectId);
		if (ImGui::MenuItem("Duplicate Selected", "Ctrl+D", false, CanModifyObject))
		{
			DuplicateSceneObject(Selection.objectId);
		}
		if (ImGui::MenuItem("Delete Selected", "Del", false, CanModifyObject))
		{
			DeleteSelectedSceneObject();
		}
		if (ImGui::MenuItem("Focus Selected", "F", false, Selection.type == EditorSelectionType::SceneObject && !playSession.IsPlaying()))
		{
			FocusSelectedSceneObject();
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Add"))
	{
		const bool CanEditScene = !playSession.IsPlaying() && activeProjectLoaded;
		DrawActorPlacementMenu(editorViewport.GetViewCenter(), CanEditScene);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Play"))
	{
		const bool Playing = playSession.IsPlaying();
		const bool Paused = playSession.IsPaused();
		if (ImGui::MenuItem("Play In PIE", "F5", false, !Playing))
		{
			PlayInPIE();
		}
		if (ImGui::MenuItem("Simulate", nullptr, false, !Playing))
		{
			SimulateInEditor();
		}
		if (ImGui::MenuItem(Paused ? "Resume View" : "Pause View", "F6", Paused, Playing))
		{
			playSession.SetPaused(!Paused);
		}
		if (ImGui::MenuItem("Stop", "F7", false, Playing))
		{
			playSession.Stop();
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Window"))
	{
		ImGui::MenuItem("Actor Palette", nullptr, &ShowActorPalette);
		ImGui::MenuItem("Asset Browser", nullptr, &showAssetBrowser);
		ImGui::MenuItem("Scene Outliner", nullptr, &showSceneOutliner);
		ImGui::MenuItem("Details", nullptr, &showDetails);
		ImGui::MenuItem("Output Log", nullptr, &showOutputLog);
		ImGui::Separator();
		if (ImGui::BeginMenu("Layouts"))
		{
			if (ImGui::MenuItem("Default Editor"))
			{
				QueueDockLayout(EditorDockLayoutPreset::Default);
			}
			if (ImGui::MenuItem("Focus Scene"))
			{
				QueueDockLayout(EditorDockLayoutPreset::FocusScene);
			}
			if (ImGui::MenuItem("Content Editing"))
			{
				QueueDockLayout(EditorDockLayoutPreset::ContentEditing);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}

void EditorApplication::DrawProjectBrowser()
{
	const float menuHeight = ImGui::GetFrameHeight();
	SetPanelBounds(0.0f, menuHeight, static_cast<float>(windowWidth), static_cast<float>(windowHeight) - menuHeight);
	ImGui::Begin(
		"Project Browser",
		nullptr,
		FixedPanelFlags() | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);

	ImGui::Text("Amber Project Browser");
	ImGui::Separator();

	const float columnWidth = std::max(320.0f, ImGui::GetContentRegionAvail().x * 0.48f);
	ImGui::BeginChild("NewProjectPanel", ImVec2(columnWidth, 0.0f), true);
	ImGui::Text("New Project");
	ImGui::Separator();
	ImGui::InputText("Name", newProjectName.data(), newProjectName.size());
	ImGui::InputText("Location", newProjectLocation.data(), newProjectLocation.size());

	const char* templates[] = {"Blank C++ Game"};
	ImGui::Combo("Template", &newProjectTemplateIndex, templates, 1);

	if (ImGui::Button("Create Project"))
	{
		CreateProjectFromBrowser(false);
	}
	ImGui::SameLine();
	if (ImGui::Button("Create + Generate Solution"))
	{
		CreateProjectFromBrowser(true);
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("OpenProjectPanel", ImVec2(0.0f, 0.0f), true);
	ImGui::Text("Open Project");
	ImGui::Separator();
	ImGui::InputText("Project File", openProjectPath.data(), openProjectPath.size());

	if (ImGui::Button("Open Project"))
	{
		OpenProjectFile(openProjectPath.data());
	}
	ImGui::SameLine();
	if (ImGui::Button("Open Current Workspace"))
	{
		OpenLegacyWorkspaceProject();
	}

	ImGui::Separator();
	ImGui::TextDisabled("Recent Projects");
	ImGui::TextDisabled("No recent projects yet.");

	if (!projectBrowserStatus.empty())
	{
		ImGui::Separator();
		const ImVec4 color = projectBrowserStatusIsError ? ImVec4(0.95f, 0.34f, 0.32f, 1.0f) : ImVec4(0.42f, 0.82f, 0.50f, 1.0f);
		ImGui::TextColored(color, "%s", projectBrowserStatus.c_str());
	}
	ImGui::EndChild();

	ImGui::End();
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
	if (ToolButton("Location", activeTool, EditorTool::Move))
	{
		LogBus::Add(LogLevel::Info, "Editor", "Location tool active.");
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
	const bool Playing = playSession.IsPlaying();
	const bool Paused = playSession.IsPaused();
	const bool Simulating = playSession.IsSimulating();
	const char* PlayButtonLabel = Playing ? (Paused ? "Resume" : "Pause") : "Play";
	if (ImGui::Button(PlayButtonLabel))
	{
		if (Playing)
		{
			playSession.SetPaused(!Paused);
		}
		else
		{
			PlayInPIE();
		}
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(Playing);
	if (ImGui::Button("Simulate"))
	{
		SimulateInEditor();
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(!Playing);
	if (ImGui::Button("Stop"))
	{
		playSession.Stop();
	}
	ImGui::EndDisabled();

	ImGui::SameLine();
	const char* state = Playing ? (Paused ? "Paused" : (Simulating ? "Simulating" : "Playing")) : "Editing";
	ImGui::TextDisabled("%s", state);

	ImGui::End();
}

void EditorApplication::DrawSceneView(float x, float y, float width, float height)
{
	const bool fixedBounds = HasFixedBounds(width, height);
	if (fixedBounds)
	{
		SetPanelBounds(x, y, width, height);
	}
	ImGui::Begin("Scene View", nullptr, PanelFlags(fixedBounds) | ImGuiWindowFlags_NoScrollbar);
	SceneDocument* viewportSceneDocument = &sceneDocument;
	if (SceneDocument* runtimeSceneDocument = playSession.GetRuntimeSceneDocument())
	{
		viewportSceneDocument = runtimeSceneDocument;
	}

	bool playRenderSubmitted = false;
	const std::optional<EditorViewport::FDropRequest> DropRequest =
		editorViewport.Draw(
			*viewportSceneDocument,
			selectionService,
			assetRegistry,
			textureCache,
			activeTool,
			window,
			renderer,
			activeProjectLoaded ? &activeProject : nullptr,
			&actorTypeRegistry,
			playSession.IsPlaying() ? EditorViewport::ViewportMode::PlayOutput : EditorViewport::ViewportMode::EditPreview,
			playSession.IsPaused(),
			playSession.GetRuntimeRegistry(),
			[&](AE::RuntimeRenderContextSDL& renderContext)
			{
				playSession.Render(&renderContext);
				playRenderSubmitted = true;
			});
	if (DropRequest && !playSession.IsPlaying())
	{
		if (DropRequest->Type == EditorViewport::EDropPayloadType::Asset)
		{
			const AssetRecord* Asset = assetRegistry.FindAssetById(DropRequest->PayloadId);
			if (!Asset || !PlaceAssetAt(*Asset, DropRequest->WorldPosition))
			{
				LogBus::Add(LogLevel::Warning, "Editor", "Dropped asset cannot be placed in the scene.");
			}
		}
		else if (DropRequest->Type == EditorViewport::EDropPayloadType::ActorType)
		{
			const FActorTypeDefinition* ActorType = actorTypeRegistry.FindByTypeId(DropRequest->PayloadId);
			if (ActorType)
			{
				AddActorAt(*ActorType, DropRequest->WorldPosition);
			}
			else
			{
				LogBus::Add(LogLevel::Warning, "Editor", "Dropped actor type is not registered: " + DropRequest->PayloadId);
			}
		}
	}
	if (const std::optional<EditorViewport::FContextMenuRequest> ContextRequest = editorViewport.ConsumeContextMenuRequest())
	{
		SceneContextTarget = ContextRequest->Target;
		SceneContextObjectId = ContextRequest->ObjectId;
		SceneContextWorldPosition = ContextRequest->WorldPosition;
		ImGui::OpenPopup("SceneViewContextMenu");
	}
	if (ImGui::BeginPopup("SceneViewContextMenu"))
	{
		if (SceneContextTarget == EditorViewport::EContextMenuTarget::SceneObject)
		{
			DrawSceneObjectContextMenu(SceneContextObjectId);
		}
		else
		{
			DrawSceneContextMenu(SceneContextWorldPosition);
		}
		ImGui::EndPopup();
	}
	if (playSession.IsPlaying() && !playRenderSubmitted)
	{
		playSession.Render();
	}
	ImGui::End();
}

void EditorApplication::DrawActorPalette(float x, float y, float width, float height)
{
	const bool FixedBounds = HasFixedBounds(width, height);
	if (FixedBounds)
	{
		SetPanelBounds(x, y, width, height);
	}
	ImGui::Begin("Actor Palette", &ShowActorPalette, PanelFlags(FixedBounds));

	ImGui::SetNextItemWidth(-1.0f);
	ImGui::InputText("Search", ActorPaletteSearch.data(), ActorPaletteSearch.size());
	ImGui::Separator();

	const std::string Filter = ActorPaletteSearch.data();
	const bool CanPlaceActor = activeProjectLoaded && !playSession.IsPlaying();
	std::string CurrentCategory;
	SizeT VisibleActorCount = 0u;

	ImGui::BeginChild("ActorTypeList", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
	for (const FActorTypeDefinition& ActorType : actorTypeRegistry.GetActorTypes())
	{
		if (!ActorTypeMatchesFilter(ActorType, Filter))
		{
			continue;
		}

		if (CurrentCategory != ActorType.Category)
		{
			if (!CurrentCategory.empty())
			{
				ImGui::Separator();
			}
			CurrentCategory = ActorType.Category;
			ImGui::TextDisabled("%s", CurrentCategory.empty() ? "Actors" : CurrentCategory.c_str());
		}

		++VisibleActorCount;
		ImGui::PushID(ActorType.TypeId.c_str());

		if (ImGui::SmallButton("+") && CanPlaceActor)
		{
			AddActorAt(ActorType, editorViewport.GetViewCenter());
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Place at view center");
		}
		ImGui::SameLine();
		DrawActorTypeSwatch(ActorType);
		ImGui::SameLine();

		if (ImGui::Selectable(ActorType.DisplayName.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick) &&
			CanPlaceActor &&
			ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			AddActorAt(ActorType, editorViewport.GetViewCenter());
		}
		if (CanPlaceActor && ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(
				"AMBER_ACTOR_TYPE",
				ActorType.TypeId.c_str(),
				ActorType.TypeId.size() + 1);
			DrawActorTypeSwatch(ActorType);
			ImGui::SameLine();
			ImGui::Text("%s", ActorType.DisplayName.c_str());
			ImGui::TextDisabled("%s", ActorType.ClassName.c_str());
			ImGui::EndDragDropSource();
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", ActorType.ClassName.c_str());
		}
		ImGui::OpenPopupOnItemClick("ActorTypeContext", ImGuiPopupFlags_MouseButtonRight);
		if (ImGui::BeginPopup("ActorTypeContext"))
		{
			if (ImGui::MenuItem("Place At View Center", nullptr, false, CanPlaceActor))
			{
				AddActorAt(ActorType, editorViewport.GetViewCenter());
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Copy Type ID"))
			{
				ImGui::SetClipboardText(ActorType.TypeId.c_str());
			}
			if (ImGui::MenuItem("Copy Class Name"))
			{
				ImGui::SetClipboardText(ActorType.ClassName.c_str());
			}
			ImGui::EndPopup();
		}
		ImGui::PopID();
	}

	if (VisibleActorCount == 0u)
	{
		ImGui::TextDisabled("No actor types");
	}
	ImGui::EndChild();
	ImGui::End();
}

void EditorApplication::DrawAssetBrowser(float x, float y, float width, float height)
{
	const bool fixedBounds = HasFixedBounds(width, height);
	if (fixedBounds)
	{
		SetPanelBounds(x, y, width, height);
	}
	ImGui::Begin("Asset Browser", &showAssetBrowser, PanelFlags(fixedBounds));

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
		const std::string EntryPath = entry.path.string();
		const std::string label = entry.directory ? "[Dir] " + name : "[" + std::string(AssetRegistry::TypeName(entry.asset ? entry.asset->type : AssetType::Unknown)) + "] " + name;
		const std::string assetId = entry.asset ? entry.asset->id : std::string{};
		const bool selected = !assetId.empty() && selectionService.IsAssetSelected(assetId);
		ImGui::PushID(EntryPath.c_str());

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
		ImGui::OpenPopupOnItemClick("AssetEntryContext", ImGuiPopupFlags_MouseButtonRight);
		const bool ItemHovered = ImGui::IsItemHovered();

		if (entry.asset && AssetRegistry::CanInstantiate(entry.asset->type) && ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("AMBER_ASSET", entry.asset->id.c_str(), entry.asset->id.size() + 1);
			ImGui::Text("%s", entry.asset->name.c_str());
			ImGui::TextDisabled("%s", AssetRegistry::TypeName(entry.asset->type));
			ImGui::EndDragDropSource();
		}
		else if (entry.asset && ItemHovered)
		{
			ImGui::SetTooltip("%s", AssetRegistry::TypeName(entry.asset->type));
		}
		if (entry.asset && !AssetRegistry::CanInstantiate(entry.asset->type))
		{
			if (ItemHovered)
			{
				ImGui::SetTooltip("%s assets are indexed but not placeable yet.", AssetRegistry::TypeName(entry.asset->type));
			}
		}
		if (ImGui::BeginPopup("AssetEntryContext"))
		{
			if (entry.directory)
			{
				if (ImGui::MenuItem("Open"))
				{
					pendingDirectory = entry.path;
					openPendingDirectory = true;
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Copy Path"))
				{
					ImGui::SetClipboardText(EntryPath.c_str());
				}
			}
			else if (entry.asset)
			{
				if (ImGui::MenuItem("Select"))
				{
					selectionService.SelectAsset(entry.asset->id);
				}
				if (entry.asset->type == AssetType::Scene && ImGui::MenuItem("Open Scene"))
				{
					OpenSceneFile(entry.asset->absolutePath);
				}
				const bool CanPlaceAsset = AssetRegistry::CanInstantiate(entry.asset->type) && !playSession.IsPlaying();
				if (ImGui::MenuItem("Place At View Center", nullptr, false, CanPlaceAsset))
				{
					PlaceAssetAtViewCenter(*entry.asset);
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Copy Asset ID"))
				{
					ImGui::SetClipboardText(entry.asset->id.c_str());
				}
				if (ImGui::MenuItem("Copy Path"))
				{
					ImGui::SetClipboardText(entry.asset->absolutePath.string().c_str());
				}
			}
			ImGui::EndPopup();
		}
		ImGui::PopID();
	}
	if (ImGui::BeginPopupContextWindow("AssetBrowserBackgroundContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::MenuItem("Refresh"))
		{
			RefreshAssets();
		}
		const bool CanGoUp = currentAssetPath != contentRoot;
		if (ImGui::MenuItem("Up", nullptr, false, CanGoUp))
		{
			pendingDirectory = currentAssetPath.parent_path();
			openPendingDirectory = true;
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Project Root", nullptr, false, !projectContentRoot.empty()))
		{
			SwitchContentRoot(projectContentRoot);
		}
		if (ImGui::MenuItem("Engine Root", nullptr, false, !engineContentRoot.empty()))
		{
			SwitchContentRoot(engineContentRoot);
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Copy Current Path"))
		{
			const std::string Path = currentAssetPath.string();
			ImGui::SetClipboardText(Path.c_str());
		}
		ImGui::EndPopup();
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
	const bool fixedBounds = HasFixedBounds(width, height);
	if (fixedBounds)
	{
		SetPanelBounds(x, y, width, height);
	}
	ImGui::Begin("Scene Outliner", &showSceneOutliner, PanelFlags(fixedBounds));

	ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DefaultOpen;
	if (sceneDocument.IsDirty())
	{
		ImGui::TextDisabled("Modified");
	}

	const bool SceneRootOpen = ImGui::TreeNodeEx(sceneDocument.GetName().c_str(), rootFlags);
	if (ImGui::BeginPopupContextItem("SceneOutlinerSceneContext", ImGuiPopupFlags_MouseButtonRight))
	{
		DrawSceneContextMenu(editorViewport.GetViewCenter());
		ImGui::EndPopup();
	}
	if (SceneRootOpen)
	{
		for (const SceneObject& object : sceneDocument.GetObjects())
		{
			ImGui::PushID(static_cast<int>(object.id));
			const bool selected = selectionService.IsSceneObjectSelected(object.id);
			if (ImGui::Selectable(object.name.c_str(), selected))
			{
				selectionService.SelectSceneObject(object.id);
			}
			if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(1))
			{
				selectionService.SelectSceneObject(object.id);
				SceneContextTarget = EditorViewport::EContextMenuTarget::SceneObject;
				SceneContextObjectId = object.id;
				SceneContextWorldPosition = editorViewport.GetViewCenter();
				ImGui::OpenPopup("SceneOutlinerObjectContext");
			}
			ImGui::SameLine();
			const FActorTypeDefinition* ActorType = actorTypeRegistry.FindByClassName(object.className);
			ImGui::TextDisabled("%s", ActorType ? ActorType->DisplayName.c_str() : SceneDocument::KindName(object.kind));
			ImGui::PopID();
		}
		ImGui::TreePop();
	}
	if (ImGui::BeginPopup("SceneOutlinerObjectContext"))
	{
		DrawSceneObjectContextMenu(SceneContextObjectId);
		ImGui::EndPopup();
	}
	if (ImGui::BeginPopupContextWindow("SceneOutlinerBackgroundContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		DrawSceneContextMenu(editorViewport.GetViewCenter());
		ImGui::EndPopup();
	}

	ImGui::End();
}

void EditorApplication::DrawDetailsPanel(float x, float y, float width, float height)
{
	const bool fixedBounds = HasFixedBounds(width, height);
	if (fixedBounds)
	{
		SetPanelBounds(x, y, width, height);
	}
	ImGui::Begin("Details", &showDetails, PanelFlags(fixedBounds));

	if (playSession.IsPlaying())
	{
		ImGui::TextDisabled("%s is running. Stop runtime to edit scene data.", playSession.GetRunModeName());
		ImGui::Text("Mode: %s", playSession.GetRunModeName());
		ImGui::Text("Game module: %s", playSession.GetRuntimeModuleName());
		ImGui::Text("Module source: %s", playSession.IsRuntimeModuleDynamic() ? "Dynamic plugin" : "Editor fallback");
		ImGui::Text("Requested target: %s", playSession.GetRequestedGameModuleTarget().c_str());
		ImGui::Text("Runtime objects: %zu", playSession.GetRuntimeObjectCount());
		const std::string runtimeFrameCount = std::to_string(playSession.GetFrameCount());
		const std::string runtimeRenderCount = std::to_string(playSession.GetRenderCount());
		ImGui::Text("Runtime frames: %s", runtimeFrameCount.c_str());
		ImGui::Text("Runtime renders: %s", runtimeRenderCount.c_str());
		ImGui::End();
		return;
	}

	const EditorSelection& selection = selectionService.GetSelection();
	if (selection.type == EditorSelectionType::SceneObject)
	{
		SceneObject* object = sceneDocument.FindObject(selection.objectId);
		if (object)
		{
			const FActorTypeDefinition* ActorType = actorTypeRegistry.FindByClassName(object->className);
			ImGui::Text("Selection: %s", object->name.c_str());
			ImGui::TextDisabled("Type: %s", SceneDocument::KindName(object->kind));
			ImGui::TextDisabled("Actor: %s", ActorType ? ActorType->DisplayName.c_str() : "Custom");
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

			const std::string ActorTypeLabel =
				ActorType ? ActorType->DisplayName + " (" + ActorType->ClassName + ")" : object->className;
			if (ImGui::BeginCombo("Actor Type", ActorTypeLabel.c_str()))
			{
				for (const FActorTypeDefinition& RegisteredActorType : actorTypeRegistry.GetActorTypes())
				{
					const bool Selected = object->className == RegisteredActorType.ClassName;
					const std::string Label = RegisteredActorType.DisplayName + " (" + RegisteredActorType.ClassName + ")";
					if (ImGui::Selectable(Label.c_str(), Selected))
					{
						object->className = RegisteredActorType.ClassName;
						object->kind = RegisteredActorType.Kind;
						object->size = RegisteredActorType.DefaultSize;
						object->assetId.clear();
						ActorType = &RegisteredActorType;
						SynchronizeSceneObjectComponents(*object, actorTypeRegistry);
						changed = true;
					}
					if (Selected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			if (!ActorType)
			{
				char classBuffer[128] = {};
				std::snprintf(classBuffer, sizeof(classBuffer), "%s", object->className.c_str());
				if (ImGui::InputText("Class Override", classBuffer, sizeof(classBuffer)))
				{
					object->className = classBuffer;
					changed = true;
				}
			}

			changed |= ImGui::Checkbox("Visible", &object->visible);
			changed |= ImGui::Checkbox("Locked", &object->locked);
			changed |= ImGui::InputFloat2("Position", &object->transform.position.x);
			changed |= ImGui::InputFloat("Rotation", &object->transform.rotationDegrees);
			changed |= ImGui::InputFloat2("Scale", &object->transform.scale.x);
			changed |= ClampTransformScale(object->transform.scale);
			if (object->kind == SceneObjectKind::AssetInstance ||
				object->kind == SceneObjectKind::Box ||
				object->kind == SceneObjectKind::Circle)
			{
				changed |= ImGui::InputFloat2("Size", &object->size.x);
			}

			SynchronizeSceneObjectComponents(*object, actorTypeRegistry);
			changed |= DrawSceneObjectComponents(*object);

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
				AssetRegistry::CanInstantiate(asset->type) ? "Drag into Scene View to place." : "Indexed, not placeable yet.");
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
	if (HasFixedBounds(width, height))
	{
		SetPanelBounds(x, y, width, height);
	}
	outputLog.Draw(&showOutputLog);
}

void EditorApplication::RefreshAssets()
{
	assetRegistry.ScanRoots(BuildAssetRoots());
	textureCache.SetAssetRoots(assetRegistry.GetRoots());
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

	std::sort(assetEntries.begin(), assetEntries.end(), [](const AssetEntry& left, const AssetEntry& right)
			  {
        if (left.directory != right.directory)
        {
            return left.directory > right.directory;
        }

        return left.path.filename().string() < right.path.filename().string(); });
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

void EditorApplication::RebuildActorTypeRegistry()
{
	actorTypeRegistry.Clear();
	RegisterDefaultActorTypes(actorTypeRegistry);

	if (!activeProjectLoaded || activeProject.actorTypes.empty())
	{
		return;
	}

	const std::filesystem::path ActorTypesPath = activeProject.ResolveProjectPath(activeProject.actorTypes);
	std::error_code ErrorCode;
	if (!std::filesystem::exists(ActorTypesPath, ErrorCode))
	{
		return;
	}

	std::string Error;
	if (!LoadActorTypesFromFile(ActorTypesPath, actorTypeRegistry, &Error))
	{
		LogBus::Add(LogLevel::Warning, "Editor", Error.empty() ? "Could not load project actor type schema." : Error);
	}
}

bool EditorApplication::CreateProjectFromBrowser(bool generateSolutionAfterCreate)
{
	ProjectGenerationRequest request;
	request.projectName = newProjectName.data();
	request.parentDirectory = newProjectLocation.data();
	request.engineRoot = FindEngineRoot();
	request.projectTemplate = ProjectTemplate::BlankCppGame;

	ProjectGenerationResult result;
	std::string error;
	if (!ProjectGenerator::CreateProject(request, result, &error))
	{
		projectBrowserStatus = error;
		projectBrowserStatusIsError = true;
		LogBus::Add(LogLevel::Error, "Editor", "Project creation failed: " + error);
		return false;
	}

	CopyToBuffer(openProjectPath, result.projectFilePath.string());
	projectBrowserStatus =
		"Created " + result.descriptor.name +
		". Configure command: " + result.configureCommand;
	projectBrowserStatusIsError = false;
	LogBus::Add(LogLevel::Info, "Editor", "Project created: " + result.projectRoot.string());
	if (!ApplyProjectDescriptor(result.descriptor))
	{
		return false;
	}

	if (generateSolutionAfterCreate)
	{
		return GenerateSolutionForActiveProject();
	}
	return true;
}

bool EditorApplication::GenerateSolutionForActiveProject()
{
	if (!activeProjectLoaded)
	{
		projectBrowserStatus = "No active project is open.";
		projectBrowserStatusIsError = true;
		LogBus::Add(LogLevel::Warning, "Editor", projectBrowserStatus);
		return false;
	}

	ProjectConfigureResult result;
	std::string error;
	LogBus::Add(LogLevel::Info, "Editor", "Generating solution: " + ProjectGenerator::GetConfigureCommand(activeProject));
	if (!ProjectGenerator::ConfigureProject(activeProject, result, &error))
	{
		projectBrowserStatus = error;
		projectBrowserStatusIsError = true;
		LogBus::Add(LogLevel::Error, "Editor", "Solution generation failed: " + error);
		return false;
	}

	projectBrowserStatus = "Generated solution: " + result.expectedSolutionPath.string();
	projectBrowserStatusIsError = false;
	LogBus::Add(LogLevel::Info, "Editor", projectBrowserStatus);
	return true;
}

bool EditorApplication::OpenProjectFile(const std::filesystem::path& path)
{
	ProjectDescriptor descriptor;
	std::string error;
	if (!LoadProjectDescriptor(path, descriptor, &error))
	{
		projectBrowserStatus = error;
		projectBrowserStatusIsError = true;
		LogBus::Add(LogLevel::Error, "Editor", "Project open failed: " + error);
		return false;
	}

	CopyToBuffer(openProjectPath, descriptor.projectFilePath.string());
	return ApplyProjectDescriptor(descriptor);
}

bool EditorApplication::OpenLegacyWorkspaceProject()
{
	const std::filesystem::path engineRoot = FindEngineRoot();
	const std::filesystem::path platformerProjectFile =
		engineRoot / "Projects" / "Platformer" / "Platformer.amberproject";

	ProjectDescriptor loadedDescriptor;
	std::string loadError;
	if (LoadProjectDescriptor(platformerProjectFile, loadedDescriptor, &loadError))
	{
		return ApplyProjectDescriptor(loadedDescriptor);
	}

	ProjectDescriptor descriptor;
	descriptor.name = "Platformer";
	descriptor.projectRoot = engineRoot / "Projects" / "Platformer";
	descriptor.engineRoot = engineRoot;
	descriptor.gameModuleTarget = "PlatformerGameModule";
	descriptor.playTarget = "PlatformerApp";
	descriptor.startupScene = std::filesystem::path("Content") / "Scenes" / "PlatformerTest.amber.scene";
	descriptor.actorTypes = std::filesystem::path("Config") / "ActorTypes.amberactors";
	descriptor.contentRoot = "Content";
	descriptor.buildPreset = "full-local-vcpkg";
	descriptor.solutionPath = std::filesystem::path("Builds") / "Editor" / "AmberEngine.sln";
	return ApplyProjectDescriptor(descriptor);
}

bool EditorApplication::ApplyProjectDescriptor(const ProjectDescriptor& descriptor)
{
	const std::filesystem::path resolvedContentRoot = descriptor.ResolveProjectPath(descriptor.contentRoot);
	std::error_code errorCode;
	if (!std::filesystem::exists(resolvedContentRoot, errorCode) ||
		!std::filesystem::is_directory(resolvedContentRoot, errorCode))
	{
		projectBrowserStatus = "Project content root does not exist: " + resolvedContentRoot.string();
		projectBrowserStatusIsError = true;
		return false;
	}

	activeProject = descriptor;
	activeProjectLoaded = true;
	RebuildActorTypeRegistry();
	showProjectBrowser = false;
	projectBrowserStatus.clear();
	projectBrowserStatusIsError = false;

	projectContentRoot = resolvedContentRoot;
	if (!activeProject.engineRoot.empty())
	{
		engineContentRoot = activeProject.engineRoot / "Engine" / "Content";
	}
	contentRoot = projectContentRoot;
	currentAssetPath = contentRoot;
	assetRegistry.ScanRoots(BuildAssetRoots());
	textureCache.SetAssetRoots(assetRegistry.GetRoots());
	RefreshAssets();

	currentScenePath = DefaultScenePath();
	std::error_code scenePathError;
	if (!currentScenePath.empty() && std::filesystem::exists(currentScenePath, scenePathError))
	{
		OpenSceneFile(currentScenePath);
	}
	else
	{
		sceneDocument.NewScene();
		selectionService.Clear();
	}

	LogBus::Add(LogLevel::Info, "Editor", "Project opened: " + activeProject.name);
	LogBus::Add(LogLevel::Info, "Editor", "Project root: " + activeProject.projectRoot.string());
	return true;
}

bool EditorApplication::DeleteSelectedSceneObject()
{
	const EditorSelection& selection = selectionService.GetSelection();
	if (selection.type != EditorSelectionType::SceneObject)
	{
		return false;
	}

	return DeleteSceneObject(selection.objectId);
}

bool EditorApplication::DeleteSceneObject(uint32 ObjectId)
{
	if (playSession.IsPlaying())
	{
		LogBus::Add(LogLevel::Warning, "Editor", "Stop PIE before deleting scene objects.");
		return false;
	}

	const SceneObject* Object = sceneDocument.FindObject(ObjectId);
	if (!Object)
	{
		selectionService.Clear();
		return false;
	}

	const std::string ObjectName = Object->name;
	if (!sceneDocument.RemoveObject(ObjectId))
	{
		LogBus::Add(LogLevel::Warning, "Editor", "Selected scene object cannot be deleted: " + ObjectName);
		return false;
	}

	const EditorSelection& Selection = selectionService.GetSelection();
	if (Selection.type == EditorSelectionType::SceneObject && Selection.objectId == ObjectId)
	{
		selectionService.Clear();
	}
	LogBus::Add(LogLevel::Info, "Editor", "Deleted scene object: " + ObjectName);
	return true;
}

bool EditorApplication::DuplicateSceneObject(uint32 ObjectId)
{
	if (playSession.IsPlaying())
	{
		LogBus::Add(LogLevel::Warning, "Editor", "Stop PIE before duplicating scene objects.");
		return false;
	}

	SceneObject* Duplicate = sceneDocument.DuplicateObject(ObjectId);
	if (!Duplicate)
	{
		LogBus::Add(LogLevel::Warning, "Editor", "Scene object cannot be duplicated.");
		return false;
	}

	selectionService.SelectSceneObject(Duplicate->id);
	LogBus::Add(LogLevel::Info, "Editor", "Duplicated scene object: " + Duplicate->name);
	return true;
}

SceneObject& EditorApplication::AddActorAt(const FActorTypeDefinition& ActorType, EditorVec2 WorldPosition)
{
	EditorTransform Transform;
	Transform.position = WorldPosition;

	const std::string ActorName = ActorType.DisplayName.empty() ? ActorType.ClassName : ActorType.DisplayName;
	SceneObject* Object = nullptr;
	if (ActorType.Kind == SceneObjectKind::Circle)
	{
		Object = &sceneDocument.AddCircleObject(ActorName, Transform, ActorType.DefaultSize);
	}
	else
	{
		Object = &sceneDocument.AddBoxObject(ActorName, Transform, ActorType.DefaultSize);
	}

	Object->className = ActorType.ClassName;
	Object->kind = ActorType.Kind;
	Object->size = ActorType.DefaultSize;
	SynchronizeSceneObjectComponents(*Object, actorTypeRegistry);
	selectionService.SelectSceneObject(Object->id);
	LogBus::Add(LogLevel::Info, "Editor", "Added actor: " + ActorName + " (" + ActorType.ClassName + ")");
	return *Object;
}

SceneObject& EditorApplication::AddBoxAt(EditorVec2 WorldPosition)
{
	const FActorTypeDefinition* ActorType = actorTypeRegistry.FindByTypeId("Default.Box");
	if (ActorType)
	{
		return AddActorAt(*ActorType, WorldPosition);
	}

	EditorTransform Transform;
	Transform.position = WorldPosition;
	SceneObject& Object = sceneDocument.AddBoxObject("Box", Transform, EditorVec2{128.0f, 32.0f});
	selectionService.SelectSceneObject(Object.id);
	LogBus::Add(LogLevel::Info, "Editor", "Added Box object.");
	return Object;
}

SceneObject& EditorApplication::AddCircleAt(EditorVec2 WorldPosition)
{
	const FActorTypeDefinition* ActorType = actorTypeRegistry.FindByTypeId("Default.Circle");
	if (ActorType)
	{
		return AddActorAt(*ActorType, WorldPosition);
	}

	EditorTransform Transform;
	Transform.position = WorldPosition;
	SceneObject& Object = sceneDocument.AddCircleObject("Circle", Transform, EditorVec2{64.0f, 64.0f});
	selectionService.SelectSceneObject(Object.id);
	LogBus::Add(LogLevel::Info, "Editor", "Added Circle object.");
	return Object;
}

bool EditorApplication::PlaceAssetAt(const AssetRecord& Asset, EditorVec2 WorldPosition)
{
	if (playSession.IsPlaying() || !AssetRegistry::CanInstantiate(Asset.type))
	{
		return false;
	}

	EditorTransform Transform;
	Transform.position = WorldPosition;
	SceneObject& Object = sceneDocument.AddAssetInstance(Asset.absolutePath.stem().string(), Asset.id, Transform);
	if (Asset.type == AssetType::Texture)
	{
		if (TexturePreview* Preview = textureCache.GetTexture(Asset))
		{
			Object.size = FitSceneTextureSize(Preview->width, Preview->height);
		}
	}
	else if (Asset.type == AssetType::Tilemap)
	{
		Object.size = EditorVec2{160.0f, 120.0f};
	}

	selectionService.SelectSceneObject(Object.id);
	LogBus::Add(LogLevel::Info, "Editor", "Placed asset: " + Asset.id);
	return true;
}

bool EditorApplication::PlaceAssetAtViewCenter(const AssetRecord& Asset)
{
	return PlaceAssetAt(Asset, editorViewport.GetViewCenter());
}

void EditorApplication::DrawActorPlacementMenu(EditorVec2 WorldPosition, bool CanEditScene)
{
	std::vector<std::string> Categories;
	for (const FActorTypeDefinition& ActorType : actorTypeRegistry.GetActorTypes())
	{
		if (std::find(Categories.begin(), Categories.end(), ActorType.Category) == Categories.end())
		{
			Categories.push_back(ActorType.Category);
		}
	}

	if (Categories.empty())
	{
		ImGui::TextDisabled("No actor types registered");
		return;
	}

	for (const std::string& Category : Categories)
	{
		const char* CategoryLabel = Category.empty() ? "Actors" : Category.c_str();
		if (!ImGui::BeginMenu(CategoryLabel, CanEditScene))
		{
			continue;
		}

		for (const FActorTypeDefinition& ActorType : actorTypeRegistry.GetActorTypes())
		{
			if (ActorType.Category != Category)
			{
				continue;
			}

			const std::string Label = ActorType.DisplayName + "##" + ActorType.TypeId;
			if (ImGui::MenuItem(Label.c_str(), nullptr, false, CanEditScene))
			{
				AddActorAt(ActorType, WorldPosition);
			}
		}
		ImGui::EndMenu();
	}
}

void EditorApplication::DrawSceneContextMenu(EditorVec2 WorldPosition)
{
	const bool CanEditScene = !playSession.IsPlaying() && activeProjectLoaded;
	if (ImGui::BeginMenu("Add Actor", CanEditScene))
	{
		DrawActorPlacementMenu(WorldPosition, CanEditScene);
		ImGui::EndMenu();
	}
	ImGui::Separator();
	if (ImGui::MenuItem("Focus Origin"))
	{
		editorViewport.FocusOrigin();
	}
	if (ImGui::MenuItem("Save Scene", "Ctrl+S", false, CanEditScene))
	{
		SaveCurrentScene();
	}
}

void EditorApplication::DrawSceneObjectContextMenu(uint32 ObjectId)
{
	SceneObject* Object = sceneDocument.FindObject(ObjectId);
	if (!Object)
	{
		ImGui::TextDisabled("Missing object");
		return;
	}

	ImGui::TextDisabled("%s", Object->name.c_str());
	if (ImGui::MenuItem("Select"))
	{
		selectionService.SelectSceneObject(ObjectId);
	}

	const bool CanEditObject = !playSession.IsPlaying() && sceneDocument.IsObjectRemovable(ObjectId);
	if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, CanEditObject))
	{
		DuplicateSceneObject(ObjectId);
		return;
	}
	if (ImGui::MenuItem("Delete", "Del", false, CanEditObject))
	{
		DeleteSceneObject(ObjectId);
		return;
	}

	ImGui::Separator();
	if (ImGui::MenuItem(Object->visible ? "Hide" : "Show", nullptr, false, !playSession.IsPlaying()))
	{
		Object->visible = !Object->visible;
		sceneDocument.SetDirty(true);
	}
	if (ImGui::MenuItem(Object->locked ? "Unlock" : "Lock", nullptr, false, !playSession.IsPlaying()))
	{
		Object->locked = !Object->locked;
		sceneDocument.SetDirty(true);
	}
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

void EditorApplication::FocusSelectedSceneObject()
{
	const EditorSelection& Selection = selectionService.GetSelection();
	if (Selection.type != EditorSelectionType::SceneObject)
	{
		return;
	}

	const SceneObject* Object = sceneDocument.FindObject(Selection.objectId);
	if (!Object)
	{
		selectionService.Clear();
		return;
	}

	editorViewport.FocusObject(*Object);
	LogBus::Add(LogLevel::Info, "Editor", "Focused selected object: " + Object->name);
}

bool EditorApplication::StartPlaySession(AE::EGameModuleRunMode RunMode)
{
	if (!activeProjectLoaded)
	{
		LogBus::Add(
			LogLevel::Warning,
			"Editor",
			"No active project is open. Open a project before starting a runtime session.");
		showProjectBrowser = true;
		return false;
	}

	const std::filesystem::path scenePath = currentScenePath.empty() ? DefaultScenePath() : currentScenePath;
	PlayInPIERequest request;
	request.projectName = activeProject.name;
	request.projectRoot = activeProject.projectRoot;
	request.scenePath = scenePath;
	request.gameModuleTarget = activeProject.gameModuleTarget;
	request.playTarget = activeProject.playTarget;
	request.RunMode = RunMode;
	return playSession.PlayInPIE(request, sceneDocument);
}

bool EditorApplication::PlayInPIE()
{
	return StartPlaySession(AE::EGameModuleRunMode::Play);
}

bool EditorApplication::SimulateInEditor()
{
	return StartPlaySession(AE::EGameModuleRunMode::Simulate);
}

std::filesystem::path EditorApplication::FindEngineRoot() const
{
	if (LooksLikeEngineRoot(activeProject.engineRoot))
	{
		return activeProject.engineRoot;
	}

	if (!engineContentRoot.empty())
	{
		const std::filesystem::path candidate = engineContentRoot.parent_path().parent_path();
		if (LooksLikeEngineRoot(candidate))
		{
			return candidate;
		}
	}

	if (!projectContentRoot.empty())
	{
		const std::filesystem::path candidate = projectContentRoot.parent_path();
		if (LooksLikeEngineRoot(candidate))
		{
			return candidate;
		}
	}

	std::error_code error;
	std::filesystem::path current = std::filesystem::weakly_canonical(std::filesystem::current_path(), error);
	if (current.empty())
	{
		current = std::filesystem::current_path();
	}

	for (int depth = 0; depth < 10; ++depth)
	{
		if (LooksLikeEngineRoot(current))
		{
			return current;
		}

		const std::filesystem::path nested = current / "AmberEngine";
		if (LooksLikeEngineRoot(nested))
		{
			return nested;
		}

		if (!current.has_parent_path() || current.parent_path() == current)
		{
			break;
		}
		current = current.parent_path();
	}

	return std::filesystem::current_path();
}

std::filesystem::path EditorApplication::DefaultScenePath() const
{
	if (activeProjectLoaded && !activeProject.startupScene.empty())
	{
		return activeProject.ResolveProjectPath(activeProject.startupScene);
	}

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
	std::error_code error;
	if (!engineContentRoot.empty() &&
		std::filesystem::exists(engineContentRoot, error) &&
		std::filesystem::is_directory(engineContentRoot, error))
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
		const std::filesystem::path relative = std::filesystem::relative(path, root.Path, error);
		if (error || relative.empty())
		{
			continue;
		}

		const std::string generic = relative.generic_string();
		if (generic == ".")
		{
			return root.Name;
		}
		if (generic.rfind("../", 0) == 0 || generic == "..")
		{
			continue;
		}

		return root.Name + "/" + generic;
	}

	return path.generic_string();
}

} // namespace AE::Editor
