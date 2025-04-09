// Renderer2D.cpp
// Implementation of the 2D renderer using SDL2

#include "../compat.h"
#include "Renderer2D.h"
#include "../core/Entity.h"
#include "../core/Terrain.h"
#include "../core/Game.h"
#include <iostream>

Renderer2D::Renderer2D()
    : mWindow(nullptr)
    , mRenderer(nullptr)
    , mWidth(800)
    , mHeight(600)
    , mInitialized(false)
    , mPixelsPerMeter(20.0f) // Conversion factor: 20 pixels = 1 meter
{
}

Renderer2D::~Renderer2D() {
    Shutdown();
}

bool Renderer2D::Initialize(int width, int height, const std::string& title) {
    // Store dimensions
    mWidth = width;
    mHeight = height;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Create window
    mWindow = SDL_CreateWindow(
        title.c_str(),          // Window title
        SDL_WINDOWPOS_CENTERED, // x position
        SDL_WINDOWPOS_CENTERED, // y position
        mWidth,                 // width
        mHeight,                // height
        0                       // flags
    );
    
    if (!mWindow) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Create renderer
    mRenderer = SDL_CreateRenderer(
        mWindow,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!mRenderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    mInitialized = true;
    std::cout << "Renderer2D initialized with dimensions: " << mWidth << "x" << mHeight
              << ", pixels per meter: " << mPixelsPerMeter << std::endl;
    return true;
}

void Renderer2D::Shutdown() {
    if (mRenderer) {
        SDL_DestroyRenderer(mRenderer);
        mRenderer = nullptr;
    }
    
    if (mWindow) {
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
    }
    
    mInitialized = false;
}

void Renderer2D::Clear() {
    if (!mInitialized) return;
    
    // Clear screen with black background
    SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
    SDL_RenderClear(mRenderer);
}

void Renderer2D::Present() {
    if (!mInitialized) return;
    
    SDL_RenderPresent(mRenderer);
}

// Convert physics coordinates (meters) to screen coordinates (pixels)
void Renderer2D::PhysicsToScreen(float physX, float physY, int& screenX, int& screenY) {
    // Convert from meters to pixels
    screenX = static_cast<int>(physX * mPixelsPerMeter);
    
    // Invert Y-axis: In physics, Y increases upward; in screen coords, Y increases downward
    screenY = mHeight - static_cast<int>(physY * mPixelsPerMeter);
    
}

void Renderer2D::RenderLander(Lander* lander) {
    if (!mInitialized || !lander) {
        std::cerr << "Failed to render lander: " << 
            (!mInitialized ? "Renderer not initialized" : "Lander is null") << std::endl;
        return;
    }
    
    // Get lander properties (in physics units - meters)
    const float* position = lander->GetPosition();
    float width = lander->GetWidth() / mPixelsPerMeter;  // Convert to meters
    float height = lander->GetHeight() / mPixelsPerMeter; // Convert to meters
    
    // Convert lander position from physics coordinates to screen coordinates
    int screenX, screenY;
    PhysicsToScreen(position[0], position[1], screenX, screenY);
    
    
    // Convert dimensions from meters to pixels for rendering
    int screenWidth = static_cast<int>(width * mPixelsPerMeter);
    int screenHeight = static_cast<int>(height * mPixelsPerMeter);
    
    // Ensure the lander is visible (increase size if needed)
    if (screenWidth < 40) screenWidth = 40;
    if (screenHeight < 60) screenHeight = 60;
    
    // Draw lander body as a rectangle centered on its position
    DrawRect(
        screenX - screenWidth / 2, 
        screenY - screenHeight / 2, 
        screenWidth, 
        screenHeight, 
        255, 0, 0
    );
    
    // Draw thrust flame if active
    if (lander->IsThrustActive()) {
        DrawRect(
            screenX - screenWidth / 4,
            screenY + screenHeight / 2,
            screenWidth / 2,
            screenHeight / 3,
            255, 165, 0
        );
    }
}

void Renderer2D::RenderTerrain(Terrain* terrain) {
    if (!mInitialized || !terrain) return;
    
    // Get terrain segments
    const std::vector<TerrainSegment>& segments = terrain->GetSegments2D();
    
    // Draw each terrain segment
    for (const auto& segment : segments) {
        // FIXED: Draw segments directly using their screen coordinates
        // since TerrainSegment coordinates are already in screen space
        
        // Use white for normal terrain and green for landing pads
        if (segment.isLandingPad) {
            DrawLine(segment.x1, segment.y1, segment.x2, segment.y2, 0, 255, 0);
        } else {
            DrawLine(segment.x1, segment.y1, segment.x2, segment.y2, 200, 200, 200);
        }
    }
}

void Renderer2D::RenderTelemetry(Game* game) {
    if (!mInitialized || !game) return;
    
    Lander* lander = game->GetLander();
    if (!lander) return;
    
    // Get lander properties
    const float* position = lander->GetPosition();
    const float* velocity = lander->GetVelocity();
    float fuel = lander->GetFuel();
    float maxFuel = lander->GetMaxFuel();
    
    // Background for telemetry panel
    DrawRect(10, 10, 200, 130, 50, 50, 50, 200);
    
    // Altitude indicator (green bar)
    int screenX, screenY;
    PhysicsToScreen(position[0], position[1], screenX, screenY);
    float altitude = position[1]; // Altitude in meters
    float maxAltitude = mHeight / mPixelsPerMeter; // Max altitude in meters
    float altitudePct = altitude / maxAltitude;
    if (altitudePct > 1.0f) altitudePct = 1.0f;
    
    DrawRect(20, 20, static_cast<int>(altitudePct * 180), 20, 0, 255, 0);
    
    // Velocity indicator (blue for downward, red for upward)
    float maxSafeVelocity = 2.0f; // m/s, safe landing velocity
    float velocityPct = std::abs(velocity[1]) / (maxSafeVelocity * 3);
    if (velocityPct > 1.0f) velocityPct = 1.0f;
    
    if (velocity[1] >= 0) { // In physics, positive Y is upward
        DrawRect(20, 50, static_cast<int>(velocityPct * 180), 20, 0, 0, 255); // Blue for downward
    } else {
        DrawRect(20, 50, static_cast<int>(velocityPct * 180), 20, 255, 0, 0); // Red for upward
    }
    
    // Fuel indicator (yellow bar)
    float fuelPct = fuel / maxFuel;
    DrawRect(20, 80, static_cast<int>(fuelPct * 180), 20, 255, 255, 0);
    
    // Numerical display of altitude and velocity
    char buffer[100];
    sprintf(buffer, "Alt: %.1f m", altitude);
    // Render text would go here if SDL_ttf was integrated
    
    sprintf(buffer, "Vel: %.1f m/s", velocity[1]);
    // Render text would go here if SDL_ttf was integrated
    
    sprintf(buffer, "Fuel: %.1f%%", fuelPct * 100);
    // Render text would go here if SDL_ttf was integrated
}

void Renderer2D::RenderGameState(Game* game) {
    // [Remainder of function stays the same]
}

void Renderer2D::DrawRect(float x, float y, float width, float height, 
                         Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (!mInitialized) return;
    
    SDL_SetRenderDrawColor(mRenderer, r, g, b, a);
    
    SDL_Rect rect {
        static_cast<int>(x),
        static_cast<int>(y),
        static_cast<int>(width),
        static_cast<int>(height)
    };
    
    SDL_RenderFillRect(mRenderer, &rect);
}

void Renderer2D::DrawLine(float x1, float y1, float x2, float y2, 
                         Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (!mInitialized) return;
    
    SDL_SetRenderDrawColor(mRenderer, r, g, b, a);
    
    SDL_RenderDrawLine(
        mRenderer,
        static_cast<int>(x1),
        static_cast<int>(y1),
        static_cast<int>(x2),
        static_cast<int>(y2)
    );
}
