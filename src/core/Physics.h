// Physics.h
// Physics system for the lunar lander simulation using Bullet Physics

#pragma once

#include "Entity.h"
#include "Terrain.h"
#include <vector>

// Bullet Physics includes
// Bullet Physics includes
#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <bullet/BulletSoftBody/btSoftBodyHelpers.h>
#include <bullet/BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>

class Physics {
public:
    Physics();
    ~Physics();
    
    // Core physics methods
    void Initialize();
    void Update(float deltaTime);
    
    // Register entities with the physics system
    void RegisterLander(Lander* lander);
    void RegisterTerrain(Terrain* terrain);
    
    // Physics constants getters/setters
    float GetGravity() const { return mGravity; }
    void SetGravity(float gravity);
    
    float GetAirDensity() const { return mAirDensity; }
    void SetAirDensity(float density) { mAirDensity = density; }
    
    float GetPixelsPerMeter() const { return mPixelsPerMeter; }
    void SetPixelsPerMeter(float ppm) { mPixelsPerMeter = ppm; }
    
    // Collision detection
    bool CheckCollisions();
    
    // Physics calculations - now using Bullet Physics internally
    void ApplyThrust(Lander* lander, float deltaTime);
    void UpdateLanderPhysics(Lander* lander);
    
    // 2D physics (keeping for backward compatibility)
    void Update2D(float deltaTime);
    bool CheckCollisions2D();
    
    // 3D physics with Bullet
    void Update3D(float deltaTime);
    bool CheckCollisions3D();

private:
    // Physics constants
    float mGravity;         // Lunar gravity (m/s²)
    float mAirDensity;      // Atmospheric density (kg/m³)
    float mTimeScale;       // Time scaling factor for simulation speed
    float mPixelsPerMeter;  // Conversion factor between physics and screen units
    
    // Simulation mode
    bool m3DMode;           // Whether to use 3D physics
    
    // Physics entities
    Lander* mLander;
    Terrain* mTerrain;
    
    // Bullet Physics objects
    btDefaultCollisionConfiguration* mCollisionConfiguration;
    btCollisionDispatcher* mDispatcher;
    btBroadphaseInterface* mBroadphase;
    btSequentialImpulseConstraintSolver* mSolver;
    btDiscreteDynamicsWorld* mDynamicsWorld;
    
    // Regolith simulation (soft body dynamics)
    btSoftBodyWorldInfo mSoftBodyWorldInfo;
    btSoftBodyRigidBodyCollisionConfiguration* mSoftBodyCollisionConfiguration;
    btSoftRigidDynamicsWorld* mSoftRigidDynamicsWorld;
    
    // Rigid bodies
    btRigidBody* mLanderRigidBody;
    std::vector<btRigidBody*> mTerrainRigidBodies;
    
    // Helper methods
    void InitializeBulletPhysics();
    void CleanupBulletPhysics();
    void CreateLanderRigidBody(Lander* lander);
    void CreateTerrainRigidBodies(Terrain* terrain);
    void CreateRegolithSoftBody(Terrain* terrain);
    void SyncLanderWithPhysics(Lander* lander);
};