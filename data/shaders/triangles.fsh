in vec4 vertexColour;
out vec4 pixelColour;

void main() {
    pixelColour = vertexColour;
    pixelColour = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}