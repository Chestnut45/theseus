// Vertex shader outputs
in vec2 texCoords;

// Final color output
out vec3 color;

// Sprite texture sampler at slot 0
layout(binding = 0) uniform sampler2D spriteTexture;

void main()
{
    float tintLimit = 0.25 + (sin(texCoords.x * 24) * 0.0075);
    float gradientRate = 28.0;

    // Sample sprite texture
    vec4 textureColor = texture(spriteTexture, texCoords);

    // Discard transparent pixels
    if (textureColor.a == 0.0) discard;
    
    float y_tint = texCoords.y <= tintLimit ? 1.0 + (tintLimit - texCoords.y) * gradientRate : 1.0;

    vec3 tempcolor = vec3(
        textureColor.r * 1.2, 
        textureColor.g * 0.4, 
        textureColor.b * 0.04
        ) * y_tint;

    color = tempcolor;
}