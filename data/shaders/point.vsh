uniform mat4 model;
// Camera UBO
layout(std140, binding = 0) uniform cameraBuffer
{
    mat4 viewProj;
};

in vec4 a_position;


void main() {
    gl_Position = viewProj * model * a_position;
}