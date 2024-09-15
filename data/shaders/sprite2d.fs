// Vertex shader outputs
in vec2 texCoords;

// Final color output
out vec3 finalColor;

// Sprite texture sampler at slot 0
layout(binding = 0) uniform sampler2D spriteTexture;

// Uniform sprite tint color
uniform vec3 spriteTint;

void main()
{
    // Sample sprite texture
    vec4 textureColor = texture(spriteTexture, texCoords);

    // Discard transparent pixels
    if (textureColor.a == 0.0) discard;
    
    // Apply tint
    finalColor = textureColor.rgb * spriteTint;
}