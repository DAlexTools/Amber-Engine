#include "Graphics.h"
#include <iostream>
#include "Logging/Logger.h"

SDL_Window* Graphics::Window = nullptr;
SDL_Renderer* Graphics::Renderer = nullptr;
int Graphics::windowWidth = 0;
int Graphics::windowHeight = 0;

int Graphics::Width() 
{
    return windowWidth;
}

int Graphics::Height() 
{
    return windowHeight;
}

bool Graphics::OpenWindow() 
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) 
    {
        AE::Logger::Error("Error initializing SDL", "Physics");
        return false;
    }
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);
    windowWidth = display_mode.w;
    windowHeight = display_mode.h;
    Window = SDL_CreateWindow(NULL, 0, 0, windowWidth, windowHeight, SDL_WINDOW_FULLSCREEN);
    if (!Window) 
    {
        AE::Logger::Error("Error creating SDL window", "Physics");
        return false;
    }
    Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!Renderer) 
    {
        AE::Logger::Error("Error creating SDL renderer", "Physics");
        return false;
    }
    return true;
}

void Graphics::ClearScreen(Uint32 color) 
{
    SDL_SetRenderDrawColor(Renderer, color >> 16, color >> 8, color, 255);
    SDL_RenderClear(Renderer);
}

void Graphics::RenderFrame() 
{
    SDL_RenderPresent(Renderer);
}

void Graphics::DrawLine(int x0, int y0, int x1, int y1, Uint32 color) 
{
    lineColor(Renderer, x0, y0, x1, y1, color);
}

void Graphics::DrawCircle(int x, int y, int radius, float angle, Uint32 color) 
{
    circleColor(Renderer, x, y, radius, color);
    lineColor(Renderer, x, y, x + cos(angle) * radius, y + sin(angle) * radius, color);
}

void Graphics::DrawFillCircle(int x, int y, int radius, Uint32 color) 
{
    filledCircleColor(Renderer, x, y, radius, color);
}

void Graphics::DrawRect(int x, int y, int width, int height, Uint32 color) 
{
    lineColor(Renderer, x - width / 2.0, y - height / 2.0, x + width / 2.0, y - height / 2.0, color);
    lineColor(Renderer, x + width / 2.0, y - height / 2.0, x + width / 2.0, y + height / 2.0, color);
    lineColor(Renderer, x + width / 2.0, y + height / 2.0, x - width / 2.0, y + height / 2.0, color);
    lineColor(Renderer, x - width / 2.0, y + height / 2.0, x - width / 2.0, y - height / 2.0, color);
}

void Graphics::DrawFillRect(int x, int y, int width, int height, Uint32 color) {
    boxColor(Renderer, x - width / 2.0, y - height / 2.0, x + width / 2.0, y + height / 2.0, color);
}

void Graphics::DrawPolygon(int x, int y, const std::vector<FVector2D>& vertices, Uint32 color) 
{
    for (int i = 0; i < static_cast<int>(vertices.size()); i++) {
        int currIndex = i;
        int nextIndex = (i + 1) % vertices.size();
        lineColor(Renderer, vertices[currIndex].X, vertices[currIndex].Y, vertices[nextIndex].X, vertices[nextIndex].Y, color);
    }
    filledCircleColor(Renderer, x, y, 1, color);
}

void Graphics::DrawFillPolygon(int x, int y, const std::vector<FVector2D>& vertices, Uint32 color) 
{
    std::vector<short> vx;
    std::vector<short> vy;

    for (int i = 0; i < static_cast<int>(vertices.size()); i++) 
    {
        vx.push_back(static_cast<int>(vertices[i].X));
    }
    for (int i = 0; i < static_cast<int>(vertices.size()); i++) 
    {
        vy.push_back(static_cast<int>(vertices[i].Y));
    }
    filledPolygonColor(Renderer, &vx[0], &vy[0], vertices.size(), color);
    filledCircleColor(Renderer, x, y, 1, 0xFF000000);
}

void Graphics::DrawTexture(int x, int y, int width, int height, float rotation, SDL_Texture* texture) 
{
    SDL_Rect dstRect = {x - (width / 2), y - (height / 2), width, height};
    float rotationDeg = rotation * 57.2958;
    SDL_RenderCopyEx(Renderer, texture, NULL, &dstRect, rotationDeg, NULL, SDL_FLIP_NONE);
}

void Graphics::CloseWindow(void) 
{
    SDL_DestroyRenderer(Renderer);
    SDL_DestroyWindow(Window);
    SDL_Quit();
}
