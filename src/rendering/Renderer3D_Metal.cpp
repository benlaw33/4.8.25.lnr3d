// Renderer3D_Metal.cpp
// Implementation of the Metal renderer

#include "../compat.h"
#include "Renderer3D_Metal.h"
#include "../core/Entity.h"
#include "../core/Terrain.h"
#include "../core/Game.h"
#include <iostream>
#include <cmath>

// Include Metal-cpp headers
#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

// Include SDL window info for Metal layer
#include <SDL2/SDL_syswm.h>

// Forward declaration of the Objective-C++ bridge function
extern "C" {
    // External functions from MetalBridge.mm
    void SetMetalLayerForSDLWindow(void* nsWindowPtr, void* metalLayerPtr);
    void* CreateCAMetalLayer();
    void ReleaseCAMetalLayer(void* layerPtr);
}


// Helper function for loading Metal shaders
NS::String* GetMetalLibraryPath() {
    // Get the path to the compiled Metal library in the app bundle
    return NS::String::string("assets/shaders/default.metallib", NS::UTF8StringEncoding);
}

Renderer3D_Metal::Renderer3D_Metal()
    : mWindow(nullptr)
    , mDevice(nullptr)
    , mCommandQueue(nullptr)
    , mShaderLibrary(nullptr)
    , mRenderPipelineState(nullptr)
    , mDepthStencilState(nullptr)
    , mMetalLayer(nullptr)
    , mLanderVertexBuffer(nullptr)
    , mLanderIndexBuffer(nullptr)
    , mTerrainVertexBuffer(nullptr)
    , mTerrainIndexBuffer(nullptr)
    , mVertexUniformBuffer(nullptr)
    , mFragmentUniformBuffer(nullptr)
    , mDepthTexture(nullptr)
    , mWidth(800)
    , mHeight(600)
    , mInitialized(false)
    , mLanderVertexCount(0)
    , mLanderIndexCount(0)
    , mTerrainIndexCount(0)
{
    // Initialize camera position
    mCameraPosition[0] = 0.0f;
    mCameraPosition[1] = 100.0f;
    mCameraPosition[2] = 200.0f;
    
    // Initialize camera target
    mCameraTarget[0] = 0.0f;
    mCameraTarget[1] = 0.0f;
    mCameraTarget[2] = 0.0f;
    
    // Initialize camera up vector
    mCameraUp[0] = 0.0f;
    mCameraUp[1] = 1.0f;
    mCameraUp[2] = 0.0f;
    
    // Initialize light position
    mLightPosition[0] = 500.0f;
    mLightPosition[1] = 1000.0f;
    mLightPosition[2] = 500.0f;
    
    // Initialize ambient light
    mAmbientLight[0] = 0.3f;
    mAmbientLight[1] = 0.3f;
    mAmbientLight[2] = 0.3f;
}

Renderer3D_Metal::~Renderer3D_Metal() {
    Shutdown();
}

bool Renderer3D_Metal::Initialize(int width, int height, const std::string& title) {
    // Store dimensions
    mWidth = width;
    mHeight = height;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Create window
    mWindow = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        mWidth,
        mHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
    );
    
    if (!mWindow) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Initialize Metal
    if (!InitializeMetal()) {
        std::cerr << "Metal initialization failed!" << std::endl;
        return false;
    }
    
    // Load shaders
    if (!LoadShaders()) {
        std::cerr << "Shader loading failed!" << std::endl;
        return false;
    }
    
    // Create render pipeline
    if (!CreateRenderPipeline()) {
        std::cerr << "Render pipeline creation failed!" << std::endl;
        return false;
    }
    
    // Create geometry buffers
    if (!CreateGeometryBuffers()) {
        std::cerr << "Geometry buffer creation failed!" << std::endl;
        return false;
    }
    
    // Set up projection matrix
    float aspectRatio = (float)mWidth / (float)mHeight;
    mProjectionMatrix = CreateProjectionMatrix(45.0f * (M_PI / 180.0f), aspectRatio, 0.1f, 1000.0f);
    
    // Set up initial view matrix
    mViewMatrix = CreateViewMatrix();
    
    // Initialize uniform structs
    UpdateCameraUniforms();
    
    mInitialized = true;
    std::cout << "Metal renderer initialized" << std::endl;
    return true;
}

