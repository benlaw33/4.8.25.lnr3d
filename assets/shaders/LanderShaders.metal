#include <metal_stdlib>
using namespace metal;

// Vertex input structure - must match the C++ Vertex struct
struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float isLandingPad [[attribute(2)]];
    float entityType [[attribute(3)]];
};

// Vertex output structure
struct VertexOut {
    float4 position [[position]];
    float3 fragmentPosition;
    float3 normal;
    float isLandingPad;
    float entityType;
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
    
    // Pass through other attributes
    out.isLandingPad = vertices.isLandingPad;
    out.entityType = vertices.entityType;
    
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
    
    // Object color based on entity type and properties
    float3 objectColor;
    
    if (in.entityType > 0.5) {
        // Lander is red
        objectColor = float3(1.0, 0.0, 0.0);
    } else {
        // Terrain color: grey with some variation for lunar surface
        float heightFactor = fract(in.fragmentPosition.y * 0.5);
        float noiseFactor = fract(sin(dot(floor(in.fragmentPosition.xz * 10.0), float2(12.9898, 78.233))) * 43758.5453);
        
        if (in.isLandingPad > 0.5) {
            // Landing pad - bluish grey and flatter texture
            objectColor = float3(0.6, 0.6, 0.7) + noiseFactor * 0.05;
        } else {
            // Regular lunar surface - grey with more variation
            objectColor = float3(0.7, 0.7, 0.7) + noiseFactor * 0.15 + heightFactor * 0.1;
        }
    }
    
    // Combine lights
    float3 result = (ambient + diffuse) * objectColor;
    
    return float4(result, 1.0);
}