// Terrain.cpp
// Implementation of the Terrain class

#include "Terrain.h"
#include "../rendering/Renderer.h"
#include "../rendering/Renderer2D.h"
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <algorithm>

Terrain::Terrain()
    : Entity()
    , mWidth(800)
    , mHeight(600)
    , mLength(800) // For 3D
    , mPixelsPerMeter(20.0f) // Conversion factor
{
    mName = "Terrain";
}

void Terrain::Update(float deltaTime) {
    // Terrain typically doesn't need updating every frame
}

void Terrain::Render(Renderer* renderer) {
    // Delegate rendering to the renderer
    renderer->RenderTerrain(this);
}

void Terrain::Generate2D(int width, int height) {
    mWidth = width;
    mHeight = height;
    
    // Clear any existing terrain
    mSegments2D.clear();
    
    // Create a baseline terrain height (in screen coordinates)
    // FIXED: Set baseline at bottom of screen (larger Y value)
    int baseHeight = height - 50; // This puts terrain 50 pixels from bottom
    
    // Create segments for the terrain with some randomness
    const int segmentCount = 10;
    
    for (int i = 0; i < segmentCount; i++) {
        TerrainSegment segment;
        // Store segment coordinates in pixels for rendering
        segment.x1 = i * (width / segmentCount);
        segment.y1 = baseHeight - (rand() % 20); // Random height variation
        segment.x2 = (i + 1) * (width / segmentCount);
        segment.y2 = baseHeight - (rand() % 20);
        segment.isLandingPad = false;
        
        mSegments2D.push_back(segment);
    }
    
    // Create a landing pad
    // Center the landing pad
    int centerX = width / 2;
    int landingPadWidth = width / segmentCount * 2;  // Make landing pad 2 segments wide
    
    // Calculate landing pad start position to be centered
    int landingPadStart = centerX - (landingPadWidth / 2);
    
    // Find which segments the landing pad overlaps with
    int startSegment = landingPadStart / (width / segmentCount);
    int endSegment = (landingPadStart + landingPadWidth) / (width / segmentCount);
    
    // Mark all segments in range as landing pad
    for (int i = startSegment; i <= endSegment && i < segmentCount; i++) {
        if (i >= 0 && i < mSegments2D.size()) {
            mSegments2D[i].isLandingPad = true;
            // Make landing pad perfectly flat
            mSegments2D[i].y1 = mSegments2D[i].y2 = baseHeight;
        }
    }
}

void Terrain::CreateLandingPad2D(int startX, int width) {
    // Find the segments that we need to modify
    for (auto& segment : mSegments2D) {
        if (segment.x1 >= startX && segment.x2 <= startX + width) {
            // This segment should be part of the landing pad
            segment.isLandingPad = true;
            
            // Make it flat
            segment.y1 = segment.y2 = mHeight - 50;
        }
    }
}

bool Terrain::CheckCollision2D(Lander* lander, float& collisionHeight) {
    if (!lander) return false;
    
    // Get lander properties in physics units (meters)
    const float* landerPos = lander->GetPosition();
    float landerWidth = lander->GetWidth() / mPixelsPerMeter;
    float landerHeight = lander->GetHeight() / mPixelsPerMeter;
    
    // Get lander bottom center position
    float landerBottomX = landerPos[0];
    float landerBottomY = landerPos[1] - landerHeight / 2;
    
    // Convert lander position to screen coordinates for segment comparison
    float screenLanderX = landerBottomX * mPixelsPerMeter;
    float screenLanderY = mHeight - (landerBottomY * mPixelsPerMeter);
    
    // Check collision with each terrain segment
    for (const auto& segment : mSegments2D) {
        // Simple line-point collision check in screen coordinates
        if (screenLanderX >= segment.x1 && screenLanderX <= segment.x2) {
            // Interpolate Y position on segment
            float segmentPct = (screenLanderX - segment.x1) / (segment.x2 - segment.x1);
            float segmentY = segment.y1 + segmentPct * (segment.y2 - segment.y1);
            
            // Convert terrain height to physics units (meters)
            float terrainHeightMeters = (mHeight - segmentY) / mPixelsPerMeter;
            
            // In physics coordinates, we check if lander's bottom Y is lower than terrain Y
            if (landerBottomY <= terrainHeightMeters) {
                // Collision detected - return the terrain height in physics units
                collisionHeight = terrainHeightMeters;
                return true;
            }
        }
    }
    
    return false;
}

