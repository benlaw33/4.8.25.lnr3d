// Physics.cpp
// Implementation of the physics system with Bullet Physics

#include "Physics.h"
#include "Entity.h"
#include <cmath>
#include <iostream>
#include <algorithm>

Physics::Physics()
    : mGravity(1.62f)      // Lunar gravity (m/s²)
    , mAirDensity(0.0f)    // No atmosphere on the moon
    , m3DMode(false)       // Start in 2D mode
    , mLander(nullptr)
    , mTerrain(nullptr)
    , mTimeScale(1.0f)     // Normal simulation speed (1:1)
    , mPixelsPerMeter(20.0f) // Conversion factor: 20 pixels = 1 meter
    , mCollisionConfiguration(nullptr)
    , mDispatcher(nullptr)
    , mBroadphase(nullptr)
    , mSolver(nullptr)
    , mDynamicsWorld(nullptr)
    , mLanderRigidBody(nullptr)
    , mSoftBodyCollisionConfiguration(nullptr)
    , mSoftRigidDynamicsWorld(nullptr)
{
}

Physics::~Physics() {
    CleanupBulletPhysics();
}

void Physics::Initialize() {
    // Initialize physics system
    std::cout << "Physics system initialized with Lunar gravity: " << mGravity << " m/s²" << std::endl;
    
    // Initialize Bullet Physics if in 3D mode
    if (m3DMode) {
        InitializeBulletPhysics();
    }
}

void Physics::InitializeBulletPhysics() {
    // Clean up previous instance if any
    CleanupBulletPhysics();
    
    // Create collision configuration
    mCollisionConfiguration = new btDefaultCollisionConfiguration();
    mDispatcher = new btCollisionDispatcher(mCollisionConfiguration);
    
    // Create broadphase
    mBroadphase = new btDbvtBroadphase();
    
    // Create solver
    mSolver = new btSequentialImpulseConstraintSolver();
    
    // Create dynamics world
    mDynamicsWorld = new btDiscreteDynamicsWorld(mDispatcher, mBroadphase, mSolver, mCollisionConfiguration);
    
    // Set gravity
    mDynamicsWorld->setGravity(btVector3(0, -mGravity, 0));
    
    // Initialize soft body world info for regolith simulation
    mSoftBodyWorldInfo.air_density = mAirDensity;
    mSoftBodyWorldInfo.water_density = 0;
    mSoftBodyWorldInfo.water_offset = 0;
    mSoftBodyWorldInfo.water_normal = btVector3(0, 0, 0);
    mSoftBodyWorldInfo.m_gravity.setValue(0, -mGravity, 0);
    mSoftBodyWorldInfo.m_broadphase = mBroadphase;
    mSoftBodyWorldInfo.m_dispatcher = mDispatcher;
    
    // Create soft body physics world
    mSoftBodyCollisionConfiguration = new btSoftBodyRigidBodyCollisionConfiguration();
    btCollisionDispatcher* softDispatcher = new btCollisionDispatcher(mSoftBodyCollisionConfiguration);
    mSoftRigidDynamicsWorld = new btSoftRigidDynamicsWorld(softDispatcher, mBroadphase, mSolver, mSoftBodyCollisionConfiguration);
    mSoftRigidDynamicsWorld->setGravity(btVector3(0, -mGravity, 0));
    
    std::cout << "Bullet Physics initialized" << std::endl;
}

void Physics::CleanupBulletPhysics() {
    // Clean up rigid bodies
    if (mLanderRigidBody) {
        if (mDynamicsWorld) {
            mDynamicsWorld->removeRigidBody(mLanderRigidBody);
        }
        delete mLanderRigidBody->getMotionState();
        delete mLanderRigidBody->getCollisionShape();
        delete mLanderRigidBody;
        mLanderRigidBody = nullptr;
    }
    
    for (auto body : mTerrainRigidBodies) {
        if (mDynamicsWorld) {
            mDynamicsWorld->removeRigidBody(body);
        }
        delete body->getMotionState();
        delete body->getCollisionShape();
        delete body;
    }
    mTerrainRigidBodies.clear();
    
    // Clean up Bullet Physics objects in reverse order of creation
    delete mSoftRigidDynamicsWorld;
    delete mSoftBodyCollisionConfiguration;
    delete mDynamicsWorld;
    delete mSolver;
    delete mBroadphase;
    delete mDispatcher;
    delete mCollisionConfiguration;
    
    mSoftRigidDynamicsWorld = nullptr;
    mSoftBodyCollisionConfiguration = nullptr;
    mDynamicsWorld = nullptr;
    mSolver = nullptr;
    mBroadphase = nullptr;
    mDispatcher = nullptr;
    mCollisionConfiguration = nullptr;
}

