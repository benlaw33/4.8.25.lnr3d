// InputHandler.cpp
// Implementation of input handling

#include "../compat.h"
#include "InputHandler.h"
#include "../core/Game.h"
#include <iostream>

InputHandler::InputHandler(Game* game)
    : mGame(game)
    , mKeyboardState(SDL_GetKeyboardState(nullptr))
{
    InitializeKeyBindings();
}

void InputHandler::InitializeKeyBindings() {
    // Default key bindings
    mKeyBindings["thrust"] = SDL_SCANCODE_UP;
    mKeyBindings["rotateLeft"] = SDL_SCANCODE_LEFT;
    mKeyBindings["rotateRight"] = SDL_SCANCODE_RIGHT;
    mKeyBindings["start"] = SDL_SCANCODE_SPACE;
    mKeyBindings["reset"] = SDL_SCANCODE_R;
    mKeyBindings["quit"] = SDL_SCANCODE_ESCAPE;
}

void InputHandler::ProcessInput() {
    // Update keyboard state
    mKeyboardState = SDL_GetKeyboardState(nullptr);
    
    // Process SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                // Handle window close
                if (mGame) {
                    mGame->OnKeyDown(SDLK_ESCAPE); // Simulate ESC key press
                }
                break;
                
            case SDL_KEYDOWN:
                // Handle key down events
                if (mGame) {
                    mGame->OnKeyDown(event.key.keysym.sym);
                }
                break;
                
            case SDL_KEYUP:
                // Handle key up events
                if (mGame) {
                    mGame->OnKeyUp(event.key.keysym.sym);
                }
                break;
        }
    }
    
    // Game-specific input handling can be done here
    // For example, checking if thrust is active and notifying the game
}

bool InputHandler::IsKeyPressed(SDL_Scancode key) const {
    return mKeyboardState[key] != 0;
}

bool InputHandler::IsThrustActive() const {
    auto it = mKeyBindings.find("thrust");
    if (it != mKeyBindings.end()) {
        return IsKeyPressed(it->second);
    }
    return false;
}

bool InputHandler::IsRotateLeftActive() const {
    auto it = mKeyBindings.find("rotateLeft");
    if (it != mKeyBindings.end()) {
        return IsKeyPressed(it->second);
    }
    return false;
}

bool InputHandler::IsRotateRightActive() const {
    auto it = mKeyBindings.find("rotateRight");
    if (it != mKeyBindings.end()) {
        return IsKeyPressed(it->second);
    }
    return false;
}

bool InputHandler::IsStartActive() const {
    auto it = mKeyBindings.find("start");
    if (it != mKeyBindings.end()) {
        return IsKeyPressed(it->second);
    }
    return false;
}

bool InputHandler::IsResetActive() const {
    auto it = mKeyBindings.find("reset");
    if (it != mKeyBindings.end()) {
        return IsKeyPressed(it->second);
    }
    return false;
}

bool InputHandler::IsQuitActive() const {
    auto it = mKeyBindings.find("quit");
    if (it != mKeyBindings.end()) {
        return IsKeyPressed(it->second);
    }
    return false;
}

void InputHandler::SetKeyBinding(const std::string& action, SDL_Scancode key) {
    mKeyBindings[action] = key;
}
