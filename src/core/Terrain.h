// Terrain.h
// Terrain generation and management

#pragma once

#include <vector>
#include "Entity.h"

// Forward declare classes we need
class Renderer;
class Lander;

// Simple 2D terrain segment (coordinates in screen pixels)
struct TerrainSegment {
    float x1, y1;       // Start point
    float x2, y2;       // End point
    bool isLandingPad;  // Whether this segment is a valid landing zone
};

// 3D terrain triangle (coordinates in physics units - meters)
struct TerrainTriangle {
    float vertices[9];  // 3 vertices x 3 coordinates (x, y, z)
    float normal[3];    // Normal vector
    bool isLandingPad;  // Whether this triangle is a valid landing zone
};

// Terrain class - handles generation and collision detection
class Terrain : public Entity {
public:
    Terrain();
    virtual ~Terrain() = default;
    
    // Implement Entity methods
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;
    
    // 2D Terrain methods
    void Generate2D(int width, int height);
    bool CheckCollision2D(Lander* lander, float& collisionHeight);
    bool IsValidLanding2D(Lander* lander);
    
    // 3D Terrain methods
    void Generate3D(int width, int length, int height);
    void LoadHeightmap(const char* filename);
    bool CheckCollision3D(Lander* lander, float& collisionHeight);
    bool IsValidLanding3D(Lander* lander);
    
    // Terrain accessors
    const std::vector<TerrainSegment>& GetSegments2D() const { return mSegments2D; }
    const std::vector<TerrainTriangle>& GetTriangles3D() const { return mTriangles3D; }
    
    // Terrain dimensions
    int GetWidth() const { return mWidth; }
    int GetHeight() const { return mHeight; }
    int GetLength() const { return mLength; } // For 3D
    
    // Physics to screen conversion
    float GetPixelsPerMeter() const { return mPixelsPerMeter; }
    void SetPixelsPerMeter(float ppm) { mPixelsPerMeter = ppm; }

private:
    // 2D terrain representation (in screen pixels)
    std::vector<TerrainSegment> mSegments2D;
    
    // 3D terrain representation (in meters)
    std::vector<TerrainTriangle> mTriangles3D;
    
    // Heightmap data (for 3D)
    std::vector<float> mHeightData;
    
    // Terrain dimensions (in screen pixels for 2D, meters for 3D)
    int mWidth;
    int mHeight;
    int mLength; // For 3D
    
    // Conversion factor between physics and screen units
    float mPixelsPerMeter;
    
    // Create a valid landing pad in the terrain
    void CreateLandingPad2D(int startX, int width);
    void CreateLandingPad3D(int startX, int startZ, int width, int length);
};
