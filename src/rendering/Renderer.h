// Renderer.h
// Abstract renderer interface for the lunar lander simulation

#pragma once

#include <string>

// Forward declarations - make sure these are included before using them
class Lander;
class Terrain;
class Game;

// Abstract renderer interface
class Renderer {
public:
    // Virtual destructor for proper cleanup
    virtual ~Renderer() = default;
    
    // Core renderer methods
    virtual bool Initialize(int width, int height, const std::string& title) = 0;
    virtual void Shutdown() = 0;
    virtual void Clear() = 0;
    virtual void Present() = 0;
    
    // Entity rendering methods
    virtual void RenderLander(Lander* lander) = 0;
    virtual void RenderTerrain(Terrain* terrain) = 0;
    
    // UI rendering methods
    virtual void RenderTelemetry(Game* game) = 0;
    virtual void RenderGameState(Game* game) = 0;
    
    // Getter methods
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual bool IsInitialized() const = 0;
    
    // Camera management (for 3D)
    virtual void SetCameraPosition(float x, float y, float z) = 0;
    virtual void SetCameraTarget(float x, float y, float z) = 0;
    virtual void SetCameraUp(float x, float y, float z) = 0;
    
    // Lighting (for 3D)
    virtual void SetLightPosition(float x, float y, float z) = 0;
    virtual void SetAmbientLight(float r, float g, float b) = 0;
};
