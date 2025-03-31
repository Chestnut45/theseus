// Vertex shader outputs
in vec2 texCoords;

// Final color output
out vec3 color;

uniform float amplitude;
uniform float frequency;
uniform float time;
uniform vec4 viewportSize; // Pass viewport size as uniform
uniform vec4 rectangleSize; // Uniform for rectangle size (width, height)

// SSBO for worldspace points (bottom-left corners of rectangles)
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
    
    vec2 warp = texCoords;

    for (int i = 0; i < points.length(); i++) {
        vec2 point = points[i];

        // Transform point to screen space
        vec2 screenPoint = (viewProj * vec4(point, 0.0, 1.0)).xy;

        // Normalise to [0,1] range
        screenPoint = screenPoint * 0.5 + 0.5;                      

        // Scale to screen space
        screenPoint = screenPoint * viewportSize.xy;                
        
        // Calculate relative position within rectangle
        vec2 relativePos = screenCoord - screenPoint;
        
        // Check if pixel is within rectangle bounds
        if (relativePos.x >= 0.0 && relativePos.x <= rectangleSize.x && relativePos.y >= 0.0 && relativePos.y <= rectangleSize.y) {
            // Accumulate warp effects
            warp = vec2(warp.x + amplitude * sin((warp.y + warp.x) * frequency + time), warp.y);
            
        }
    }
    
    vec4 textureColor = texture(spriteTexture, warp);
    
    if (textureColor.a == 0.0) discard;
    
    vec3 tempColor = textureColor.rgb;

    color = tempColor;
}