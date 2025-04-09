// Game.cpp
// Main game implementation for the lunar lander simulation

#include "../compat.h"
#include "Game.h"
#include "Entity.h"
#include "Physics.h"
#include "Terrain.h"
#include "../rendering/Renderer.h"
#include "../rendering/Renderer2D.h"
#include "../rendering/Renderer3D_Metal.h"
#include "../input/InputHandler.h"
#include <iostream>
#include <SDL2/SDL.h>
#include <cmath>
#ifdef USE_METAL
#include "../rendering/Renderer3D_Metal.h"
#endif

Game::Game()
    : mGameState(GameState::READY)
    , mDifficulty(Difficulty::NORMAL)
    , m3DMode(false)
    , mScore(0.0f)
    , mElapsedTime(0.0f)
    , mFuelUsed(0.0f)
    , mLastFrameTime(0)
    , mWindowWidth(800)
    , mWindowHeight(600)
    , mIsRunning(false)
    , mPixelsPerMeter(20.0f) // Set consistent conversion factor
{
}

Game::~Game() {
    Shutdown();
}

bool Game::Initialize() {
    std::cout << "Initializing Lunar Lander Simulator..." << std::endl;
    
    // Create core game components
    mLander = std::make_unique<Lander>();
    mTerrain = std::make_unique<Terrain>();
    mPhysics = std::make_unique<Physics>();
    mInputHandler = std::make_unique<InputHandler>(this);

    // Create renderer (2D or 3D based on setting)
    if (m3DMode) {
        std::cout << "Using Metal 3D renderer" << std::endl;
        mRenderer = std::make_unique<Renderer3D_Metal>();
    } else {
        mRenderer = std::make_unique<Renderer2D>();
    }
    
    // Initialize renderer
    if (!mRenderer->Initialize(mWindowWidth, mWindowHeight, "Lunar Lander Simulator")) {
        std::cerr << "Failed to initialize renderer!" << std::endl;
        return false;
    }
    
    // Register entities with physics
    mPhysics->RegisterLander(mLander.get());
    mPhysics->RegisterTerrain(mTerrain.get());
    mPhysics->Initialize();
    
    // Set physics parameters based on difficulty
    SetDifficulty(mDifficulty);
    
    // Initialize terrain
    if (m3DMode) {
        // For 3D mode, generate terrain with dimensions in meters
        float terrainWidth = mWindowWidth / mPixelsPerMeter;
        float terrainLength = mWindowWidth / mPixelsPerMeter;
        float terrainHeight = mWindowHeight / mPixelsPerMeter;
        mTerrain->Generate3D(terrainWidth, terrainLength, terrainHeight);
    } else {
        // For 2D mode, generate terrain with dimensions in pixels
        // (Terrain class will handle conversion internally)
        mTerrain->Generate2D(mWindowWidth, mWindowHeight);
    }
    
    // Reset game state
    Reset();
    
    mIsRunning = true;
    mLastFrameTime = SDL_GetTicks();
    
    std::cout << "Initialization complete" << std::endl;
    return true;
}

void Game::Run() {
    std::cout << "Starting game loop..." << std::endl;

    if (!mIsRunning) {
        std::cerr << "Game not initialized!" << std::endl;
        return;
    }
    
    // Main game loop
    while (mIsRunning) {
        // Calculate delta time
        unsigned int currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - mLastFrameTime) / 1000.0f;
        mLastFrameTime = currentTime;
        
        // Cap delta time to prevent physics issues on lag spikes
        if (deltaTime > 0.1f) {
            deltaTime = 0.1f;
        }
        
        // Process input, update game state, and render
        ProcessInput();
        Update(deltaTime);
        Render();
        
        // Small delay to prevent 100% CPU usage
        SDL_Delay(1);
    }
}

void Game::Shutdown() {
    mIsRunning = false;
    
    // Clean up components in reverse order of creation
    mInputHandler.reset();
    mRenderer.reset();
    mPhysics.reset();
    mTerrain.reset();
    mLander.reset();
    
    // Quit SDL
    SDL_Quit();
    
    std::cout << "Game shut down" << std::endl;
}

