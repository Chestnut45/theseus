// Vertex shader outputs
in vec2 texCoords;

// Final color output
out vec3 color;

// Sprite texture sampler at slot 0
layout(binding = 0) uniform sampler2D spriteTexture;
// Blending texture
uniform sampler2D blendTex;


// Tint
uniform vec3 tint;

void main()
{
    // Sample sprite texture
    vec4 textureColor = texture(spriteTexture, texCoords);
    if (textureColor.a == 0.0) discard;

    vec4 blendTexColor = texture(blendTex, texCoords);

    // Discard transparent pixels
    color = (1.0 - blendTexColor.a) * textureColor.rgb + blendTexColor.a * blendTexColor.rgb;
    color = color * tint;
    //color = textureColor.rgb * tint;
}