void Physics::RegisterLander(Lander* lander) {
    mLander = lander;
    
    // Create rigid body for lander if in 3D mode
    if (m3DMode && mDynamicsWorld) {
        CreateLanderRigidBody(lander);
    }
}

void Physics::RegisterTerrain(Terrain* terrain) {
    mTerrain = terrain;
    
    // Create rigid bodies for terrain if in 3D mode
    if (m3DMode && mDynamicsWorld) {
        CreateTerrainRigidBodies(terrain);
        CreateRegolithSoftBody(terrain);
    }
}

void Physics::SetGravity(float gravity) {
    mGravity = gravity;
    
    // Update Bullet Physics gravity if initialized
    if (mDynamicsWorld) {
        mDynamicsWorld->setGravity(btVector3(0, -mGravity, 0));
    }
    
    if (mSoftRigidDynamicsWorld) {
        mSoftRigidDynamicsWorld->setGravity(btVector3(0, -mGravity, 0));
    }
}

void Physics::Update(float deltaTime) {
    // Scale deltaTime to adjust simulation speed
    float scaledDeltaTime = deltaTime * mTimeScale;
    
    if (m3DMode) {
        // Update Bullet physics simulation
        if (mDynamicsWorld) {
            mDynamicsWorld->stepSimulation(scaledDeltaTime, 10);
        }
        
        // Update soft body physics
        if (mSoftRigidDynamicsWorld) {
            mSoftRigidDynamicsWorld->stepSimulation(scaledDeltaTime, 10);
        }
        
        // Sync lander position with physics
        if (mLander && mLanderRigidBody) {
            SyncLanderWithPhysics(mLander);
        }
        
        // Check for collisions
        CheckCollisions3D();
    } else {
        // Use original 2D physics for backward compatibility
        Update2D(scaledDeltaTime);
    }
}

// Create a rigid body for the lander
void Physics::CreateLanderRigidBody(Lander* lander) {
    if (!lander) return;
    
    // Clean up existing rigid body
    if (mLanderRigidBody) {
        mDynamicsWorld->removeRigidBody(mLanderRigidBody);
        delete mLanderRigidBody->getMotionState();
        delete mLanderRigidBody->getCollisionShape();
        delete mLanderRigidBody;
        mLanderRigidBody = nullptr;
    }
    
    // Create collision shape for lander
    // Using a box shape for simplicity
    float width = lander->GetWidth() / (2.0f * mPixelsPerMeter);
    float height = lander->GetHeight() / (2.0f * mPixelsPerMeter);
    float depth = lander->GetDepth() / (2.0f * mPixelsPerMeter);
    btCollisionShape* landerShape = new btBoxShape(btVector3(width, height, depth));
    
    // Create motion state
    const float* position = lander->GetPosition();
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(position[0], position[1], position[2]));
    
    // Handle rotation
    const float* rotation = lander->GetRotation();
    btQuaternion quat;
    quat.setEulerZYX(
        rotation[2] * (M_PI / 180.0f),  // Z rotation (in radians)
        rotation[1] * (M_PI / 180.0f),  // Y rotation
        rotation[0] * (M_PI / 180.0f)   // X rotation
    );
    startTransform.setRotation(quat);
    
    btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);
    
    // Calculate inertia
    btVector3 localInertia(0, 0, 0);
    float mass = lander->GetMass();
    landerShape->calculateLocalInertia(mass, localInertia);
    
    // Create rigid body
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, landerShape, localInertia);
    mLanderRigidBody = new btRigidBody(rbInfo);
    
    // Set damping
    mLanderRigidBody->setDamping(0.1f, 0.1f);
    
    // Add to world
    mDynamicsWorld->addRigidBody(mLanderRigidBody);
    
    std::cout << "Created rigid body for lander with mass: " << mass << " kg" << std::endl;
}

