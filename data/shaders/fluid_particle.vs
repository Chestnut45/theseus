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

// Model matrix for the object transform
uniform mat4 model;
uniform float kernelRadius;

// Outputs to fragment shader
out vec2 pos;
out flat vec2 vel;
out flat vec4 forceDensityPressure;

void main()
{
    vec2 scaledQuadPos = (quadPos - vec2(0.5)) * kernelRadius;
    ParticleData p = particles[gl_InstanceID];
    pos = scaledQuadPos;
    vel = p.posVel.zw;
    forceDensityPressure = p.forceDensityPressure;
    gl_Position = viewProj * model * vec4((scaledQuadPos + p.posVel.xy), 0.0, 1.0);
}