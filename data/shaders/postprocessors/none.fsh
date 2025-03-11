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

    color = textureColor.rgb;
}