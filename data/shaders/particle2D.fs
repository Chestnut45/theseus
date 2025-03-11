in vec4 ParticleColor;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D particleTexture;
uniform int useTexture;

void main()
{
    if (useTexture == 1)
    {
        vec4 texColor = texture(particleTexture, TexCoord);
        
        if (texColor.a < 0.1) discard;
        
        FragColor = texColor * ParticleColor;
    }
    else
    {
        // For point particles, create a circular shape
        float distance = length(gl_PointCoord - vec2(0.5));
        if (distance > 0.5) {
            discard; // Discard fragments outside the circle
        }
        FragColor = ParticleColor;
    }
}