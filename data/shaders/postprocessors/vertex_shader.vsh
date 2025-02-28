// Vertex array inputs
layout(location = 0) in vec2 vertexPosition;
layout(location = 2) in vec2 vertexUV;

// Output to fragment shader
out vec2 texCoords;

// Model matrix to transform the sprite to world space
uniform mat4 model;

// Vertex shader entrypoint
void main()
{
    texCoords = vertexUV;
    gl_Position =  vec4(vertexPosition, 0.0, 1.0); 
}