bool Terrain::IsValidLanding2D(Lander* lander) {
    if (!lander) return false;
    
    // Get lander properties in physics units (meters)
    const float* landerPos = lander->GetPosition();
    const float* landerVel = lander->GetVelocity();
    
    // Get lander bottom center position
    float landerBottomX = landerPos[0];
    
    // Convert to screen coordinates for segment comparison
    float screenLanderX = landerBottomX * mPixelsPerMeter;
    
    // Debug output to help diagnose landing issues
    std::cout << "Landing check - Position: (" << landerPos[0] << ", " << landerPos[1] 
              << ") m, Velocity: (" << landerVel[0] << ", " << landerVel[1] 
              << ") m/s" << std::endl;
    
    // Check if lander is on a landing pad
    bool onLandingPad = false;
    for (const auto& segment : mSegments2D) {
        if (segment.isLandingPad && 
            screenLanderX >= segment.x1 && 
            screenLanderX <= segment.x2) {
            
            onLandingPad = true;
            
            // Calculate terrain height in meters
            float terrainHeightMeters = (mHeight - segment.y1) / mPixelsPerMeter;
            
            std::cout << "Lander is on landing pad! Terrain height: " << terrainHeightMeters 
                      << " m, Lander y: " << landerPos[1] << " m" << std::endl;
            
            // Check landing conditions:
            // 1. Vertical velocity must be low (regardless of direction)
            // 2. Horizontal velocity must be low
            const float safeVerticalVelocity = 2.0f;   // m/s
            const float safeHorizontalVelocity = 1.0f; // m/s
            
            bool safeVertical = std::abs(landerVel[1]) <= safeVerticalVelocity;
            bool safeHorizontal = std::abs(landerVel[0]) <= safeHorizontalVelocity;
            
            std::cout << "Safe vertical: " << (safeVertical ? "YES" : "NO") 
                      << " (" << std::abs(landerVel[1]) << " m/s), "
                      << "Safe horizontal: " << (safeHorizontal ? "YES" : "NO") 
                      << " (" << std::abs(landerVel[0]) << " m/s)" << std::endl;
            
            if (safeVertical && safeHorizontal) {
                return true;
            }
        }
    }
    
    if (!onLandingPad) {
        std::cout << "Lander is NOT on a landing pad!" << std::endl;
    }
    
    return false;
}

// 3D Terrain methods (for Phase 3)
// [3D implementation remains largely unchanged]

void Terrain::Generate3D(int width, int length, int height) {
    mWidth = width;
    mLength = length;
    mHeight = height;
    
    // Clear any existing terrain
    mTriangles3D.clear();
    
    // In a real implementation, this would generate a proper 3D terrain mesh
    // For now, just create a flat plane with some height variations
    
    // Generate a grid of vertices
    const int gridSize = 20;
    const float cellWidth = (float)width / gridSize;
    const float cellLength = (float)length / gridSize;
    
    // Generate heightmap data
    mHeightData.resize((gridSize + 1) * (gridSize + 1));
    for (int z = 0; z <= gridSize; z++) {
        for (int x = 0; x <= gridSize; x++) {
            float height = mHeight - 50;
            
            // Add some random height variation
            if (!(x > gridSize / 3 && x < 2 * gridSize / 3 && 
                  z > gridSize / 3 && z < 2 * gridSize / 3)) {
                height += (rand() % 20) - 10;
            }
            
            mHeightData[z * (gridSize + 1) + x] = height;
        }
    }
    
    // Create triangles from the grid
    for (int z = 0; z < gridSize; z++) {
        for (int x = 0; x < gridSize; x++) {
            // Get heights of the four corners
            float h1 = mHeightData[z * (gridSize + 1) + x];
            float h2 = mHeightData[z * (gridSize + 1) + x + 1];
            float h3 = mHeightData[(z + 1) * (gridSize + 1) + x];
            float h4 = mHeightData[(z + 1) * (gridSize + 1) + x + 1];
            
            // Create two triangles for this grid cell
            TerrainTriangle tri1, tri2;
            
            // First triangle (top-left, top-right, bottom-left)
            tri1.vertices[0] = x * cellWidth;
            tri1.vertices[1] = h1;
            tri1.vertices[2] = z * cellLength;
            
            tri1.vertices[3] = (x + 1) * cellWidth;
            tri1.vertices[4] = h2;
            tri1.vertices[5] = z * cellLength;
            
            tri1.vertices[6] = x * cellWidth;
            tri1.vertices[7] = h3;
            tri1.vertices[8] = (z + 1) * cellLength;
            
            // Calculate normal (simplified)
            tri1.normal[0] = 0.0f;
            tri1.normal[1] = 1.0f; // Pointing up
            tri1.normal[2] = 0.0f;
            
            // Second triangle (bottom-left, top-right, bottom-right)
            tri2.vertices[0] = x * cellWidth;
            tri2.vertices[1] = h3;
            tri2.vertices[2] = (z + 1) * cellLength;
            
            tri2.vertices[3] = (x + 1) * cellWidth;
            tri2.vertices[4] = h2;
            tri2.vertices[5] = z * cellLength;
            
            tri2.vertices[6] = (x + 1) * cellWidth;
            tri2.vertices[7] = h4;
            tri2.vertices[8] = (z + 1) * cellLength;
            
            // Calculate normal (simplified)
            tri2.normal[0] = 0.0f;
            tri2.normal[1] = 1.0f; // Pointing up
            tri2.normal[2] = 0.0f;
            
            // Set landing pad status (center area is landing pad)
            tri1.isLandingPad = (x > gridSize / 3 && x < 2 * gridSize / 3 && 
                                 z > gridSize / 3 && z < 2 * gridSize / 3);
            tri2.isLandingPad = tri1.isLandingPad;
            
            // Add triangles to terrain
            mTriangles3D.push_back(tri1);
            mTriangles3D.push_back(tri2);
        }
    }
}

