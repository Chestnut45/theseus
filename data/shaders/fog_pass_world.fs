layout(std140, binding = 0) uniform cameraBuffer
{
    mat4 viewProj;
};

uniform float time;
uniform vec4 viewport;
uniform vec4 labyrinthDimWorldScale;

layout(binding = 6) uniform sampler2D fogMask;

in vec2 texCoords;
out vec4 finalColor;

// Noise methods taken from https://www.shadertoy.com/view/tst3zr
float random(vec2 uv);
vec2 random2(vec2 uv);
float noise(vec2 uv);
float fbm(vec2 uv);

// Fragment shader entrypoint
void main()
{
    // Convert screen coordinates to world coordinates
    vec4 clipSpacePos = vec4(texCoords * 2.0 - 1.0, 0.0, 1.0);
    vec4 worldPos = inverse(viewProj) * clipSpacePos;
    worldPos /= worldPos.w;

    // Grab initial coordinate and sample motion
    vec2 fogUV = worldPos.xy / 2000;
	vec2 motion = vec2(fbm(fogUV * 8 + vec2(time * -0.5, time * -0.3)));
    vec2 coord = fogUV * 8 + motion;

    // Sample the fog noise
    float value = fbm(coord) * 2;
    vec3 fogColor = mix(vec3(0.0, 0.0, 0.0), vec3(0.42, 0.40, 0.47), value);

    finalColor = vec4(fogColor, 0.32);
}

// Noise implementation

float random(vec2 uv)
{
    return fract(sin(dot(uv.xy, vec2(12.9818, 79.279))) * 43758.5453123);
}

vec2 random2(vec2 uv)
{
    uv = vec2(dot(uv, vec2(127.1, 311.7)), dot(uv, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(uv) * 7);
}

float noise(vec2 uv)
{
    vec2 i = floor(uv);
    vec2 f = fract(uv);

    // Smoothstep
    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix( mix(dot(random2(i + vec2(0.0, 0.0)), f - vec2(0.0, 0.0)),
                    dot(random2(i + vec2(1.0, 0.0)), f - vec2(1.0, 0.0)), u.x),
                mix(dot(random2(i + vec2(0.0, 1.0)), f - vec2(0.0, 1.0)),
                    dot(random2(i + vec2(1.0, 1.0)), f - vec2(1.0, 1.0)), u.x), u.y);
}

float fbm(vec2 uv)
{
    float value = 0.2;
	float scale = 0.2;
	for (int i = 0; i < 4; i++)
    {
		value += noise(uv) * scale;
		uv *= 2.0;
		scale *= 0.5;
	}
    return value;
}