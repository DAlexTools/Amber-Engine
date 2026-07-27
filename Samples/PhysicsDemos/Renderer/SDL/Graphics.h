#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include "Core/Math/Vector2D.h"
#include <vector>

struct Graphics 
{
    static int windowWidth;
    static int windowHeight;
    static SDL_Window* Window;
    static SDL_Renderer* Renderer;

    static int Width();
    static int Height();
    static bool OpenWindow();
    static void CloseWindow();
    static void ClearScreen(Uint32 color);
    static void RenderFrame();
    static void DrawLine(int x0, int y0, int x1, int y1, Uint32 color);
    static void DrawCircle(int x, int y, int radius, float angle, Uint32 color);
    static void DrawFillCircle(int x, int y, int radius, Uint32 color);
    static void DrawRect(int x, int y, int width, int height, Uint32 color);
    static void DrawFillRect(int x, int y, int width, int height, Uint32 color);
    static void DrawPolygon(int x, int y, const std::vector<FVector2D>& vertices, Uint32 color);
    static void DrawFillPolygon(int x, int y, const std::vector<FVector2D>& vertices, Uint32 color);
    static void DrawTexture(int x, int y, int width, int height, float rotation, SDL_Texture* texture);
};

#endif
