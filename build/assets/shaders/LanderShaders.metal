#include <metal_stdlib>
using namespace metal;

// Vertex input structure - must match the C++ Vertex struct
struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
};

// Vertex output structure
struct VertexOut {
    float4 position [[position]];
    float3 fragmentPosition;
    float3 normal;
};

// Uniform structures - must match the C++ structs
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

// Vertex shader function
vertex VertexOut vertex_main(const VertexIn vertices [[stage_in]],
                             constant VertexUniforms& uniforms [[buffer(1)]]) {
    VertexOut out;
    
    // Calculate position
    float4 worldPosition = uniforms.modelMatrix * float4(vertices.position, 1.0);
    out.fragmentPosition = worldPosition.xyz;
    out.position = uniforms.projectionMatrix * uniforms.viewMatrix * worldPosition;
    
    // Calculate normal in world space
    float3x3 normalMatrix = float3x3(uniforms.modelMatrix[0].xyz,
                                     uniforms.modelMatrix[1].xyz,
                                     uniforms.modelMatrix[2].xyz);
    out.normal = normalize(normalMatrix * vertices.normal);
    
    return out;
}

// Fragment shader function
fragment float4 fragment_main(VertexOut in [[stage_in]],
                              constant FragmentUniforms& uniforms [[buffer(0)]]) {
    // Normalize vectors
    float3 norm = normalize(in.normal);
    float3 lightDir = normalize(uniforms.lightPosition - in.fragmentPosition);
    
    // Ambient light
    float3 ambient = uniforms.ambientLight;
    
    // Diffuse light
    float diff = max(dot(norm, lightDir), 0.0);
    float3 diffuse = diff * float3(1.0, 1.0, 1.0);
    
    // Object color based on normal for testing
    float3 objectColor = float3(1.0, 0.0, 0.0); // Red color for lander
    
    // Combine lights
    float3 result = (ambient + diffuse) * objectColor;
    
    return float4(result, 1.0);
}