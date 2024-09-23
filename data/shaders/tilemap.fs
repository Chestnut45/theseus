// Vertex shader outputs
in vec3 texCoords;

// Final color output
out vec3 finalColor;

// Tile set array texture sampler
layout(binding = 1) uniform sampler2DArray tileSetTex;

// Uniform tilemap tint color
uniform vec3 mapTint;

// Fragment shader entrypoint
void main()
{
    // Sample the tile texture
    vec4 textureColor = texture(tileSetTex, texCoords);

    // Discard transparent pixels
    if (textureColor.a == 0.0) discard;
    
    // Apply tint
    finalColor = textureColor.rgb * mapTint;
}