bool Renderer3D_Metal::InitializeMetal() {
    // Create Metal device
    mDevice = MTL::CreateSystemDefaultDevice();
    if (!mDevice) {
        std::cerr << "Failed to create Metal device" << std::endl;
        return false;
    }
    
    // Create command queue
    mCommandQueue = mDevice->newCommandQueue();
    if (!mCommandQueue) {
        std::cerr << "Failed to create command queue" << std::endl;
        return false;
    }
    
    // Get window info for Metal layer setup
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(mWindow, &wmInfo)) {
        std::cerr << "Failed to get window info: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Create Metal layer using the Objective-C++ bridge
    void* metalLayerPtr = CreateCAMetalLayer();
    mMetalLayer = reinterpret_cast<CA::MetalLayer*>(metalLayerPtr);

    // Configure the Metal layer
    mMetalLayer->setDevice(mDevice);
    mMetalLayer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    mMetalLayer->setFramebufferOnly(true);
    
    // Set drawable size with high DPI support
    int drawableWidth, drawableHeight;
    SDL_GL_GetDrawableSize(mWindow, &drawableWidth, &drawableHeight);
    mMetalLayer->setDrawableSize(CGSizeMake(drawableWidth, drawableHeight));
    
   
    // Platform-specific code to attach Metal layer to window
    #if TARGET_OS_OSX
    // Get the NSWindow from SDL
    void* nsWindow = wmInfo.info.cocoa.window;

    // Use our bridge function to set the Metal layer
    SetMetalLayerForSDLWindow(nsWindow, metalLayerPtr);
    #else
    // iOS or other platform - would need different implementation
    std::cerr << "Unsupported platform" << std::endl;
    return false;
    #endif
    
    // Create uniform buffers
    mVertexUniformBuffer = mDevice->newBuffer(sizeof(VertexUniforms), 
                                             MTL::ResourceStorageModeShared);
    mFragmentUniformBuffer = mDevice->newBuffer(sizeof(FragmentUniforms), 
                                              MTL::ResourceStorageModeShared);
    
    // Create depth texture
    MTL::TextureDescriptor* depthTextureDesc = MTL::TextureDescriptor::texture2DDescriptor(
        MTL::PixelFormatDepth32Float,
        drawableWidth,
        drawableHeight,
        false
    );
    depthTextureDesc->setStorageMode(MTL::StorageModePrivate);
    depthTextureDesc->setUsage(MTL::TextureUsageRenderTarget);
    mDepthTexture = mDevice->newTexture(depthTextureDesc);
    
    return true;
}

void Renderer3D_Metal::SetMetalLayerForWindow(void* nsWindowPtr, CA::MetalLayer* layer) {
    // We now use the external bridge function directly in InitializeMetal
    // This method is kept for backwards compatibility but is no longer needed
}

bool Renderer3D_Metal::LoadShaders() {
    // In a real application, you would compile the Metal shaders during the build process
    // For this example, we'll try to load from a pre-compiled metallib file
    NS::Error* error = nullptr;
    
    // First try to load from a bundled metallib file
    NS::String* path = GetMetalLibraryPath();
    mShaderLibrary = mDevice->newLibrary(path, &error);
    
    if (!mShaderLibrary) {
        // If that fails, try to compile from source string
        // For simplicity, let's include a basic shader in the C++ code
        const char* shaderSource = R"(
            #include <metal_stdlib>
            using namespace metal;
            
            struct VertexIn {
                float3 position [[attribute(0)]];
                float3 normal [[attribute(1)]];
            };
            
            struct VertexOut {
                float4 position [[position]];
                float3 fragmentPosition;
                float3 normal;
            };
            
            struct VertexUniforms {
                float4x4 modelMatrix;
                float4x4 viewMatrix;
                float4x4 projectionMatrix;
            };
            
            struct FragmentUniforms {
                float3 lightPosition;
                float3 ambientLight;
                float3 cameraPosition;
            };
            
            vertex VertexOut vertex_main(const VertexIn vertices [[stage_in]],
                                       constant VertexUniforms& uniforms [[buffer(1)]]) {
                VertexOut out;
                
                float4 worldPosition = uniforms.modelMatrix * float4(vertices.position, 1.0);
                out.fragmentPosition = worldPosition.xyz;
                out.position = uniforms.projectionMatrix * uniforms.viewMatrix * worldPosition;
                
                float3x3 normalMatrix = float3x3(uniforms.modelMatrix[0].xyz,
                                               uniforms.modelMatrix[1].xyz,
                                               uniforms.modelMatrix[2].xyz);
                out.normal = normalize(normalMatrix * vertices.normal);
                
                return out;
            }
            
            fragment float4 fragment_main(VertexOut in [[stage_in]],
                                        constant FragmentUniforms& uniforms [[buffer(0)]]) {
                float3 norm = normalize(in.normal);
                float3 lightDir = normalize(uniforms.lightPosition - in.fragmentPosition);
                
                float3 ambient = uniforms.ambientLight;
                
                float diff = max(dot(norm, lightDir), 0.0);
                float3 diffuse = diff * float3(1.0, 1.0, 1.0);
                
                float3 objectColor = float3(1.0, 0.0, 0.0);
                
                float3 result = (ambient + diffuse) * objectColor;
                
                return float4(result, 1.0);
            }
        )";
        
        NS::String* source = NS::String::string(shaderSource, NS::UTF8StringEncoding);
        MTL::CompileOptions* options = MTL::CompileOptions::alloc()->init();
        mShaderLibrary = mDevice->newLibrary(source, options, &error);
        options->release();
    }
    
    if (!mShaderLibrary) {
        if (error) {
            std::cerr << "Failed to load Metal shaders: " 
                     << error->localizedDescription()->utf8String() << std::endl;
        } else {
            std::cerr << "Failed to load Metal shaders: unknown error" << std::endl;
        }
        return false;
    }
    
    return true;
}

