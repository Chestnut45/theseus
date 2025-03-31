layout(std140, binding = 0) uniform cameraBuffer
{
    mat4 viewProj;
};

struct ParticleData
{
    vec4 posVel;
    vec4 forceDensityPressure;
};

layout(std430, binding = 3) buffer ParticleSSBO
{
    ParticleData particles[];
};

layout(location = 0) in vec2 quadPos;

uniform float kernelRadius;

// NOTE: Be wary of interpolator limits!
out vec2 pos;
out vec2 worldPos;
out flat vec2 vel;
out flat vec4 forceDensityPressure;

void main()
{
    // Grab particle data
    ParticleData p = particles[gl_InstanceID];

    // Scale input quad verts
    vec2 scaledQuadPos = (quadPos - vec2(0.5)) * kernelRadius;
    
    // Set varying outputs
    pos = scaledQuadPos;
    worldPos = scaledQuadPos + p.posVel.xy;
    vel = p.posVel.zw;
    forceDensityPressure = p.forceDensityPressure;

    // Output position
    gl_Position = viewProj * vec4((scaledQuadPos + p.posVel.xy), 0.0, 1.0);
}