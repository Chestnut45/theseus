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
        FragColor = ParticleColor;
    }
}