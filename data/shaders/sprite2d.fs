// Vertex shader inputs
in vec2 texCoords;

// Final color output
out vec3 finalColor;

// Sprite texture sampler at slot 0
layout(binding = 0) uniform sampler2D spriteTexture;

// Uniform sprite tint color
uniform vec3 spriteTint;

void main()
{
    vec3 textureColor = texture(spriteTexture, texCoords).rgb;
    finalColor = textureColor * spriteTint;
}