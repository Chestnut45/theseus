// Constants
const float MAX_PRESSURE_DELTA = 128.0;

// Interpolated particle-relative position
in vec2 pos;
in vec4 forceDensityPressure;

// Final output
out vec4 finalColor;

// Simulation parameters
uniform float kernelRadius;
uniform float gasConstant;
uniform float restDensity;

// Fluid color parameters
uniform vec4 fluidColor;
uniform vec4 waveColor;

void main()
{
    // Discard pixels outside of particle radius
    if (length(pos) > kernelRadius * 0.5) discard;

    // Calculate fluid properties that should affect color
    // TODO: Ensure these values are properly normalized!
    float densityFactor = forceDensityPressure.z * 16.0;
    float pressureFactor = 1.0 - (-forceDensityPressure.w - (gasConstant * restDensity - MAX_PRESSURE_DELTA)) / MAX_PRESSURE_DELTA;

    // Compute color contributions
    vec4 waveColorContribution = max(waveColor * (1.0 - pressureFactor * 8.0), vec4(0.0));

    // Calculate final color
    finalColor = fluidColor + waveColorContribution;
}