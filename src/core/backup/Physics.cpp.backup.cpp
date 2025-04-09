// Physics.cpp
// Implementation of the physics system

#include "Physics.h"
#include <cmath>
#include <iostream>

Physics::Physics()
    : mGravity(1.62f)      // Lunar gravity (m/s²)
    , mAirDensity(0.0f)    // No atmosphere on the moon
    , m3DMode(false)       // Start in 2D mode
    , mLander(nullptr)
    , mTerrain(nullptr)
    , mIntegrationMethod(EULER)
    , mTimeScale(1.0f)     // Normal simulation speed (1:1)
    , mPixelsPerMeter(20.0f) // Conversion factor: 20 pixels = 1 meter
{
}

void Physics::Initialize() {
    // Initialize physics system
    std::cout << "Physics system initialized with Lunar gravity: " << mGravity << " m/s²" << std::endl;
}

void Physics::RegisterLander(Lander* lander) {
    mLander = lander;
}

void Physics::RegisterTerrain(Terrain* terrain) {
    mTerrain = terrain;
}

void Physics::Update(float deltaTime) {
    // Choose the appropriate update method based on mode
    if (m3DMode) {
        Update3D(deltaTime);
    } else {
        Update2D(deltaTime);
    }
}

bool Physics::CheckCollisions() {
    // Choose the appropriate collision method based on mode
    if (m3DMode) {
        return CheckCollisions3D();
    } else {
        return CheckCollisions2D();
    }
}

// Apply gravity force to the lander
void Physics::ApplyGravity(Lander* lander, float deltaTime) {
    if (!lander || lander->IsLanded() || lander->IsCrashed()) {
        return;
    }
    
    // Get lander velocity (in m/s)
    float* velocity = lander->GetVelocity();
    
    // Apply gravity (acceleration in m/s²)
    // negative gravity means increasing downward velocity
    velocity[1] -= mGravity * deltaTime;
    
    // In 3D mode, gravity would be applied based on coordinate system
    if (m3DMode) {
        // Could adjust for planetary gravity if needed
    }
}

// Apply thrust force to the lander
void Physics::ApplyThrust(Lander* lander, float deltaTime) {
    if (!lander || lander->IsLanded() || lander->IsCrashed() || !lander->IsThrustActive()) {
        return;
    }
    
    // Calculate thrust force based on lander properties (in N)
    float maxThrust = lander->GetMass() * 2.5f * mGravity; // Thrust-to-weight ratio of 2.5
    float thrustForce = maxThrust * lander->GetThrustLevel();
    
    // Convert force to acceleration (F = ma, so a = F/m)
    float thrustAccel = thrustForce / lander->GetMass();
    
    // Get lander velocity and rotation
    float* velocity = lander->GetVelocity();
    const float* rotation = lander->GetRotation();
    
    if (!m3DMode) {
        // 2D mode + thrust is opposite to gravity (upward)
        velocity[1] += thrustAccel * deltaTime;
    } else {
        // 3D mode - thrust depends on lander orientation
        // Convert rotation from degrees to radians
        float rotX = rotation[0] * M_PI / 180.0f;
        float rotY = rotation[1] * M_PI / 180.0f;
        float rotZ = rotation[2] * M_PI / 180.0f;
        
        // Calculate thrust vector based on rotation
        float thrustX = sin(rotZ) * thrustAccel;
        float thrustY = cos(rotZ) * thrustAccel;
        float thrustZ = sin(rotX) * thrustAccel;
        
        // Apply thrust acceleration to velocity
        velocity[0] -= thrustX * deltaTime;
        velocity[1] -= thrustY * deltaTime;
        velocity[2] -= thrustZ * deltaTime;
    }
}

// Apply drag forces to the lander
void Physics::ApplyDrag(Lander* lander, float deltaTime) {
    if (!lander || lander->IsLanded() || lander->IsCrashed() || mAirDensity <= 0) {
        return; // No drag in vacuum or when landed
    }
    
    // Get lander velocity
    float* velocity = lander->GetVelocity();
    
    // Calculate drag force
    // Drag = 0.5 * density * velocity^2 * drag_coefficient * area
    float dragCoefficient = 0.5f;
    
    // Convert lander dimensions from pixels to meters
    float width = lander->GetWidth() / mPixelsPerMeter;
    float height = lander->GetHeight() / mPixelsPerMeter;
    float area = width * height; // Simplified cross-sectional area
    
    // Apply drag in each direction
    for (int i = 0; i < (m3DMode ? 3 : 2); i++) {
        float speed = velocity[i];
        float dragForce = 0.5f * mAirDensity * speed * std::abs(speed) * dragCoefficient * area;
        
        // Convert force to acceleration (F = ma, so a = F/m)
        float dragAccel = dragForce / lander->GetMass();
        
        // Drag always opposes motion
        if (speed != 0) {
            velocity[i] -= dragAccel * deltaTime;
        }
    }
}