void Terrain::LoadHeightmap(const char* filename) {
    // This would load a heightmap from an image file
    // For now, just generate some random terrain
    Generate3D(mWidth, mLength, mHeight);
}

bool Terrain::CheckCollision3D(Lander* lander, float& collisionHeight) {
    const float* landerPos = lander->GetPosition();
    float landerWidth = lander->GetWidth();
    float landerHeight = lander->GetHeight();
    float landerDepth = lander->GetDepth();
    
    // Simplified 3D collision detection
    // In a real implementation, this would do proper 3D collision detection
    
    // For now, just project down to find height at lander position
    float landerX = landerPos[0];
    float landerY = landerPos[1];
    float landerZ = landerPos[2];
    
    // Find which triangle the lander is above
    for (const auto& triangle : mTriangles3D) {
        // Simple point-in-triangle test (2D projection)
        // This is a simplification - real collision would be more complex
        
        // Get the three vertices of the triangle
        float x1 = triangle.vertices[0];
        float y1 = triangle.vertices[1];
        float z1 = triangle.vertices[2];
        
        float x2 = triangle.vertices[3];
        float y2 = triangle.vertices[4];
        float z2 = triangle.vertices[5];
        
        float x3 = triangle.vertices[6];
        float y3 = triangle.vertices[7];
        float z3 = triangle.vertices[8];
        
        // Check if lander (x,z) is within triangle (x,z) bounds
        // This is a very simplified approach
        if (landerX >= std::min({x1, x2, x3}) && 
            landerX <= std::max({x1, x2, x3}) &&
            landerZ >= std::min({z1, z2, z3}) && 
            landerZ <= std::max({z1, z2, z3})) {
            
            // Approximate height at this position
            float avgHeight = (y1 + y2 + y3) / 3.0f;
            
            // Check if lander has collided with terrain
            if (landerY + landerHeight/2 >= avgHeight) {
                collisionHeight = avgHeight;
                return true;
            }
        }
    }
    
    return false;
}

bool Terrain::IsValidLanding3D(Lander* lander) {
    const float* landerPos = lander->GetPosition();
    const float* landerVel = lander->GetVelocity();
    
    // Get lander position
    float landerX = landerPos[0];
    float landerZ = landerPos[2];
    
    // Check if lander is on a landing pad and moving slowly enough
    for (const auto& triangle : mTriangles3D) {
        if (!triangle.isLandingPad) {
            continue;
        }
        
        // Get the three vertices of the triangle
        float x1 = triangle.vertices[0];
        float z1 = triangle.vertices[2];
        
        float x2 = triangle.vertices[3];
        float z2 = triangle.vertices[5];
        
        float x3 = triangle.vertices[6];
        float z3 = triangle.vertices[8];
        
        // Check if lander (x,z) is within triangle (x,z) bounds
        if (landerX >= std::min({x1, x2, x3}) && 
            landerX <= std::max({x1, x2, x3}) &&
            landerZ >= std::min({z1, z2, z3}) && 
            landerZ <= std::max({z1, z2, z3})) {
            
            // Check velocities for safe landing
            const float safeVelocity = 2.0f; // m/s
            if (std::abs(landerVel[0]) <= safeVelocity && 
                landerVel[1] >= 0 && landerVel[1] <= safeVelocity &&
                std::abs(landerVel[2]) <= safeVelocity) {
                return true;
            }
        }
    }
    
    return false;
}
