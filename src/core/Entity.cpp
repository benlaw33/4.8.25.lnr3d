// Entity.cpp
// Implementation of Entity and Lander classes

#include "Entity.h"
#include "../rendering/Renderer.h" // Include full Renderer definition
#include <algorithm>
#include <iostream>

// Initialize static ID counter
int Entity::sNextID = 0;

// Base Entity implementation
Entity::Entity() 
    : mActive(true)
    , mID(sNextID++)
    , mName("Entity")
{
    // Initialize position, rotation, and scale
    mPosition[0] = mPosition[1] = mPosition[2] = 0.0f;
    mRotation[0] = mRotation[1] = mRotation[2] = 0.0f;
    mScale[0] = mScale[1] = mScale[2] = 1.0f;
}

void Entity::SetPosition(float x, float y, float z) {
    mPosition[0] = x;
    mPosition[1] = y;
    mPosition[2] = z;
}

void Entity::SetRotation(float x, float y, float z) {
    mRotation[0] = x;
    mRotation[1] = y;
    mRotation[2] = z;
}

void Entity::SetScale(float x, float y, float z) {
    mScale[0] = x;
    mScale[1] = y;
    mScale[2] = z;
}

// Lander implementation
Lander::Lander()
    : Entity()
    , mWidth(20.0f)         // Width in pixels
    , mHeight(30.0f)        // Height in pixels
    , mDepth(20.0f)         // Depth in pixels (for 3D)
    , mMass(1000.0f)        // Mass in kg (1 metric ton)
    , mThrustLevel(0.0f)    // Current thrust level (0-1)
    , mThrustActive(false)  // Whether thrust is currently active
    , mMaxThrustForce(25000.0f) // Max thrust in Newtons (25 kN)
    , mFuel(1000.0f)        // Fuel in kg
    , mMaxFuel(1000.0f)     // Max fuel capacity in kg
    , mFuelConsumptionRate(10.0f) // Fuel consumption in kg/s at max thrust
    , mLanded(false)        // Landing status
    , mCrashed(false)       // Crash status
{
    mName = "Lander";
    
    // Initialize velocity and acceleration (in m/s and m/s²)
    mVelocity[0] = mVelocity[1] = mVelocity[2] = 0.0f;
    mAcceleration[0] = mAcceleration[1] = mAcceleration[2] = 0.0f;
    
    // Log creation
    std::cout << "Lander created with mass: " << mMass 
              << " kg, max thrust: " << mMaxThrustForce 
              << " N (TWR: " << (mMaxThrustForce / (mMass * 1.62f)) 
              << ")" << std::endl;
}

void Lander::Update(float deltaTime) {
    // This is now primarily a place for entity-specific logic
    // Main physics updates are handled by the Physics system
    
    // Handle fuel consumption if thrust is active
    if (mThrustActive && mFuel > 0) {
        // Fuel consumption is proportional to thrust level
        float consumptionRate = mFuelConsumptionRate * mThrustLevel;
        mFuel -= consumptionRate * deltaTime;
        mFuel = std::max(0.0f, mFuel);
        
        if (mFuel <= 0) {
            mThrustActive = false;
            mThrustLevel = 0.0f;
            std::cout << "Out of fuel!" << std::endl;
        }
    }
}

void Lander::Render(Renderer* renderer) {
    // Delegate rendering to the renderer
    renderer->RenderLander(this);
}

void Lander::ApplyThrust(float amount) {
    if (mFuel <= 0) {
        mThrustActive = false;
        mThrustLevel = 0.0f;
        return;
    }
    
    mThrustLevel = std::max(0.0f, std::min(1.0f, amount));
    mThrustActive = mThrustLevel > 0.0f;
    
    // Debug output
    if (mThrustActive) {
        std::cout << "Thrust applied: " << (mThrustLevel * 100) << "%" << std::endl;
    }
}

void Lander::RotateLeft(float amount) {
    mRotation[2] += amount;
    // Normalize rotation to 0-360 degrees
    while (mRotation[2] >= 360.0f) mRotation[2] -= 360.0f;
}

void Lander::RotateRight(float amount) {
    mRotation[2] -= amount;
    // Normalize rotation to 0-360 degrees
    while (mRotation[2] < 0.0f) mRotation[2] += 360.0f;
}

void Lander::Reset() {
    // Reset position
    SetPosition(0.0f, 5.0f, 0.0f); // Position in meters (y=5m above ground)
    
    // Reset rotation
    SetRotation(0.0f, 0.0f, 0.0f);
    
    // Reset velocity and acceleration
    mVelocity[0] = mVelocity[1] = mVelocity[2] = 0.0f;
    mAcceleration[0] = mAcceleration[1] = mAcceleration[2] = 0.0f;
    
    // Reset thrust
    mThrustLevel = 0.0f;
    mThrustActive = false;
    
    // Reset fuel
    mFuel = mMaxFuel;
    
    // Reset landing status
    mLanded = false;
    mCrashed = false;
    
    // Reset active status
    mActive = true;
    
    std::cout << "Lander reset to initial state" << std::endl;
}