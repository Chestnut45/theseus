// Quad vertex inputs
layout(location = 0) in vec2 quadVertexPos;
layout(location = 1) in vec2 quadVertexUV;

// Tile vertex inputs (instanced per-quad, not per-quad-vertex)
// NOTE: Packed tile position into .xy, tile set layer index into .z
layout(location = 2) in vec3 tilePosLayer;

// Camera UBO
layout(std140, binding = 0) uniform cameraBuffer
{
    mat4 viewProj;
};

// Output to fragment shader
// NOTE: Array textures take a 3rd coordinate (layer index)
out vec3 texCoords;

// Model matrix to transform the tilemap to world space
uniform mat4 model;

// Vertex shader entrypoint
void main()
{
    // Pack texture coordinates
    texCoords = vec3(quadVertexUV, tilePosLayer.z);

    // Output position
    gl_Position = viewProj * model * vec4(quadVertexPos + tilePosLayer.xy, 0.0, 1.0);
}