bool Renderer3D_Metal::CreateRenderPipeline() {
    // Create render pipeline state
    MTL::RenderPipelineDescriptor* pipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    
    // Get shader functions
    MTL::Function* vertexFunction = mShaderLibrary->newFunction(
        NS::String::string("vertex_main", NS::UTF8StringEncoding));
    MTL::Function* fragmentFunction = mShaderLibrary->newFunction(
        NS::String::string("fragment_main", NS::UTF8StringEncoding));
    
    if (!vertexFunction || !fragmentFunction) {
        std::cerr << "Failed to find shader functions in library" << std::endl;
        return false;
    }
    
    pipelineDescriptor->setVertexFunction(vertexFunction);
    pipelineDescriptor->setFragmentFunction(fragmentFunction);
    
    // Set up color attachment
    pipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    pipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    
    // Set up vertex descriptor to match our Vertex struct
    MTL::VertexDescriptor* vertexDescriptor = MTL::VertexDescriptor::alloc()->init();

    // Position attribute
    vertexDescriptor->attributes()->object(0)->setFormat(MTL::VertexFormatFloat3);
    vertexDescriptor->attributes()->object(0)->setOffset(0);
    vertexDescriptor->attributes()->object(0)->setBufferIndex(0);

    // Normal attribute
    vertexDescriptor->attributes()->object(1)->setFormat(MTL::VertexFormatFloat3);
    vertexDescriptor->attributes()->object(1)->setOffset(sizeof(float) * 3);
    vertexDescriptor->attributes()->object(1)->setBufferIndex(0);

    // isLandingPad attribute
    vertexDescriptor->attributes()->object(2)->setFormat(MTL::VertexFormatFloat);
    vertexDescriptor->attributes()->object(2)->setOffset(sizeof(float) * 6);
    vertexDescriptor->attributes()->object(2)->setBufferIndex(0);

    // entityType attribute
    vertexDescriptor->attributes()->object(3)->setFormat(MTL::VertexFormatFloat);
    vertexDescriptor->attributes()->object(3)->setOffset(sizeof(float) * 7);
    vertexDescriptor->attributes()->object(3)->setBufferIndex(0);

    // Set layout
    vertexDescriptor->layouts()->object(0)->setStride(sizeof(Vertex));
    
    pipelineDescriptor->setVertexDescriptor(vertexDescriptor);
    
    // Create render pipeline state
    NS::Error* error = nullptr;
    mRenderPipelineState = mDevice->newRenderPipelineState(pipelineDescriptor, &error);
    
    // Clean up
    vertexDescriptor->release();
    pipelineDescriptor->release();
    vertexFunction->release();
    fragmentFunction->release();
    
    if (!mRenderPipelineState) {
        if (error) {
            std::cerr << "Failed to create render pipeline state: " 
                     << error->localizedDescription()->utf8String() << std::endl;
        } else {
            std::cerr << "Failed to create render pipeline state: unknown error" << std::endl;
        }
        return false;
    }
    
    // Create depth stencil state
    MTL::DepthStencilDescriptor* depthDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthDescriptor->setDepthCompareFunction(MTL::CompareFunctionLess);
    depthDescriptor->setDepthWriteEnabled(true);
    
    mDepthStencilState = mDevice->newDepthStencilState(depthDescriptor);
    depthDescriptor->release();
    
    return true;
}

bool Renderer3D_Metal::CreateGeometryBuffers() {
    // Create a simple cube model for the lander
    CreateCubeModel();
    
    // Terrain buffers will be created dynamically when rendering
    
    return true;
}

void Renderer3D_Metal::CreateCubeModel() {
    
    // Cube vertices (position + normal + isLandingPad + entityType)
Vertex cubeVertices[] = {
    // Front face
    {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, 0.0f, 1.0f},
    {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, 0.0f, 1.0f},
    {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, 0.0f, 1.0f},
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, 0.0f, 1.0f},
    
    // Back face
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, 0.0f, 1.0f},
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, 0.0f, 1.0f},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, 0.0f, 1.0f},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, 0.0f, 1.0f},
    
    // Top face
    {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, 0.0f, 1.0f},
    {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0.0f, 1.0f},
    {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0.0f, 1.0f},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, 0.0f, 1.0f},
    
    // Bottom face
    {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, 0.0f, 1.0f},
    {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, 0.0f, 1.0f},
    {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, 0.0f, 1.0f},
    {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, 0.0f, 1.0f},
    
    // Right face
    {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, 0.0f, 1.0f},
    {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, 0.0f, 1.0f},
    {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, 0.0f, 1.0f},
    {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, 0.0f, 1.0f},
    
    // Left face
    {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, 0.0f, 1.0f},
    {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, 0.0f, 1.0f},
    {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, 0.0f, 1.0f},
    {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, 0.0f, 1.0f}
    };
    
    // Cube indices
    uint16_t cubeIndices[] = {
        0, 1, 2, 2, 3, 0,       // Front face
        4, 5, 6, 6, 7, 4,       // Back face
        8, 9, 10, 10, 11, 8,    // Top face
        12, 13, 14, 14, 15, 12, // Bottom face
        16, 17, 18, 18, 19, 16, // Right face
        20, 21, 22, 22, 23, 20  // Left face
    };
    
    // Create vertex buffer
    mLanderVertexBuffer = mDevice->newBuffer(
        cubeVertices,
        sizeof(cubeVertices),
        MTL::ResourceStorageModeShared
    );
    
    // Create index buffer
    mLanderIndexBuffer = mDevice->newBuffer(
        cubeIndices,
        sizeof(cubeIndices),
        MTL::ResourceStorageModeShared
    );
    
    // Store counts
    mLanderVertexCount = sizeof(cubeVertices) / sizeof(Vertex);
    mLanderIndexCount = sizeof(cubeIndices) / sizeof(uint16_t);
    
    std::cout << "Created cube model with " << mLanderVertexCount << " vertices and " 
              << mLanderIndexCount << " indices" << std::endl;
}

