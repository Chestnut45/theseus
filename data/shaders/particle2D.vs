layout(std140, binding = 0) uniform cameraBuffer
{
    mat4 viewProj;
};

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in float aSize;
layout (location = 3) in vec2 aTexCoord;

out vec4 ParticleColor;
out vec2 TexCoord;

void main()
{
    gl_Position = viewProj * vec4(aPos, 0.0, 1.0);
    gl_PointSize = aSize;
    ParticleColor = aColor;
    TexCoord = aTexCoord;
}