layout(std140, binding = 0) uniform cameraBuffer
{
    mat4 viewProj;
};


layout(binding = 10) uniform sampler2D fieldTex;

in vec2 texCoords;
out vec4 finalColor;

void main()
{
    // Transform screen coordinates back to world space
    vec4 clipSpacePos = vec4(texCoords * 2.0 - 1.0, 0.0, 1.0);
    vec4 worldPos = inverse(viewProj) * clipSpacePos;  
    worldPos /= worldPos.w;

    // Scale by texture and texture resolution and scaling factor
    vec2 sampleCoords = worldPos.xy / (128 * 3);

    // Final sample
    vec4 color = texture(fieldTex, sampleCoords);
    finalColor = color;
}