void Renderer3D_Metal::Shutdown() {
    // Release Metal objects in reverse order of creation
    
    // Release buffers
    if (mLanderVertexBuffer) { mLanderVertexBuffer->release(); mLanderVertexBuffer = nullptr; }
    if (mLanderIndexBuffer) { mLanderIndexBuffer->release(); mLanderIndexBuffer = nullptr; }
    if (mTerrainVertexBuffer) { mTerrainVertexBuffer->release(); mTerrainVertexBuffer = nullptr; }
    if (mTerrainIndexBuffer) { mTerrainIndexBuffer->release(); mTerrainIndexBuffer = nullptr; }
    if (mVertexUniformBuffer) { mVertexUniformBuffer->release(); mVertexUniformBuffer = nullptr; }
    if (mFragmentUniformBuffer) { mFragmentUniformBuffer->release(); mFragmentUniformBuffer = nullptr; }
    
    // Release textures
    if (mDepthTexture) { mDepthTexture->release(); mDepthTexture = nullptr; }
    
    // Release pipeline states
    if (mRenderPipelineState) { mRenderPipelineState->release(); mRenderPipelineState = nullptr; }
    if (mDepthStencilState) { mDepthStencilState->release(); mDepthStencilState = nullptr; }
    
    // Release shader library
    if (mShaderLibrary) { mShaderLibrary->release(); mShaderLibrary = nullptr; }
    
    // Release command queue
    if (mCommandQueue) { mCommandQueue->release(); mCommandQueue = nullptr; }
    
    // Release Metal layer using the bridge
    if (mMetalLayer) {
    // We don't call release on mMetalLayer directly because it's managed by Objective-C
    ReleaseCAMetalLayer(reinterpret_cast<void*>(mMetalLayer));
    mMetalLayer = nullptr;
    }
    
    // Release Metal device
    if (mDevice) { mDevice->release(); mDevice = nullptr; }
    
    // Destroy SDL window
    if (mWindow) {
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
    }
    
    // Shut down SDL
    SDL_Quit();
    
    mInitialized = false;
}

void Renderer3D_Metal::Clear() {
    // Metal doesn't have a separate clear function,
    // instead the clear color is set in the render pass descriptor
    // which is created in the Present method
}

void Renderer3D_Metal::Present() {
    if (!mInitialized) return;
    
    // Get the next drawable
    CA::MetalDrawable* drawable = mMetalLayer->nextDrawable();
    if (!drawable) return;
    
    // Create a render pass descriptor
    MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
    
    // Configure color attachment
    renderPassDescriptor->colorAttachments()->object(0)->setTexture(drawable->texture());
    renderPassDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
    renderPassDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor(0.2, 0.4, 0.6, 1.0));
    renderPassDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
    
    // Configure depth attachment
    renderPassDescriptor->setDepthAttachment(MTL::RenderPassDepthAttachmentDescriptor::alloc()->init());
    renderPassDescriptor->depthAttachment()->setTexture(mDepthTexture);
    renderPassDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionClear);
    renderPassDescriptor->depthAttachment()->setClearDepth(1.0);
    renderPassDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
    
    // Create command buffer
    MTL::CommandBuffer* commandBuffer = mCommandQueue->commandBuffer();
    
    // Create render command encoder
    MTL::RenderCommandEncoder* renderEncoder = commandBuffer->renderCommandEncoder(renderPassDescriptor);
    
    // Set render pipeline state
    renderEncoder->setRenderPipelineState(mRenderPipelineState);
    
    // Set depth stencil state
    renderEncoder->setDepthStencilState(mDepthStencilState);
    
    // Set fragment uniforms
    renderEncoder->setFragmentBuffer(mFragmentUniformBuffer, 0, 0);
    
    // End encoding
    renderEncoder->endEncoding();
    
    // Present drawable
    commandBuffer->presentDrawable(drawable);
    
    // Commit command buffer
    commandBuffer->commit();
    
    // Clean up
    renderPassDescriptor->release();
}

