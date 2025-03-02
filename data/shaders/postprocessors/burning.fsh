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

    vec3 tempcolor = textureColor.rgb;
    tempcolor = vec3(tempcolor.r * 1.2 , tempcolor.g * 0.8, tempcolor.b * 0.8);
    tempcolor = vec3(
        tempcolor.r * y_tint, 
        tempcolor.g * y_tint * 0.5, 
        tempcolor.b * y_tint * 0.05
        );
    color = tempcolor;
}