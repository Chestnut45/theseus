// Camera UBO
layout(std140, binding = 0) uniform cameraBuffer
{
    mat4 viewProj;
};

uniform vec4 colour;
uniform float radius;
uniform vec3 lightPos;

in vec4 vertexPos;

out vec4 pixelColour;

void main() {
	// Thank you to D'Anyil for helping with this

	// Put the vertex position out of clip space and into world space by inversing the View-Projection matrix
	// (this inverse could be precomputed if this becomes a bottleneck)
	vec4 worldPos = inverse(viewProj) * vertexPos;

	// Compute the attenuation by getting the distance between the vertex and the light positions,
	// within the radius, as a value between 1 and 0
	float attenuationFactor = mix(1.0, 0.0, distance(worldPos.xy, lightPos.xy) / radius);

	// Multiply the color by the attenuation
	pixelColour = vec4(colour.r, colour.g, colour.b, colour.a) * attenuationFactor;
}