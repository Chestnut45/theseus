// Interpolated particle-relative position
in vec2 pos;

// Final output
out vec4 finalColor;

// Uniforms
uniform float kernelRadius;

void main()
{
    // Discard pixels outside of particle radius
    if (length(pos) > kernelRadius * 0.5) discard;

    // TODO: Final color should be tweakable
    finalColor = vec4(0.0, 0.1, 0.45, 0.5);
}