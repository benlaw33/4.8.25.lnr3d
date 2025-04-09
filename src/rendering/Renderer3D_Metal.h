// Renderer3D_Metal.h
// 3D rendering implementation using Metal and SDL2

#pragma once

#include "../compat.h"
#include "Renderer.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>

// Forward declarations for Metal types (to avoid including Metal headers here)
namespace MTL {
    class Device;
    class CommandQueue;
    class Library;
    class RenderPipelineState;
    class Buffer;
    class Texture;
    class RenderPassDescriptor;
    class DepthStencilState;
}

namespace CA {
    class MetalLayer;
    class MetalDrawable;
}

// Vertex structure for Metal
struct Vertex {
    float position[3];
    float normal[3];
    float isLandingPad;   // 1.0 for landing pad, 0.0 for normal terrain
    float entityType;     // 1.0 for lander, 0.0 for terrain
};

// Vertex shader uniforms
struct VertexUniforms {
    float modelMatrix[16];
    float viewMatrix[16];
    float projectionMatrix[16];
};

// Fragment shader uniforms
struct FragmentUniforms {
    float lightPosition[3];
    float ambientLight[3];
    float cameraPosition[3];
};

// Simple Matrix4x4 struct
struct Matrix4x4 {
    float values[16];
};

class Renderer3D_Metal : public Renderer {
public:
    Renderer3D_Metal();
    virtual ~Renderer3D_Metal();
    
    // Implement Renderer interface
    bool Initialize(int width, int height, const std::string& title) override;
    void Shutdown() override;
    void Clear() override;
    void Present() override;
    
    void RenderLander(Lander* lander) override;
    void RenderTerrain(Terrain* terrain) override;
    
    void RenderTelemetry(Game* game) override;
    void RenderGameState(Game* game) override;
    
    int GetWidth() const override { return mWidth; }
    int GetHeight() const override { return mHeight; }
    bool IsInitialized() const override { return mInitialized; }
    
    // 3D camera methods
    void SetCameraPosition(float x, float y, float z) override;
    void SetCameraTarget(float x, float y, float z) override;
    void SetCameraUp(float x, float y, float z) override;
    
    // 3D lighting methods
    void SetLightPosition(float x, float y, float z) override;
    void SetAmbientLight(float r, float g, float b) override;
    
private:
    // Initialize Metal
    bool InitializeMetal();
    
    // Bridge function to connect Metal-cpp with Cocoa APIs
    void SetMetalLayerForWindow(void* nsWindowPtr, CA::MetalLayer* layer);
    
    // Load Metal shader library
    bool LoadShaders();
    
    // Create render pipeline
    bool CreateRenderPipeline();
    
    // Create buffers for models
    bool CreateGeometryBuffers();
    
    // Create a cube model for the lander
    void CreateCubeModel();
    
    // Update uniform buffers
    void UpdateCameraUniforms();
    void UpdateModelUniforms(const float* position, const float* rotation, const float* scale);
    
    // Helper methods for 3D math
    Matrix4x4 CreateProjectionMatrix(float fov, float aspect, float near, float far);
    Matrix4x4 CreateViewMatrix();
    Matrix4x4 CreateModelMatrix(const float* position, const float* rotation, const float* scale);
    void MultiplyMatrices(Matrix4x4& result, const Matrix4x4& a, const Matrix4x4& b);
    
    // SDL window
    SDL_Window* mWindow;
    
    // Metal objects
    MTL::Device* mDevice;
    MTL::CommandQueue* mCommandQueue;
    MTL::Library* mShaderLibrary;
    MTL::RenderPipelineState* mRenderPipelineState;
    MTL::DepthStencilState* mDepthStencilState;
    CA::MetalLayer* mMetalLayer;
    
    // Buffers
    MTL::Buffer* mLanderVertexBuffer;
    MTL::Buffer* mLanderIndexBuffer;
    MTL::Buffer* mTerrainVertexBuffer;
    MTL::Buffer* mTerrainIndexBuffer;
    MTL::Buffer* mVertexUniformBuffer;
    MTL::Buffer* mFragmentUniformBuffer;
    
    // Textures
    MTL::Texture* mDepthTexture;
    
    // Renderer properties
    int mWidth;
    int mHeight;
    bool mInitialized;
    
    // Model properties
    int mLanderVertexCount;
    int mLanderIndexCount;
    int mTerrainIndexCount;
    
    // Camera properties
    float mCameraPosition[3];
    float mCameraTarget[3];
    float mCameraUp[3];
    
    // Light properties
    float mLightPosition[3];
    float mAmbientLight[3];
    
    // Matrices
    Matrix4x4 mProjectionMatrix;
    Matrix4x4 mViewMatrix;
    Matrix4x4 mModelMatrix;
    
    // Uniform structs
    VertexUniforms mVertexUniforms;
    FragmentUniforms mFragmentUniforms;
};