layout(binding = 9) uniform sampler2D fluidTex;

in vec2 texCoords;
out vec4 finalColor;

void main()
{
    // Discard transparent pixels
    vec4 fluidColor = texture(fluidTex, texCoords);
    if (fluidColor.a == 0.0) discard;
    finalColor = fluidColor;
}