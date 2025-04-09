cmake_minimum_required(VERSION 3.10)
project(LunarLanderSimulator)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force Metal for macOS
message(STATUS "Forcing Metal renderer for macOS")
add_definitions(-DUSE_METAL=1)

# Architecture handling
set(CMAKE_OSX_ARCHITECTURES "x86_64")

# Disable X86 intrinsics for SDL
add_compile_definitions(SDL_DISABLE_IMMINTRIN_H=1)
add_compile_definitions(SDL_DISABLE_MMINTRIN_H=1)
add_compile_definitions(SDL_DISABLE_XMMINTRIN_H=1)

# Metal frameworks
set(METAL_FRAMEWORKS
    "-framework Metal"
    "-framework MetalKit"
    "-framework Foundation"
    "-framework QuartzCore"
    "-framework AppKit"
)

# Find required packages
find_package(SDL2 REQUIRED)
find_package(Bullet REQUIRED)

# Include directories
include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${BULLET_INCLUDE_DIRS}
    /usr/local/opt/bullet/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/metal-cpp
)

# Define source files
set(SOURCES
    src/main.cpp
    src/core/Entity.cpp
    src/core/Game.cpp
    src/core/Physics.cpp
    src/core/Terrain.cpp
    src/rendering/Renderer2D.cpp
    src/rendering/Renderer3D_Metal.cpp
    src/input/InputHandler.cpp
)

# Add Objective-C++ files
set(OBJCPP_SOURCES
    src/rendering/MetalBridge.mm
)

# Set Objective-C++ properties
set_source_files_properties(${OBJCPP_SOURCES} PROPERTIES
    COMPILE_FLAGS "-x objective-c++"
)

# Metal shader compilation
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/assets/shaders)
file(COPY ${CMAKE_SOURCE_DIR}/assets/shaders/LanderShaders.metal 
     DESTINATION ${CMAKE_BINARY_DIR}/assets/shaders)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/assets/shaders/default.metallib
    COMMAND xcrun -sdk macosx metal -c 
            ${CMAKE_SOURCE_DIR}/assets/shaders/LanderShaders.metal 
            -o ${CMAKE_BINARY_DIR}/assets/shaders/LanderShaders.air
    COMMAND xcrun -sdk macosx metallib 
            ${CMAKE_BINARY_DIR}/assets/shaders/LanderShaders.air 
            -o ${CMAKE_BINARY_DIR}/assets/shaders/default.metallib
    DEPENDS ${CMAKE_SOURCE_DIR}/assets/shaders/LanderShaders.metal
    COMMENT "Compiling Metal shaders"
)

# Define shader resources
set(RESOURCES ${CMAKE_BINARY_DIR}/assets/shaders/default.metallib)

# Create shader compilation target
add_custom_target(metallib_resources 
                  DEPENDS ${CMAKE_BINARY_DIR}/assets/shaders/default.metallib)

# Create executable
add_executable(LunarLander ${SOURCES} ${OBJCPP_SOURCES} ${RESOURCES})
add_dependencies(LunarLander metallib_resources)

# Link libraries
target_link_libraries(LunarLander
    ${SDL2_LIBRARIES}
    ${BULLET_LIBRARIES}
    ${METAL_FRAMEWORKS}
)

# Create asset directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/assets)