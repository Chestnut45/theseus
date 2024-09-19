uniform mat4 model;

in vec4 a_position;


void main() {
    gl_Position = model * a_position;
}