
// Vertex array inputs
layout(location = 0) in vec2 vertexPosition;
layout(location = 2) in vec2 vertexUV;

out vec2 texCoords;

void main()
{
    texCoords = vertexUV;
    gl_Position =  vec4(vertexPosition, 0.0, 1.0); 
}