// Create rigid bodies for terrain
void Physics::CreateTerrainRigidBodies(Terrain* terrain) {
    if (!terrain) return;
    
    // Clean up existing rigid bodies
    for (auto body : mTerrainRigidBodies) {
        mDynamicsWorld->removeRigidBody(body);
        delete body->getMotionState();
        delete body->getCollisionShape();
        delete body;
    }
    mTerrainRigidBodies.clear();
    
    // Get terrain triangles
    const std::vector<TerrainTriangle>& triangles = terrain->GetTriangles3D();
    
    if (triangles.empty()) {
        std::cout << "No terrain triangles to create rigid bodies for" << std::endl;
        return;
    }
    
    // Create a single trimesh for all terrain
    btTriangleMesh* terrainMesh = new btTriangleMesh();
    
    for (const auto& triangle : triangles) {
        // Extract vertices
        btVector3 v1(triangle.vertices[0], triangle.vertices[1], triangle.vertices[2]);
        btVector3 v2(triangle.vertices[3], triangle.vertices[4], triangle.vertices[5]);
        btVector3 v3(triangle.vertices[6], triangle.vertices[7], triangle.vertices[8]);
        
        // Add triangle to mesh
        terrainMesh->addTriangle(v1, v2, v3);
    }
    
    // Create terrain shape
    btBvhTriangleMeshShape* terrainShape = new btBvhTriangleMeshShape(terrainMesh, true);
    
    // Create motion state (terrain is static)
    btTransform terrainTransform;
    terrainTransform.setIdentity();
    btDefaultMotionState* terrainMotionState = new btDefaultMotionState(terrainTransform);
    
    // Create rigid body (mass = 0 for static objects)
    btRigidBody::btRigidBodyConstructionInfo terrainRbInfo(0, terrainMotionState, terrainShape, btVector3(0, 0, 0));
    btRigidBody* terrainBody = new btRigidBody(terrainRbInfo);
    
    // Set friction
    terrainBody->setFriction(0.5f);
    
    // Add to world
    mDynamicsWorld->addRigidBody(terrainBody);
    mTerrainRigidBodies.push_back(terrainBody);
    
    std::cout << "Created rigid body for terrain with " << triangles.size() << " triangles" << std::endl;
}

// Create a soft body for regolith simulation
void Physics::CreateRegolithSoftBody(Terrain* terrain) {
    if (!terrain || !mSoftRigidDynamicsWorld) return;
    
    // Get terrain dimensions
    int width = terrain->GetWidth();
    int length = terrain->GetLength();
    
    // Get terrain triangles
    const std::vector<TerrainTriangle>& triangles = terrain->GetTriangles3D();
    
    if (triangles.empty()) {
        std::cout << "No terrain triangles to create regolith simulation for" << std::endl;
        return;
    }
    
    // Find landing pad boundaries
    float minX = FLT_MAX, maxX = -FLT_MAX;
    float minZ = FLT_MAX, maxZ = -FLT_MAX;
    float avgY = 0.0f;
    int landingPadTriangles = 0;
    
    for (const auto& triangle : triangles) {
        if (triangle.isLandingPad) {
            // Extract vertices
            float x1 = triangle.vertices[0];
            float y1 = triangle.vertices[1];
            float z1 = triangle.vertices[2];
            
            float x2 = triangle.vertices[3];
            float y2 = triangle.vertices[4];
            float z2 = triangle.vertices[5];
            
            float x3 = triangle.vertices[6];
            float y3 = triangle.vertices[7];
            float z3 = triangle.vertices[8];
            
            // Update boundaries
            minX = std::min({minX, x1, x2, x3});
            maxX = std::max({maxX, x1, x2, x3});
            minZ = std::min({minZ, z1, z2, z3});
            maxZ = std::max({maxZ, z1, z2, z3});
            
            // Sum up heights for average
            avgY += (y1 + y2 + y3);
            landingPadTriangles++;
        }
    }
    
    // Create regolith soft body if landing pad found
    if (landingPadTriangles > 0) {
        // Calculate average height
        avgY /= (landingPadTriangles * 3);
        
        // Create a cloth-like soft body for regolith
        const int res = 20; // Resolution of regolith grid
        
        // Create a patch for the landing pad area
        btSoftBody* regolithBody = btSoftBodyHelpers::CreatePatch(
            mSoftBodyWorldInfo,
            btVector3(minX, avgY + 0.05f, minZ),       // Corner 00 (slight offset above terrain)
            btVector3(maxX, avgY + 0.05f, minZ),       // Corner 10
            btVector3(minX, avgY + 0.05f, maxZ),       // Corner 01
            btVector3(maxX, avgY + 0.05f, maxZ),       // Corner 11
            res, res,                                   // Resolution
            1 + 2 + 4 + 8,                             // Fix all corners
            true                                        // Generate diagonal links
        );
        
        if (regolithBody) {
            // Configure material properties for lunar regolith
            regolithBody->m_materials[0]->m_kLST = 0.1f;  // Linear stiffness (low for soft regolith)
            regolithBody->m_materials[0]->m_kAST = 0.1f;  // Area stiffness
            regolithBody->m_materials[0]->m_kVST = 0.1f;  // Volume stiffness
            
            // Configure soft body settings
            regolithBody->m_cfg.kDF = 0.5f;               // Dynamic friction coefficient
            regolithBody->m_cfg.kDP = 0.01f;              // Damping coefficient
            regolithBody->m_cfg.kPR = 0.1f;               // Pressure coefficient
            regolithBody->m_cfg.kVC = 0.2f;               // Volume conservation coefficient
            
            // Generate clusters for better deformation
            regolithBody->generateClusters(16);
            regolithBody->generateBendingConstraints(2);
            
            // Add soft body to world
            mSoftRigidDynamicsWorld->addSoftBody(regolithBody);
            
            std::cout << "Created regolith soft body simulation over landing pad area" << std::endl;
        } else {
            std::cout << "Failed to create regolith soft body" << std::endl;
        }
    } else {
        std::cout << "No landing pad found for regolith simulation" << std::endl;
    }
}

