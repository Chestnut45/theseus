// Vertex shader outputs
in vec2 texCoords;

// Final color output
out vec3 color;

uniform float amplitude;
uniform float frequency;
uniform float time;


// Sprite texture sampler at slot 0
layout(binding = 0) uniform sampler2D spriteTexture;

void main()
{

    // Warp    
    vec2 warp = vec2(texCoords.x + amplitude * sin(texCoords.y * frequency + time) , texCoords.y);

    // Sample sprite texture
    vec4 textureColor = texture(spriteTexture, warp);

    // Discard transparent pixels
    if (textureColor.a == 0.0) discard;
    
    vec3 tempcolor = textureColor.rgb;

    // grayscale pixels
    color = vec3(tempcolor.r * 0.5, tempcolor.g * 1.2, tempcolor.b * 0.5);
}