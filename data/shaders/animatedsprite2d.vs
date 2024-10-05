// Vertex array inputs
layout(location = 0) in vec2 vertexPosition;
layout(location = 2) in vec2 vertexUV;

// Camera UBO
layout(std140, binding = 0) uniform cameraBuffer
{
    mat4 viewProj;
};

// Output to fragment shader
out vec2 texCoords;

// Model matrix to transform the sprite to world space
uniform mat4 model;

// Vertex shader entrypoint
void main()
{
    // Output texture coordinates
    texCoords = vertexUV;

    // Output position
    gl_Position = viewProj * model * vec4(vertexPosition, 0.0, 1.0);
}