// Sync lander entity with Bullet Physics rigid body
void Physics::SyncLanderWithPhysics(Lander* lander) {
    if (!lander || !mLanderRigidBody) return;
    
    // Get transform from rigid body
    btTransform transform;
    mLanderRigidBody->getMotionState()->getWorldTransform(transform);
    
    // Update position
    btVector3 position = transform.getOrigin();
    lander->SetPosition(position.x(), position.y(), position.z());
    
    // Update rotation
    btQuaternion rotation = transform.getRotation();
    btScalar roll, pitch, yaw;
    transform.getBasis().getEulerZYX(yaw, pitch, roll);
    
    // Convert to degrees
    lander->SetRotation(
        roll * (180.0f / M_PI),
        pitch * (180.0f / M_PI),
        yaw * (180.0f / M_PI)
    );
    
    // Update velocity
    btVector3 velocity = mLanderRigidBody->getLinearVelocity();
    float* landerVelocity = lander->GetVelocity();
    landerVelocity[0] = velocity.x();
    landerVelocity[1] = velocity.y();
    landerVelocity[2] = velocity.z();
}

// Apply thrust to lander using Bullet Physics
void Physics::ApplyThrust(Lander* lander, float deltaTime) {
    if (!lander || !mLanderRigidBody || !lander->IsThrustActive()) return;
    
    // Calculate thrust force based on lander properties
    float maxThrust = lander->GetMass() * 2.5f * mGravity; // Thrust-to-weight ratio of 2.5
    float thrustForce = maxThrust * lander->GetThrustLevel();
    
    // Get lander rotation
    const float* rotation = lander->GetRotation();
    
    // Calculate thrust direction based on lander orientation
    btVector3 thrustDirection(0, 1, 0); // Default is upward
    
    if (m3DMode) {
        // Convert rotation from degrees to radians
        float rotX = rotation[0] * (M_PI / 180.0f);
        float rotY = rotation[1] * (M_PI / 180.0f);
        float rotZ = rotation[2] * (M_PI / 180.0f);
        
        // Create rotation matrix from Euler angles
        btQuaternion quat;
        quat.setEulerZYX(rotZ, rotY, rotX);
        btMatrix3x3 rotMatrix(quat);
        
        // Transform thrust direction by rotation
        thrustDirection = rotMatrix * btVector3(0, 1, 0);
    } else {
        // 2D rotation around Z axis only
        float rotZ = rotation[2] * (M_PI / 180.0f);
        thrustDirection = btVector3(sin(rotZ), cos(rotZ), 0);
    }
    
    // Scale direction by force
    btVector3 thrustVector = thrustDirection * thrustForce;
    
    // Apply force at center of mass
    mLanderRigidBody->applyCentralForce(thrustVector);
    
    // Track fuel consumption (in kg/s)
    float fuelConsumptionRate = 10.0f; // Same as original
    float fuelUsed = fuelConsumptionRate * lander->GetThrustLevel() * deltaTime;
    float currentFuel = lander->GetFuel();
    float newFuel = std::max(0.0f, currentFuel - fuelUsed);
    
    // Update lander fuel
    if (currentFuel > 0 && newFuel <= 0) {
        // Out of fuel
        lander->ApplyThrust(0.0f); // Turn off thrust
    }
}

