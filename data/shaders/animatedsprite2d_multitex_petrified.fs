// Vertex shader outputs
in vec2 texCoords;

// Final color output
out vec4 color;

// Sprite texture sampler at slot 0
layout(binding = 0) uniform sampler2D spriteTexture;
// Blending texture
layout(binding = 1) uniform sampler2D multiTex;

// Calculates fading in/out of texture
uniform float fadingTime;

// Tint
uniform vec3 tint;


vec4 mixColours(vec4 colour1, vec4 colour2, float weight)
{
    return colour1 * (1 - weight) + colour2 * weight;
}

void main()
{
    // Sample sprite texture
    vec4 textureColor = texture(spriteTexture, texCoords);
    // Discard transparent pixels
    if (textureColor.a == 0.0) discard;

    // Sample other texture
    vec4 multiTexColor = texture(multiTex, texCoords);
    multiTexColor.a = fadingTime;

    // Mix textures over time
    color = mixColours(textureColor, multiTexColor, fadingTime - 0.25);

    // Tinting
    color = color * (tint, 1.0);

    //Grayscaling for better effect
    vec3 gray = vec3(0.299 * color.r + 0.587 * color.g + 0.114 * color.b);
    color = mixColours(color, vec4(gray, 1.0), fadingTime);
}
