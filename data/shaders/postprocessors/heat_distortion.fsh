// Vertex shader outputs
in vec2 texCoords;

// Final color output
out vec3 color;

uniform float amplitude;
uniform float frequency;
uniform float time;
uniform float screenspaceRadius;
uniform float zoom;
uniform vec4 viewportSize; // Pass viewport size as uniform

// SSBO for worldspace points
layout(std430, binding = 1) buffer PointsBuffer
{
    vec2 points[];
};

// Sprite texture sampler at slot 0
layout(binding = 0) uniform sampler2D spriteTexture;

layout(std140, binding = 0) uniform cameraBuffer
{
    mat4 viewProj;
};


void main()
{
    vec2 screenCoord = gl_FragCoord.xy; // Use gl_FragCoord for screen space

    // Apply warp effect around points
    
    vec2 warp = texCoords;

    for (int i = 0; i < points.length(); i++) {
        vec2 point = points[i];
        
        // Transform point to screen space
        vec4 projectedPoint = viewProj * vec4(point, 0.0, 1.0);
        vec2 screenPoint = projectedPoint.xy / projectedPoint.w;

        // Normalise to [0,1] range
        screenPoint = screenPoint * 0.5 + 0.5;                      

        // Scale to screen space
        screenPoint = screenPoint * viewportSize.xy;                
        
        
        // Calculate distance in screen space
        float distanceFromPoint = length(screenCoord - screenPoint);
        
        if (distanceFromPoint < screenspaceRadius) {
            // Accumulate warp effects
            warp = vec2(warp.x + amplitude * sin(warp.y * frequency + time), warp.y);
            break;
        }
    }
    
    vec4 textureColor = texture(spriteTexture, warp);
    
    if (textureColor.a == 0.0) discard;
    
    vec3 tempColor = textureColor.rgb;

    color = tempColor;
}