// Legacy 2D physics methods (kept for backward compatibility)
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
    // Note: Using original code for 2D mode
    
    if (!mLander->IsLanded() && !mLander->IsCrashed()) {
        // Apply gravity
        velocity[1] -= mGravity * scaledDeltaTime;
        
        // Apply thrust if active
        if (mLander->IsThrustActive()) {
            const float* rotation = mLander->GetRotation();
            float rotZ = rotation[2] * (M_PI / 180.0f);
            
            // Calculate thrust components
            float thrustX = -sin(rotZ);
            float thrustY = cos(rotZ);
            
            // Calculate thrust acceleration
            float maxThrust = mLander->GetMass() * 2.5f * mGravity;
            float thrustAccel = (maxThrust * mLander->GetThrustLevel()) / mLander->GetMass();
            
            // Apply thrust acceleration
            velocity[0] += thrustX * thrustAccel * scaledDeltaTime;
            velocity[1] += thrustY * thrustAccel * scaledDeltaTime;
        }
        
        // Update position based on velocity (Euler integration)
        const float* position = mLander->GetPosition();
        float newX = position[0] + velocity[0] * scaledDeltaTime;
        float newY = position[1] + velocity[1] * scaledDeltaTime;
        
        // Set new position
        mLander->SetPosition(newX, newY);
    }
    
    // Check for collisions
    CheckCollisions2D();
}

bool Physics::CheckCollisions2D() {
    // 2D collision detection code (kept from original)
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

// 3D physics collision checking using Bullet Physics
bool Physics::CheckCollisions3D() {
    if (!mLander || !mLanderRigidBody) {
        return false;
    }
    
    // Skip if already landed or crashed
    if (mLander->IsLanded() || mLander->IsCrashed()) {
        return false;
    }
    
    // Check contacts with Bullet Physics
    bool isColliding = false;
    bool isValidLanding = false;
    
    // Get lander velocity
    btVector3 velocity = mLanderRigidBody->getLinearVelocity();
    float verticalVelocity = velocity.y();
    float horizontalVelocity = sqrt(velocity.x() * velocity.x() + velocity.z() * velocity.z());
    
    // Check all contact points in the dynamics world
    int numManifolds = mDynamicsWorld->getDispatcher()->getNumManifolds();
    for (int i = 0; i < numManifolds; i++) {
        btPersistentManifold* contactManifold = mDynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
        
        // Check if the lander is one of the colliding objects
        const btCollisionObject* obA = contactManifold->getBody0();
        const btCollisionObject* obB = contactManifold->getBody1();
        
        if (obA == mLanderRigidBody || obB == mLanderRigidBody) {
            int numContacts = contactManifold->getNumContacts();
            if (numContacts > 0) {
                isColliding = true;
                
                // Check if this is a landing pad
                btCollisionObject* terrainObj = (obA == mLanderRigidBody) ? (btCollisionObject*)obB : (btCollisionObject*)obA;
                
                // Determine if this is the landing pad by checking against terrain bodies
                bool isLandingPad = false;
                for (auto terrainBody : mTerrainRigidBodies) {
                    if (terrainObj == terrainBody) {
                        // In a real implementation, we would check if the specific contact triangles 
                        // are part of the landing pad. For simplicity, we'll just check velocity.
                        isLandingPad = true;
                        break;
                    }
                }
                
                // Check landing conditions
                if (isLandingPad) {
                    // Safe landing conditions:
                    // 1. Vertical velocity must be low (downward or slightly upward)
                    // 2. Horizontal velocity must be low
                    const float safeVerticalVelocity = 2.0f;   // m/s
                    const float safeHorizontalVelocity = 1.0f; // m/s
                    
                    bool safeVertical = verticalVelocity > -safeVerticalVelocity && verticalVelocity < safeVerticalVelocity;
                    bool safeHorizontal = horizontalVelocity < safeHorizontalVelocity;
                    
                    if (safeVertical && safeHorizontal) {
                        isValidLanding = true;
                    }
                }
                
                break; // Found a collision with the lander
            }
        }
    }
    
    // Update lander state based on collision
    if (isColliding) {
        if (isValidLanding) {
            // Safe landing
            mLander->SetLanded(true);
            
            // Stop movement and fix position
            mLanderRigidBody->setLinearVelocity(btVector3(0, 0, 0));
            mLanderRigidBody->setAngularVelocity(btVector3(0, 0, 0));
            
            // Make it static
            mLanderRigidBody->setMassProps(0, btVector3(0, 0, 0));
            
            std::cout << "Successful 3D landing!" << std::endl;
        } else {
            // Crash landing
            mLander->SetCrashed(true);
            
            // Let physics handle the crash - don't fix position
            
            std::cout << "Crash landing in 3D!" << std::endl;
        }
        
        return true;
    }
    
    return false;
}

bool Physics::CheckCollisions() {
    // Choose the appropriate collision method based on mode
    if (m3DMode) {
        return CheckCollisions3D();
    } else {
        return CheckCollisions2D();
    }
}