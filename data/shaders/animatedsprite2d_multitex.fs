// Vertex shader outputs
in vec2 texCoords;

// Final color output
out vec4 color;

// Sprite texture sampler at slot 0
layout(binding = 0) uniform sampler2D spriteTexture;
// Blending texture
layout(binding = 1) uniform sampler2D multiTex;


// Tint
uniform vec3 tint;

void main()
{
    // Sample sprite texture
    vec4 textureColor = texture(spriteTexture, texCoords);
    // Discard transparent pixels
    if (textureColor.a == 0.0) discard;

    vec4 multiTexColor = texture(multiTex, texCoords);

    color = multiTexColor * (textureColor * 0.5 + 0.5);
    color = color * (tint, 1.0);
    //color = textureColor.rgb * tint;
}