// Rest of the methods remain largely the same, just fixing Metal-cpp specific issues
// and ensuring proper reference counting

void Renderer3D_Metal::RenderLander(Lander* lander) {
    if (!mInitialized || !lander) return;
    
    // Get lander properties
    const float* position = lander->GetPosition();
    const float* rotation = lander->GetRotation();
    const float* scale = lander->GetScale();
    
    // Update model uniforms
    UpdateModelUniforms(position, rotation, scale);
    
    // Get the next drawable
    CA::MetalDrawable* drawable = mMetalLayer->nextDrawable();
    if (!drawable) return;
    
    // Create a render pass descriptor
    MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
    
    // Configure color attachment
    renderPassDescriptor->colorAttachments()->object(0)->setTexture(drawable->texture());
    renderPassDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
    renderPassDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor(0.2, 0.4, 0.6, 1.0));
    renderPassDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
    
    // Configure depth attachment
    renderPassDescriptor->setDepthAttachment(MTL::RenderPassDepthAttachmentDescriptor::alloc()->init());
    renderPassDescriptor->depthAttachment()->setTexture(mDepthTexture);
    renderPassDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionClear);
    renderPassDescriptor->depthAttachment()->setClearDepth(1.0);
    renderPassDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
    
    // Create command buffer
    MTL::CommandBuffer* commandBuffer = mCommandQueue->commandBuffer();
    
    // Create render command encoder
    MTL::RenderCommandEncoder* renderEncoder = commandBuffer->renderCommandEncoder(renderPassDescriptor);
    
    // Set render pipeline state
    renderEncoder->setRenderPipelineState(mRenderPipelineState);
    
    // Set depth stencil state
    renderEncoder->setDepthStencilState(mDepthStencilState);
    
    // Set vertex buffer
    renderEncoder->setVertexBuffer(mLanderVertexBuffer, 0, 0);
    
    // Set uniform buffers
    renderEncoder->setVertexBuffer(mVertexUniformBuffer, 0, 1);
    renderEncoder->setFragmentBuffer(mFragmentUniformBuffer, 0, 0);
    
    // Draw indexed primitives
    renderEncoder->drawIndexedPrimitives(
        MTL::PrimitiveTypeTriangle,
        mLanderIndexCount,
        MTL::IndexTypeUInt16,
        mLanderIndexBuffer,
        0
    );
    
    // End encoding
    renderEncoder->endEncoding();
    
    // Present drawable
    commandBuffer->presentDrawable(drawable);
    
    // Commit command buffer
    commandBuffer->commit();
    
    // Clean up
    renderPassDescriptor->release();
}

// Update camera uniform buffers
void Renderer3D_Metal::UpdateCameraUniforms() {
    // Update vertex uniforms
    VertexUniforms* vertUniforms = static_cast<VertexUniforms*>(mVertexUniformBuffer->contents());
    memcpy(vertUniforms->viewMatrix, mViewMatrix.values, sizeof(mViewMatrix.values));
    memcpy(vertUniforms->projectionMatrix, mProjectionMatrix.values, sizeof(mProjectionMatrix.values));
    
    // Update fragment uniforms
    FragmentUniforms* fragUniforms = static_cast<FragmentUniforms*>(mFragmentUniformBuffer->contents());
    memcpy(fragUniforms->lightPosition, mLightPosition, sizeof(mLightPosition));
    memcpy(fragUniforms->ambientLight, mAmbientLight, sizeof(mAmbientLight));
    memcpy(fragUniforms->cameraPosition, mCameraPosition, sizeof(mCameraPosition));
}

// Update model uniform buffer
void Renderer3D_Metal::UpdateModelUniforms(const float* position, const float* rotation, const float* scale) {
    // Create model matrix
    mModelMatrix = CreateModelMatrix(position, rotation, scale);
    
    // Update vertex uniforms
    VertexUniforms* vertUniforms = static_cast<VertexUniforms*>(mVertexUniformBuffer->contents());
    memcpy(vertUniforms->modelMatrix, mModelMatrix.values, sizeof(mModelMatrix.values));
}

// Additional methods omitted for brevity - they remain largely the same
// For a full implementation, please copy over the remaining methods

