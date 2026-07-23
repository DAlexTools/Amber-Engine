#ifndef IMGUI_SDL_H
#define IMGUI_SDL_H

struct ImDrawData;
struct SDL_Renderer;
struct SDL_Window;

namespace ImGuiSDL {
	// Call this to initialize the SDL renderer device that is internally used by the renderer.
	void Initialize(SDL_Renderer* renderer, int windowWidth, int windowHeight);

	// Call after ImGui_ImplSDL2_NewFrame when the SDL renderer uses a logical size.
	// This keeps ImGui display and mouse coordinates in the same logical space as SDL_Renderer draw calls.
	void ApplyLogicalDisplaySize(SDL_Window* window, SDL_Renderer* renderer, int logicalWidth, int logicalHeight);

	// Call this before destroying your SDL renderer or ImGui to ensure that proper cleanup is done. This doesn't do anything critically important though,
	// so if you're fine with small memory leaks at the end of your application, you can even omit this.
	void Deinitialize();

	// Call this every frame after ImGui::Render with ImGui::GetDrawData(). This will use the SDL_Renderer provided to the interfrace with Initialize
	// to draw the contents of the draw data to the screen.
	void Render(ImDrawData* drawData);
}

#endif
