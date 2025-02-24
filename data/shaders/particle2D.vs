layout(std140, binding = 0) uniform cameraBuffer
{
    mat4 viewProj;
};

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in float aSize;

out vec4 ParticleColor;

void main()
{
    vec2 worldPos = aPos;
    gl_Position = viewProj * vec4(worldPos, 0.0, 1.0);
    gl_PointSize = aSize;
    ParticleColor = aColor;
}