// Implementations for the remaining public interface methods
void Renderer3D_Metal::RenderTerrain(Terrain* terrain) {
    if (!mInitialized || !terrain) return;
    
    // Get terrain triangles
    const std::vector<TerrainTriangle>& triangles = terrain->GetTriangles3D();
    if (triangles.empty()) {
        std::cout << "No terrain triangles to render" << std::endl;
        return;
    }
    
    // Create or update terrain buffers
    if (!mTerrainVertexBuffer || !mTerrainIndexBuffer) {
        // Count number of vertices and prepare data
        int vertexCount = triangles.size() * 3; // 3 vertices per triangle
        std::vector<Vertex> vertices(vertexCount);
        std::vector<uint16_t> indices(vertexCount);
        
    // Populate vertices and indices
    for (size_t i = 0; i < triangles.size(); i++) {
    const TerrainTriangle& tri = triangles[i];
    
    // Add three vertices for each triangle
    for (int j = 0; j < 3; j++) {
        int vIdx = i * 3 + j;
        
        // Position
        vertices[vIdx].position[0] = tri.vertices[j * 3];     // x
        vertices[vIdx].position[1] = tri.vertices[j * 3 + 1]; // y
        vertices[vIdx].position[2] = tri.vertices[j * 3 + 2]; // z
        
        // Normal
        vertices[vIdx].normal[0] = tri.normal[0];
        vertices[vIdx].normal[1] = tri.normal[1];
        vertices[vIdx].normal[2] = tri.normal[2];
        
        // Set isLandingPad and entityType
        vertices[vIdx].isLandingPad = tri.isLandingPad ? 1.0f : 0.0f;
        vertices[vIdx].entityType = 0.0f; // 0.0 for terrain
        
        // Simple indexing
        indices[vIdx] = vIdx;
        }
    }
        
        // Create vertex buffer
        mTerrainVertexBuffer = mDevice->newBuffer(
            vertices.data(),
            vertices.size() * sizeof(Vertex),
            MTL::ResourceStorageModeShared
        );
        
        // Create index buffer
        mTerrainIndexBuffer = mDevice->newBuffer(
            indices.data(),
            indices.size() * sizeof(uint16_t),
            MTL::ResourceStorageModeShared
        );
        
        // Store count for drawing
        mTerrainIndexCount = indices.size();
        
        std::cout << "Created terrain buffers with " << vertexCount << " vertices" << std::endl;
    }
    
    // Get the next drawable
    CA::MetalDrawable* drawable = mMetalLayer->nextDrawable();
    if (!drawable) return;
    
    // Create a render pass descriptor
    MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
    
    // Configure color attachment
    renderPassDescriptor->colorAttachments()->object(0)->setTexture(drawable->texture());
    renderPassDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
    renderPassDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor(0.0, 0.0, 0.1, 1.0)); // Dark blue/black sky
    renderPassDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
    
    // Configure depth attachment
    renderPassDescriptor->setDepthAttachment(MTL::RenderPassDepthAttachmentDescriptor::alloc()->init());
    renderPassDescriptor->depthAttachment()->setTexture(mDepthTexture);
    renderPassDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionClear);
    renderPassDescriptor->depthAttachment()->setClearDepth(1.0);
    renderPassDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
    
    // Create command buffer
    MTL::CommandBuffer* commandBuffer = mCommandQueue->commandBuffer();
    
    // Create render command encoder
    MTL::RenderCommandEncoder* renderEncoder = commandBuffer->renderCommandEncoder(renderPassDescriptor);
    
    // Set render pipeline state
    renderEncoder->setRenderPipelineState(mRenderPipelineState);
    
    // Set depth stencil state
    renderEncoder->setDepthStencilState(mDepthStencilState);
    
    // Set vertex buffer
    renderEncoder->setVertexBuffer(mTerrainVertexBuffer, 0, 0);
    
    // Create model matrix for terrain
    float terrainPosition[3] = {0.0f, 0.0f, 0.0f}; // Center terrain at origin
    float terrainRotation[3] = {0.0f, 0.0f, 0.0f}; // No rotation
    float terrainScale[3] = {1.0f, 1.0f, 1.0f};    // Default scale
    
    // Update model matrix
    mModelMatrix = CreateModelMatrix(terrainPosition, terrainRotation, terrainScale);
    
    // Update vertex uniforms
    VertexUniforms* vertUniforms = static_cast<VertexUniforms*>(mVertexUniformBuffer->contents());
    memcpy(vertUniforms->modelMatrix, mModelMatrix.values, sizeof(mModelMatrix.values));
    
    // Set uniform buffers
    renderEncoder->setVertexBuffer(mVertexUniformBuffer, 0, 1);
    renderEncoder->setFragmentBuffer(mFragmentUniformBuffer, 0, 0);
    
    // Draw indexed primitives
    renderEncoder->drawIndexedPrimitives(
        MTL::PrimitiveTypeTriangle,
        mTerrainIndexCount,
        MTL::IndexTypeUInt16,
        mTerrainIndexBuffer,
        0
    );
    
    // End encoding
    renderEncoder->endEncoding();
    
    // Present drawable
    commandBuffer->presentDrawable(drawable);
    
    // Commit command buffer
    commandBuffer->commit();
    
    // Clean up
    renderPassDescriptor->release();
}

void Renderer3D_Metal::RenderTelemetry(Game* game) {
    // Implementation omitted for brevity
    // This would render 2D telemetry information
}

void Renderer3D_Metal::RenderGameState(Game* game) {
    // Implementation omitted for brevity
    // This would render 2D game state information
}

