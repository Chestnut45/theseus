layout(location = 0) in vec2 a_position;
layout(location = 2) in vec2 a_UV;

out vec2 texCoords;

void main() {
    gl_Position = vec4(a_position, 0.0, 1.0);
    texCoords = a_UV;
}