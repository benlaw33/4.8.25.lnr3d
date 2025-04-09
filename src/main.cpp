// main.cpp
// Entry point for the lunar lander simulation
#include "compat.h"
#include "core/Game.h"
#include <iostream>

int main(int argc, char* argv[]) {
    // Check for 3D mode command line argument
    bool use3DMode = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--3d" || arg == "-3d") {
            use3DMode = true;
            break;
        }
    }
    
    // Create the game instance
    Game game;
    
    // Set rendering mode
    game.SetRenderingMode(use3DMode);
    
    // Initialize the game
    bool success = game.Initialize();
    if (!success) {
        std::cerr << "Failed to initialize game!" << std::endl;
        return 1;
    }
    
    // Run the game
    game.Run();
    
    // Clean up resources
    game.Shutdown();
    
    return 0;
}
