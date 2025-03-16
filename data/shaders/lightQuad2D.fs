// Vertex shader outputs
in vec2 texCoords;

// Final color output
out vec4 FragColor;

// Sprite texture sampler at slot 0
layout(binding = 0) uniform sampler2D screenTexture;

void main()
{
    // Sample sprite texture
    FragColor = texture(screenTexture, texCoords);
}