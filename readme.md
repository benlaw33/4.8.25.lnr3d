# Lunar Lander Simulator

A 3D lunar lander simulation game developed in C++ using SDL2 and OpenGL.

## Project Overview

This simulator allows players to experience the challenges of landing a spacecraft on the lunar surface. The simulator can operate in both 2D and 3D modes, with physics that approximate real lunar conditions.

## Features

- **Dual Rendering Modes**: Switch between 2D and 3D rendering at runtime
- **Realistic Physics**: Lunar gravity (1.62 m/s²), momentum, and collision detection
- **Multiple Difficulty Levels**: Easy, Normal, and Hard modes
- **Terrain Generation**: Procedurally generated terrain with designated landing pads
- **Telemetry Display**: Real-time altitude, velocity, and fuel information
- **3D Camera Controls**: Follow the lander or switch to fixed views

## Controls

- **Up Arrow**: Apply thrust
- **Left/Right Arrows**: Rotate lander
- **Space**: Start game (from READY state)
- **R**: Reset game
- **1/2/3**: Set difficulty (Easy/Normal/Hard)
- **Tab**: Toggle between 2D and 3D mode
- **Escape**: Quit game

## Technical Implementation

### Architecture

The simulator uses a component-based architecture:

- **Core**: Entity, Lander, Game, Physics, Terrain
- **Rendering**: Renderer interface, Renderer2D, Renderer3D implementations
- **Input**: InputHandler for user controls

### Phases of Development

The project was developed in phases according to the roadmap:

#### Phase 1: Planning and Environment Setup
- Defined requirements
- Set up development environment
- Created project structure
- Implemented test framework

#### Phase 2: 2D Prototype (Vertical Descent)
- Created window and rendering with SDL2
- Implemented basic physics
- Added thrust mechanics and fuel system
- Created collision detection
- Added telemetry display

#### Phase 3: Expanding to 3D (Current)
- Set up 3D rendering pipeline with OpenGL
- Implemented 3D lander model
- Extended physics to 3D space
- Implemented rotation controls in 3D
- Added camera controls

## Building the Project

### Prerequisites

- CMake (3.10 or higher)
- SDL2 development libraries
- OpenGL development libraries

### Build Instructions

```bash
# Create build directory
mkdir build
cd build

# Generate build files
cmake ..

# Build the project
make

# Run the simulator
./LunarLander

# Run in 3D mode
./LunarLander --3d
```

### Platform-Specific Notes

#### macOS
- Special handling for Apple Silicon is included in the CMake configuration
- You may need to install SDL2 via Homebrew: `brew install sdl2`

#### Linux
- Install SDL2: `sudo apt-get install libsdl2-dev`
- Install OpenGL: `sudo apt-get install libgl1-mesa-dev`

#### Windows
- Install SDL2 development libraries and update CMake paths accordingly
- Make sure OpenGL libraries are available

## Next Development Steps

### Phase 4: Terrain and Environment
- Create more detailed terrain generator
- Import NASA lunar DEM data
- Implement improved terrain collision detection
- Add lighting and shadow effects

### Phase 5: Physics Refinement
- Implement more accurate inertia and momentum calculations
- Add torque physics
- Create regolith interaction model

## License

This project is released under the MIT License. See the LICENSE file for details.
