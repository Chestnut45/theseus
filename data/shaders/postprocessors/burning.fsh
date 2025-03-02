// Vertex shader outputs
in vec2 texCoords;

// Final color output
out vec3 color;

// Sprite texture sampler at slot 0
layout(binding = 0) uniform sampler2D spriteTexture;

void main()
{
    float tintLimit = 0.25;

    // Sample sprite texture
    vec4 textureColor = texture(spriteTexture, texCoords);

    // Discard transparent pixels
    if (textureColor.a == 0.0) discard;
    
    float y_tint = texCoords.y <= tintLimit ? 1.0 + (tintLimit - texCoords.y) * 25.0 : 1.0;

    vec3 tempcolor = vec3(
        textureColor.r * 1.2, 
        textureColor.g * 0.4, 
        textureColor.b * 0.04
        ) * y_tint;

    color = tempcolor;
}