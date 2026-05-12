#version 330

// Input vertex attributes (from Raylib's internal full-screen quad)
in vec2 fragTexCoord;
in vec4 fragColor;

// The Render Target texture
uniform sampler2D texture0;

// Custom uniforms for tweaking the final look
uniform vec3 tintColor;
uniform float vignetteStrength;

out vec4 finalColor;

// --- ACES TONEMAPPING ---
// This is an industry-standard curve used in film and high-end games.
// It softly compresses bright highlights and deepens midtones.
vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    // 1. Fetch the raw rendered pixel from your game
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec3 color = texelColor.rgb;

    // 2. Apply the warm museum tint
    color *= tintColor;

    // 3. Run the colors through the ACES Filmic curve
    color = ACESFilm(color);

    // 4. Calculate a subtle Vignette
    // Convert texture coordinates to a -1.0 to 1.0 range (centered)
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    float dist = length(uv);
    // Smoothly fade to black as we get further from the center
    float vignette = smoothstep(1.5, 1.5 - vignetteStrength, dist);
    
    color *= vignette;

    // 5. Output the final cinematic frame
    finalColor = vec4(color, texelColor.a);
}