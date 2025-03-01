// Vertex shader outputs
in vec2 texCoords;

// Final color output
out vec3 color;

// Sprite texture sampler at slot 0
layout(binding = 0) uniform sampler2D spriteTexture;

void main()
{
    // Sample sprite texture
    vec4 textureColor = texture(spriteTexture, texCoords);

    // Discard transparent pixels
    if (textureColor.a == 0.0) discard;
    
    vec3 tempcolor = textureColor.rgb;

    // grayscale pixels
    color = vec3(tempcolor.r, 0.5 * tempcolor.g, 0.5* tempcolor.b);
}