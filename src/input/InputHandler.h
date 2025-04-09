// InputHandler.h
// Input handling for the lunar lander simulation

#pragma once

#include "../compat.h"
#include <SDL2/SDL.h>
#include <map>

// Forward declarations
class Game;

class InputHandler {
public:
    InputHandler(Game* game);
    ~InputHandler() = default;
    
    // Process input events
    void ProcessInput();
    
    // Check if a key is currently pressed
    bool IsKeyPressed(SDL_Scancode key) const;
    
    // Helper methods for common game controls
    bool IsThrustActive() const;
    bool IsRotateLeftActive() const;
    bool IsRotateRightActive() const;
    bool IsStartActive() const;
    bool IsResetActive() const;
    bool IsQuitActive() const;
    
    // Set key bindings
    void SetKeyBinding(const std::string& action, SDL_Scancode key);
    
private:
    // Reference to the game
    Game* mGame;
    
    // Current keyboard state
    const Uint8* mKeyboardState;
    
    // Key bindings (action name -> key code)
    std::map<std::string, SDL_Scancode> mKeyBindings;
    
    // Initialize default key bindings
    void InitializeKeyBindings();
};