void Renderer3D_Metal::SetCameraPosition(float x, float y, float z) {
    mCameraPosition[0] = x;
    mCameraPosition[1] = y;
    mCameraPosition[2] = z;
    
    // Update view matrix
    mViewMatrix = CreateViewMatrix();
    
    // Update camera uniforms
    UpdateCameraUniforms();
}

void Renderer3D_Metal::SetCameraTarget(float x, float y, float z) {
    mCameraTarget[0] = x;
    mCameraTarget[1] = y;
    mCameraTarget[2] = z;
    
    // Update view matrix
    mViewMatrix = CreateViewMatrix();
    
    // Update camera uniforms
    UpdateCameraUniforms();
}

void Renderer3D_Metal::SetCameraUp(float x, float y, float z) {
    mCameraUp[0] = x;
    mCameraUp[1] = y;
    mCameraUp[2] = z;
    
    // Update view matrix
    mViewMatrix = CreateViewMatrix();
    
    // Update camera uniforms
    UpdateCameraUniforms();
}

void Renderer3D_Metal::SetLightPosition(float x, float y, float z) {
    mLightPosition[0] = x;
    mLightPosition[1] = y;
    mLightPosition[2] = z;
    
    // Update fragment uniforms
    FragmentUniforms* fragUniforms = static_cast<FragmentUniforms*>(mFragmentUniformBuffer->contents());
    memcpy(fragUniforms->lightPosition, mLightPosition, sizeof(mLightPosition));
}

void Renderer3D_Metal::SetAmbientLight(float r, float g, float b) {
    mAmbientLight[0] = r;
    mAmbientLight[1] = g;
    mAmbientLight[2] = b;
    
    // Update fragment uniforms
    FragmentUniforms* fragUniforms = static_cast<FragmentUniforms*>(mFragmentUniformBuffer->contents());
    memcpy(fragUniforms->ambientLight, mAmbientLight, sizeof(mAmbientLight));
}

// Matrix math methods would remain the same

// Create a projection matrix
Matrix4x4 Renderer3D_Metal::CreateProjectionMatrix(float fov, float aspect, float near, float far) {
    Matrix4x4 result;
    
    // Metal uses a different coordinate system than OpenGL
    // In Metal, clip space is [-1, 1] for x and y, and [0, 1] for z
    const float f = 1.0f / tanf(fov / 2.0f);
    
    result.values[0] = f / aspect;
    result.values[1] = 0.0f;
    result.values[2] = 0.0f;
    result.values[3] = 0.0f;
    
    result.values[4] = 0.0f;
    result.values[5] = f;
    result.values[6] = 0.0f;
    result.values[7] = 0.0f;
    
    result.values[8] = 0.0f;
    result.values[9] = 0.0f;
    result.values[10] = (far) / (near - far);
    result.values[11] = -1.0f;
    
    result.values[12] = 0.0f;
    result.values[13] = 0.0f;
    result.values[14] = (near * far) / (near - far);
    result.values[15] = 0.0f;
    
    return result;
}