// 2D physics update
void Physics::Update2D(float deltaTime) {
    if (!mLander || !mTerrain) {
        return;
    }
    
    // Scale deltaTime to adjust simulation speed
    float scaledDeltaTime = deltaTime * mTimeScale;
    
    // Save current velocity for debugging
    float* velocity = mLander->GetVelocity();
    float oldVelY = velocity[1];
    
    // Apply forces to update velocity
    ApplyGravity(mLander, scaledDeltaTime);
    ApplyThrust(mLander, scaledDeltaTime);
    ApplyDrag(mLander, scaledDeltaTime);
    
    
    // Update position based on velocity (Euler integration)
    if (!mLander->IsLanded() && !mLander->IsCrashed()) {
        const float* position = mLander->GetPosition();
        
        // Update position (in meters)
        float newX = position[0] + velocity[0] * scaledDeltaTime;
        float newY = position[1] + velocity[1] * scaledDeltaTime;
        
        // Set new position
        mLander->SetPosition(newX, newY);
        
    }
    
    // Check for collisions
    CheckCollisions2D();
}

bool Physics::CheckCollisions2D() {
    if (!mLander || !mTerrain) {
        return false;
    }
    
    // Skip if already landed or crashed
    if (mLander->IsLanded() || mLander->IsCrashed()) {
        return false;
    }
    
    // Check for collision with terrain
    float collisionHeight = 0.0f;
    if (mTerrain->CheckCollision2D(mLander, collisionHeight)) {
        // Collision detected
        
        // Update lander position to be at collision point
        const float* position = mLander->GetPosition();
        float landerHeight = mLander->GetHeight() / mPixelsPerMeter; // Convert to meters
        
        mLander->SetPosition(position[0], collisionHeight + landerHeight / 2);
        
        // Check if this is a valid landing
        if (mTerrain->IsValidLanding2D(mLander)) {
            // Safe landing
            mLander->SetLanded(true);
            
            // Stop movement
            float* velocity = mLander->GetVelocity();
            velocity[0] = velocity[1] = 0.0f;
            
            std::cout << "Successful landing!" << std::endl;
        } else {
            // Crash landing
            mLander->SetCrashed(true);
            
            // Stop movement
            float* velocity = mLander->GetVelocity();
            velocity[0] = velocity[1] = 0.0f;
            
            std::cout << "Crash landing!" << std::endl;
        }
        
        return true;
    }
    
    return false;
}

// 3D physics update
void Physics::Update3D(float deltaTime) {
    // Implementation similar to Update2D but for 3D
    if (!mLander || !mTerrain) {
        return;
    }
    
    // Apply forces
    ApplyGravity(mLander, deltaTime);
    ApplyThrust(mLander, deltaTime);
    ApplyDrag(mLander, deltaTime);
    
    // Update position based on velocity (simple Euler integration)
    if (!mLander->IsLanded() && !mLander->IsCrashed()) {
        float* velocity = mLander->GetVelocity();
        const float* position = mLander->GetPosition();
        
        // New position = old position + velocity * deltaTime
        float newX = position[0] + velocity[0] * deltaTime;
        float newY = position[1] + velocity[1] * deltaTime;
        float newZ = position[2] + velocity[2] * deltaTime;
        
        mLander->SetPosition(newX, newY, newZ);
    }
    
    // Check for collisions
    CheckCollisions3D();
}

bool Physics::CheckCollisions3D() {
    // 3D collision implementation
    if (!mLander || !mTerrain) {
        return false;
    }
    
    // Skip if already landed or crashed
    if (mLander->IsLanded() || mLander->IsCrashed()) {
        return false;
    }
    
    // Check for collision with terrain
    float collisionHeight = 0.0f;
    if (mTerrain->CheckCollision3D(mLander, collisionHeight)) {
        // Collision detected
        
        // Update lander position to be at collision point
        const float* position = mLander->GetPosition();
        float landerHeight = mLander->GetHeight() / mPixelsPerMeter; // Convert to meters
        
        mLander->SetPosition(position[0], collisionHeight + landerHeight / 2, position[2]);
        
        // Check if this is a valid landing
        if (mTerrain->IsValidLanding3D(mLander)) {
            // Safe landing
            mLander->SetLanded(true);
            
            // Stop movement
            float* velocity = mLander->GetVelocity();
            velocity[0] = velocity[1] = velocity[2] = 0.0f;
            
            std::cout << "Successful 3D landing!" << std::endl;
        } else {
            // Crash landing
            mLander->SetCrashed(true);
            
            // Stop movement
            float* velocity = mLander->GetVelocity();
            velocity[0] = velocity[1] = velocity[2] = 0.0f;
            
            std::cout << "Crash landing in 3D!" << std::endl;
        }
        
        return true;
    }
    
    return false;
}