void Game::SetDifficulty(Difficulty difficulty) {
    mDifficulty = difficulty;
    
    // Adjust physics parameters based on difficulty
    switch (mDifficulty) {
        case Difficulty::EASY:
            mPhysics->SetGravity(1.0f);  // Lower gravity (m/s²)
            break;
            
        case Difficulty::NORMAL:
            mPhysics->SetGravity(1.62f); // Lunar gravity (m/s²)
            break;
            
        case Difficulty::HARD:
            mPhysics->SetGravity(2.0f);  // Higher gravity (m/s²)
            break;
    }
    
    // Reset the game with new settings
    Reset();
    
    std::cout << "Difficulty set to: " << 
        (mDifficulty == Difficulty::EASY ? "Easy" : 
         mDifficulty == Difficulty::NORMAL ? "Normal" : "Hard") << 
        ", Gravity: " << mPhysics->GetGravity() << " m/s²" << std::endl;
}

void Game::SetRenderingMode(bool use3D) {
    // Only change if needed
    if (m3DMode != use3D) {
        m3DMode = use3D;
        
        // Reinitialize if the game is already running
        if (mIsRunning) {
            Shutdown();
            Initialize();
        }
    }
}

void Game::Reset() {
    // Reset game state
    mGameState = GameState::FLYING; // Start in FLYING
    std::cout << "DEBUG: Starting in FLYING state" << std::endl;
    mScore = 0.0f;
    mElapsedTime = 0.0f;
    mFuelUsed = 0.0f;
    
    // Reset lander
    if (mLander) {
        mLander->Reset();
        
        // Set initial position in meters
        float centerX = mWindowWidth / (2 * mPixelsPerMeter); // Center X in meters
        float startHeight = 20.0f; // More height to give time for physics simulation
        
        if (m3DMode) {
            float centerZ = mWindowWidth / (2 * mPixelsPerMeter); // Center Z in meters
            mLander->SetPosition(centerX, startHeight, centerZ);
        } else {
            mLander->SetPosition(centerX, startHeight);
        }
        
        // Reset velocity explicitly to prevent any issues
        float* velocity = mLander->GetVelocity();
        velocity[0] = 0.0f;
        velocity[1] = 0.0f;
        if (m3DMode) velocity[2] = 0.0f;
        
        std::cout << "Lander reset to position: (" << centerX << ", " << startHeight << ") m" << std::endl;
    }
    
    // Reset terrain (regenerate if needed)
    if (mTerrain) {
        if (m3DMode) {
            float terrainWidth = mWindowWidth / mPixelsPerMeter;
            float terrainLength = mWindowWidth / mPixelsPerMeter;
            float terrainHeight = mWindowHeight / mPixelsPerMeter;
            mTerrain->Generate3D(terrainWidth, terrainLength, terrainHeight);
        } else {
            mTerrain->Generate2D(mWindowWidth, mWindowHeight);
        }
    }
}

void Game::ProcessInput() {
    // Use the input handler to process input
    if (mInputHandler) {
        mInputHandler->ProcessInput();
        
        // Handle input based on game state
        if (mGameState == GameState::READY) {
            // Check for game start
            if (mInputHandler->IsStartActive()) {
                std::cout << "Game started by user input - switching to FLYING state" << std::endl;
                mGameState = GameState::FLYING;
            }
        } else if (mGameState == GameState::FLYING) {
            // Apply thrust if active
            if (mInputHandler->IsThrustActive()) {
                mLander->ApplyThrust(1.0f);
                std::cout << "Thrust applied" << std::endl;
                
                // Track fuel usage
                float originalFuel = mLander->GetFuel();
                float newFuel = originalFuel;
                if (originalFuel > newFuel) {
                    mFuelUsed += originalFuel - newFuel;
                }
            } else {
                mLander->ApplyThrust(0.0f);
            }
            
            // Handle rotation
            if (mInputHandler->IsRotateLeftActive()) {
                mLander->RotateLeft(2.0f);
            }
            
            if (mInputHandler->IsRotateRightActive()) {
                mLander->RotateRight(2.0f);
            }
        } else if (mGameState == GameState::LANDED || mGameState == GameState::CRASHED) {
            // Check for game reset
            if (mInputHandler->IsResetActive()) {
                Reset();
            }
        }
        
        // Check for quit
        if (mInputHandler->IsQuitActive()) {
            mIsRunning = false;
        }
    }
}

