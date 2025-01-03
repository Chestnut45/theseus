// Vertex shader outputs
in vec2 texCoords;

// Final color output
out vec3 color;

// Sprite texture sampler at slot 0
layout(binding = 0) uniform sampler2D spriteTexture;

// Tint
uniform vec3 tint;

void main()
{
    // Sample sprite texture
    vec4 textureColor = texture(spriteTexture, texCoords);

    // Discard transparent pixels
    if (textureColor.a == 0.0) discard;

    // white pixels
    color = vec3(1.0, 1.0, 1.0);
}