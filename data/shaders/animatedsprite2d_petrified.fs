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
    
    vec3 tempcolor = textureColor.rgb * tint;

    // greyscale pixels
    color = vec3(0.299 * tempcolor.r + 0.587 * tempcolor.g + 0.114 * tempcolor.b);
}