// Create a view matrix
Matrix4x4 Renderer3D_Metal::CreateViewMatrix() {
    // Calculate view direction vector
    float dir[3];
    dir[0] = mCameraTarget[0] - mCameraPosition[0];
    dir[1] = mCameraTarget[1] - mCameraPosition[1];
    dir[2] = mCameraTarget[2] - mCameraPosition[2];
    
    // Normalize view direction
    float len = sqrtf(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
    dir[0] /= len;
    dir[1] /= len;
    dir[2] /= len;
    
    // Calculate right vector
    float right[3];
    right[0] = dir[1] * mCameraUp[2] - dir[2] * mCameraUp[1];
    right[1] = dir[2] * mCameraUp[0] - dir[0] * mCameraUp[2];
    right[2] = dir[0] * mCameraUp[1] - dir[1] * mCameraUp[0];
    
    // Normalize right vector
    len = sqrtf(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
    right[0] /= len;
    right[1] /= len;
    right[2] /= len;
    
    // Calculate normalized up vector
    float up[3];
    up[0] = right[1] * dir[2] - right[2] * dir[1];
    up[1] = right[2] * dir[0] - right[0] * dir[2];
    up[2] = right[0] * dir[1] - right[1] * dir[0];
    
    // Create view matrix
    Matrix4x4 result;
    
    result.values[0] = right[0];
    result.values[1] = up[0];
    result.values[2] = -dir[0];
    result.values[3] = 0.0f;
    
    result.values[4] = right[1];
    result.values[5] = up[1];
    result.values[6] = -dir[1];
    result.values[7] = 0.0f;
    
    result.values[8] = right[2];
    result.values[9] = up[2];
    result.values[10] = -dir[2];
    result.values[11] = 0.0f;
    
    result.values[12] = -(right[0] * mCameraPosition[0] + right[1] * mCameraPosition[1] + right[2] * mCameraPosition[2]);
    result.values[13] = -(up[0] * mCameraPosition[0] + up[1] * mCameraPosition[1] + up[2] * mCameraPosition[2]);
    result.values[14] = dir[0] * mCameraPosition[0] + dir[1] * mCameraPosition[1] + dir[2] * mCameraPosition[2];
    result.values[15] = 1.0f;
    
    return result;
}

// Create a model matrix
Matrix4x4 Renderer3D_Metal::CreateModelMatrix(const float* position, const float* rotation, const float* scale) {
    // Create rotation matrices
    float radX = rotation[0] * (M_PI / 180.0f);
    float radY = rotation[1] * (M_PI / 180.0f);
    float radZ = rotation[2] * (M_PI / 180.0f);
    
    float sinX = sinf(radX);
    float cosX = cosf(radX);
    float sinY = sinf(radY);
    float cosY = cosf(radY);
    float sinZ = sinf(radZ);
    float cosZ = cosf(radZ);
    
    // Create model matrix
    Matrix4x4 result;
    
    // Scale
    result.values[0] = scale[0];
    result.values[1] = 0.0f;
    result.values[2] = 0.0f;
    result.values[3] = 0.0f;
    
    result.values[4] = 0.0f;
    result.values[5] = scale[1];
    result.values[6] = 0.0f;
    result.values[7] = 0.0f;
    
    result.values[8] = 0.0f;
    result.values[9] = 0.0f;
    result.values[10] = scale[2];
    result.values[11] = 0.0f;
    
    result.values[12] = 0.0f;
    result.values[13] = 0.0f;
    result.values[14] = 0.0f;
    result.values[15] = 1.0f;
    
    // Apply rotation matrices (in order: Y, X, Z)
    Matrix4x4 rotationMatrix;
    
    // Rotation around Y axis
    rotationMatrix.values[0] = cosY;
    rotationMatrix.values[1] = 0.0f;
    rotationMatrix.values[2] = -sinY;
    rotationMatrix.values[3] = 0.0f;
    
    rotationMatrix.values[4] = 0.0f;
    rotationMatrix.values[5] = 1.0f;
    rotationMatrix.values[6] = 0.0f;
    rotationMatrix.values[7] = 0.0f;
    
    rotationMatrix.values[8] = sinY;
    rotationMatrix.values[9] = 0.0f;
    rotationMatrix.values[10] = cosY;
    rotationMatrix.values[11] = 0.0f;
    
    rotationMatrix.values[12] = 0.0f;
    rotationMatrix.values[13] = 0.0f;
    rotationMatrix.values[14] = 0.0f;
    rotationMatrix.values[15] = 1.0f;
    
    Matrix4x4 temp = result;
    MultiplyMatrices(result, rotationMatrix, temp);
    
    // Rotation around X axis
    rotationMatrix.values[0] = 1.0f;
    rotationMatrix.values[1] = 0.0f;
    rotationMatrix.values[2] = 0.0f;
    rotationMatrix.values[3] = 0.0f;
    
    rotationMatrix.values[4] = 0.0f;
    rotationMatrix.values[5] = cosX;
    rotationMatrix.values[6] = sinX;
    rotationMatrix.values[7] = 0.0f;
    
    rotationMatrix.values[8] = 0.0f;
    rotationMatrix.values[9] = -sinX;
    rotationMatrix.values[10] = cosX;
    rotationMatrix.values[11] = 0.0f;
    
    rotationMatrix.values[12] = 0.0f;
    rotationMatrix.values[13] = 0.0f;
    rotationMatrix.values[14] = 0.0f;
    rotationMatrix.values[15] = 1.0f;
    
    temp = result;
    MultiplyMatrices(result, rotationMatrix, temp);
    
    // Rotation around Z axis
    rotationMatrix.values[0] = cosZ;
    rotationMatrix.values[1] = sinZ;
    rotationMatrix.values[2] = 0.0f;
    rotationMatrix.values[3] = 0.0f;
    
    rotationMatrix.values[4] = -sinZ;
    rotationMatrix.values[5] = cosZ;
    rotationMatrix.values[6] = 0.0f;
    rotationMatrix.values[7] = 0.0f;
    
    rotationMatrix.values[8] = 0.0f;
    rotationMatrix.values[9] = 0.0f;
    rotationMatrix.values[10] = 1.0f;
    rotationMatrix.values[11] = 0.0f;
    
    rotationMatrix.values[12] = 0.0f;
    rotationMatrix.values[13] = 0.0f;
    rotationMatrix.values[14] = 0.0f;
    rotationMatrix.values[15] = 1.0f;
    
    temp = result;
    MultiplyMatrices(result, rotationMatrix, temp);
    
    // Apply translation
    result.values[12] = position[0];
    result.values[13] = position[1];
    result.values[14] = position[2];
    
    return result;
}

// Multiply two matrices
void Renderer3D_Metal::MultiplyMatrices(Matrix4x4& result, const Matrix4x4& a, const Matrix4x4& b) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.values[i * 4 + j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                result.values[i * 4 + j] += a.values[i * 4 + k] * b.values[k * 4 + j];
            }
        }
    }
}