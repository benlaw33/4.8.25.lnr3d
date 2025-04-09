// Entity.h
// Base classes for game entities

#pragma once

#include <vector>
#include <string>

// Forward declarations
class Renderer;
class Physics;

// Common base class for all game entities
class Entity {
public:
    Entity();
    virtual ~Entity() = default;
    
    // Core entity methods
    virtual void Update(float deltaTime) = 0;
    virtual void Render(Renderer* renderer) = 0;
    
    // Position and orientation getters/setters
    void SetPosition(float x, float y, float z = 0.0f);
    const float* GetPosition() const { return mPosition; }
    
    void SetRotation(float x, float y, float z = 0.0f);
    const float* GetRotation() const { return mRotation; }
    
    void SetScale(float x, float y, float z = 1.0f);
    const float* GetScale() const { return mScale; }
    
    // Entity state
    bool IsActive() const { return mActive; }
    void SetActive(bool active) { mActive = active; }
    
    // Entity identification
    int GetID() const { return mID; }
    const std::string& GetName() const { return mName; }
    void SetName(const std::string& name) { mName = name; }

protected:
    // Spatial properties
    float mPosition[3]; // x, y, z
    float mRotation[3]; // x, y, z (in degrees)
    float mScale[3];    // x, y, z
    
    // Entity state
    bool mActive;
    
    // Entity identification
    int mID;
    std::string mName;
    
    // Auto-incrementing ID counter for unique entity IDs
    static int sNextID;
};

// Lander entity
class Lander : public Entity {
public:
    Lander();
    virtual ~Lander() = default;
    
    // Implement Entity methods
    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;
    
    // Lander-specific methods
    void ApplyThrust(float amount);
    void RotateLeft(float amount);
    void RotateRight(float amount);
    void Reset();
    
    // Getters
    float GetFuel() const { return mFuel; }
    float GetMaxFuel() const { return mMaxFuel; }
    float GetThrustLevel() const { return mThrustLevel; }
    bool IsThrustActive() const { return mThrustActive; }
    bool IsLanded() const { return mLanded; }
    bool IsCrashed() const { return mCrashed; }
    
    // Physics properties
    float* GetVelocity() { return mVelocity; }
    const float* GetVelocity() const { return mVelocity; }
    float GetMass() const { return mMass; }
    float GetWidth() const { return mWidth; }
    float GetHeight() const { return mHeight; }
    float GetDepth() const { return mDepth; } // For 3D
    
    // Status settings
    void SetLanded(bool landed) { mLanded = landed; }
    void SetCrashed(bool crashed) { mCrashed = crashed; }

private:
    // Physical properties
    float mWidth;
    float mHeight;
    float mDepth;  // For 3D
    float mMass;   // kg
    
    // Movement properties
    float mVelocity[3]; // vx, vy, vz
    float mAcceleration[3]; // ax, ay, az
    
    // Thrust properties
    float mThrustLevel;     // 0.0 - 1.0
    bool mThrustActive;
    float mMaxThrustForce;  // Newtons
    
    // Fuel properties
    float mFuel;
    float mMaxFuel;
    float mFuelConsumptionRate;
    
    // Landing status
    bool mLanded;
    bool mCrashed;
};