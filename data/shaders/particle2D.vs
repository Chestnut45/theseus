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

uniform mat4 model;
uniform int useTexture;

void main()
{
    ParticleColor = aColor;
    
    if (useTexture == 1) {
        // For textured quads, use the model matrix for positioning and scaling
        gl_Position = viewProj * model * vec4(aPos, 0.0, 1.0);
        TexCoord = aTexCoord;
    } else {
        // For point particles
        gl_Position = viewProj * vec4(aPos, 0.0, 1.0);
        gl_PointSize = aSize;
        TexCoord = vec2(0.5, 0.5); // Center point for potential point sprite texturing
    }
}