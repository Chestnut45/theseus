// Camera UBO
layout(std140, binding = 0) uniform cameraBuffer
{
    mat4 viewProj;
};

struct ParticleData
{
    vec4 posVel;
    vec4 forceDensityPressure;
};

// Particle data SSBO
layout(std430, binding = 3) buffer ParticleSSBO
{
    ParticleData particles[];
};

// Quad attributes
layout(location = 0) in vec2 quadPos;

// Model matrix for the Transform component
// uniform mat4 model;

uniform float kernelRadius;

// Outputs to fragment shader
out vec2 pos;

void main()
{
    vec2 scaledQuadPos = (quadPos - vec2(0.5)) * kernelRadius;
    pos = scaledQuadPos;
    gl_Position = viewProj * vec4((scaledQuadPos + particles[gl_InstanceID].posVel.xy), 0.0, 1.0);
}