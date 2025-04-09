// Renderer2D.h
// 2D rendering implementation using SDL2

#pragma once

#include "../compat.h"
#include "Renderer.h"
#include <SDL2/SDL.h>

class Renderer2D : public Renderer {
public:
    Renderer2D();
    virtual ~Renderer2D();
    
    // Implement Renderer interface
    bool Initialize(int width, int height, const std::string& title) override;
    void Shutdown() override;
    void Clear() override;
    void Present() override;
    
    void RenderLander(Lander* lander) override;
    void RenderTerrain(Terrain* terrain) override;
    
    void RenderTelemetry(Game* game) override;
    void RenderGameState(Game* game) override;
    
    int GetWidth() const override { return mWidth; }
    int GetHeight() const override { return mHeight; }
    bool IsInitialized() const override { return mInitialized; }
    
    // 3D camera methods (implemented as no-ops for 2D renderer)
    void SetCameraPosition(float x, float y, float z) override {}
    void SetCameraTarget(float x, float y, float z) override {}
    void SetCameraUp(float x, float y, float z) override {}
    
    // 3D lighting methods (implemented as no-ops for 2D renderer)
    void SetLightPosition(float x, float y, float z) override {}
    void SetAmbientLight(float r, float g, float b) override {}
    
    // Helper methods for 2D rendering
    void DrawRect(float x, float y, float width, float height, 
                 Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    void DrawLine(float x1, float y1, float x2, float y2, 
                 Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    
    // Coordinate conversion method
    void PhysicsToScreen(float physX, float physY, int& screenX, int& screenY);
    
    // Getter for pixels per meter
    float GetPixelsPerMeter() const { return mPixelsPerMeter; }
    
private:
    // SDL rendering variables
    SDL_Window* mWindow;
    SDL_Renderer* mRenderer;
    
    // Renderer properties
    int mWidth;
    int mHeight;
    bool mInitialized;
    
    // Conversion from world coordinates to screen coordinates
    float mPixelsPerMeter;
};
