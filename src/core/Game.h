// Game.h
// Main game class for the lunar lander simulation

#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../rendering/Renderer.h" // Base renderer interface
#include "../rendering/Renderer3D_Metal.h" // Add this line
#include "Entity.h"

// Forward declarations
class Renderer;
class Physics;
class Terrain;
class InputHandler;

// Game states
enum class GameState {
    READY,
    FLYING,
    LANDED,
    CRASHED
};

// Game difficulty levels
enum class Difficulty {
    EASY,
    NORMAL,
    HARD
};

class Game {
public:
    Game();
    ~Game();
    
    // Core game functions
    bool Initialize();
    void Run();
    void Shutdown();
    
    // Game state methods
    GameState GetGameState() const { return mGameState; }
    void SetGameState(GameState state) { mGameState = state; }
    
    // Entity getters
    Lander* GetLander() { return mLander.get(); }
    Terrain* GetTerrain() { return mTerrain.get(); }
    
    // Game config settings
    void SetDifficulty(Difficulty difficulty);
    void SetRenderingMode(bool use3D);
    void Reset();
    
    // Game statistics
    float GetScore() const { return mScore; }
    float GetElapsedTime() const { return mElapsedTime; }
    float GetFuelUsed() const { return mFuelUsed; }
    
    // Physics to screen conversion
    float GetPixelsPerMeter() const { return mPixelsPerMeter; }
    
    // Input callbacks
    void OnKeyDown(int keyCode);
    void OnKeyUp(int keyCode);
    
private:
    // Game loop functions
    void ProcessInput();
    void Update(float deltaTime);
    void Render();
    
    // Game state
    GameState mGameState;
    Difficulty mDifficulty;
    bool m3DMode;
    
    // Game entities
    std::unique_ptr<Lander> mLander;
    std::unique_ptr<Terrain> mTerrain;
    
    // Core systems
    std::unique_ptr<Renderer> mRenderer;
    std::unique_ptr<Physics> mPhysics;
    std::unique_ptr<InputHandler> mInputHandler;
    
    // Game statistics
    float mScore;
    float mElapsedTime;
    float mFuelUsed;
    
    // Timing
    unsigned int mLastFrameTime;
    
    // Window dimensions
    int mWindowWidth;
    int mWindowHeight;
    
    // Physics to screen conversion
    float mPixelsPerMeter;
    
    // Game is running flag
    bool mIsRunning;
};