void Game::Update(float deltaTime) {
    // Only update physics when flying
    if (mGameState == GameState::FLYING) {
        // Update physics
        if (mPhysics) {
            mPhysics->Update(deltaTime);
        }
        
        // Update lander
        if (mLander) {
            mLander->Update(deltaTime);
            
    // KEEP AND MODIFY this block:
    if (mLander->IsLanded()) {
    mGameState = GameState::LANDED;
    
    // Calculate score based on fuel remaining and landing position
    float fuelRemaining = mLander->GetFuel() / mLander->GetMaxFuel();
    mScore = fuelRemaining * 1000.0f;
    
    // Get current position
    const float* position = mLander->GetPosition();
    
    // Print final landing statistics
    std::cout << "Landing successful! Time: " << mElapsedTime 
              << "s, Score: " << mScore 
              << ", Final position: (" << position[0] << ", " << position[1] << ") m" << std::endl;
    } else if (mLander->IsCrashed()) {
    mGameState = GameState::CRASHED;
    mScore = 0.0f;
    
    // Get current position
    const float* position = mLander->GetPosition();
    
    // Print crash statistics
    std::cout << "Crash landing! Time: " << mElapsedTime 
              << "s, Final position: (" << position[0] << ", " << position[1] << ") m" << std::endl;
    }
            
    }
        
        // Update elapsed time
        mElapsedTime += deltaTime;
    }
    
    // Update terrain (generally static, but may have animations)
    if (mTerrain) {
        mTerrain->Update(deltaTime);
    }
    
    // If 3D mode, update camera to follow lander
    if (m3DMode && mRenderer && mLander) {
    const float* landerPos = mLander->GetPosition();

    // Add debug output
    std::cout << "Lander position: (" 
    << landerPos[0] << ", " << landerPos[1] << ", " << landerPos[2] 
    << ")" << std::endl;
        
    // Position camera with a fixed offset to see both lander and terrain
    mRenderer->SetCameraPosition(
        landerPos[0] - 30.0f,     // offset to side
        40.0f,                    // fixed height
        landerPos[2] + 40.0f      // behind lander
    );

    // Make sure the camera is looking directly at the lander
    mRenderer->SetCameraTarget(
        landerPos[0],          // X
        landerPos[1],          // Y
        landerPos[2]           // Z
    );

    std::cout << "Camera position set to: (" 
    << landerPos[0] - 30.0f << ", " 
    << 40.0f << ", " 
    << landerPos[2] + 40.0f << ")" << std::endl;
        
    // Set camera up vector
    mRenderer->SetCameraUp(0.0f, 1.0f, 0.0f);
        
        // Set light position (sun)
        float terrainSize = mWindowWidth / mPixelsPerMeter;
        mRenderer->SetLightPosition(
            terrainSize / 2, 
            terrainSize + 25.0f, // 25 meters above terrain
            terrainSize / 2
        );
        
        // Set ambient light
        mRenderer->SetAmbientLight(0.3f, 0.3f, 0.3f);
    }
}

void Game::Render() {
    // Clear the screen
    if (mRenderer) {
        mRenderer->Clear();
        
        // Render terrain
        if (mTerrain) {
            mTerrain->Render(mRenderer.get());
        }
        
        // Render lander
        if (mLander && mLander->IsActive()) {
            mLander->Render(mRenderer.get());
        }
        
        // Render UI elements
        mRenderer->RenderTelemetry(this);
        mRenderer->RenderGameState(this);
        
        // Present rendered frame
        mRenderer->Present();
    }
}

void Game::OnKeyDown(int keyCode) {
    // Handle key press events
    switch (keyCode) {
        case SDLK_r:
            // Reset game
            Reset();
            break;
            
        case SDLK_ESCAPE:
            // Quit game
            mIsRunning = false;
            break;
            
        case SDLK_1:
            // Set easy difficulty
            SetDifficulty(Difficulty::EASY);
            break;
            
        case SDLK_2:
            // Set normal difficulty
            SetDifficulty(Difficulty::NORMAL);
            break;
            
        case SDLK_3:
            // Set hard difficulty
            SetDifficulty(Difficulty::HARD);
            break;
            
        case SDLK_TAB:
            // Toggle between 2D and 3D mode
            SetRenderingMode(!m3DMode);
            break;
    }
}

void Game::OnKeyUp(int keyCode) {
    // Handle key release events
    // No additional functionality needed here
}