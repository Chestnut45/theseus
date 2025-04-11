// Vertex shader outputs
in vec2 texCoords;

// Final color output
out vec3 color;

// Sprite texture sampler at slot 0
layout(binding = 0) uniform sampler2D spriteTexture;

uniform float fireGradientRate;
uniform float fireSineAmplitude;
uniform float fireSineFrequency;
uniform float fireSineMidline;
uniform float time;

uniform vec3 burnRGB;

void main()
{
    float tintLimit = fireSineMidline + sin(sin(texCoords.x * fireSineFrequency) + time) * fireSineAmplitude;

    // Sample sprite texture
    vec4 textureColor = texture(spriteTexture, texCoords);

    // Discard transparent pixels
    if (textureColor.a == 0.0) discard;
    
    float y_tint = texCoords.y <= tintLimit ? 1.0 + (tintLimit - texCoords.y) * fireGradientRate : 1.0;

    vec3 tempcolor = vec3(
        textureColor.r * burnRGB.r, 
        textureColor.g * burnRGB.g, 
        textureColor.b * burnRGB.b
        ) * y_tint;

    color = tempcolor;
}