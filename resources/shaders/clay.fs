#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragPosition;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

// The base darkness of the room
uniform vec3 ambientColor;

// --- MULTI-LIGHT SYSTEM ---
#define MAX_LIGHTS 16
uniform int activeLightCount;
uniform vec3 lightPositions[MAX_LIGHTS];
uniform vec3 lightColors[MAX_LIGHTS];
uniform float lightRadii[MAX_LIGHTS];

out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec3 baseColor = texelColor.rgb * colDiffuse.rgb * fragColor.rgb;
    vec3 N = normalize(fragNormal);

    // Start with the dark ambient room color
    vec3 totalIllumination = ambientColor;

    // Loop through every light in the room
    for(int i = 0; i < activeLightCount; i++) 
    {
        vec3 lightVec = lightPositions[i] - fragPosition;
        float distance = length(lightVec);

        // Only calculate if the pixel is actually inside the light's radius
        if (distance < lightRadii[i]) 
        {
            vec3 L = normalize(lightVec);

            // Wrapped Lambert (Claymation aesthetic)
            float NdotL = dot(N, L);
            float wrap = NdotL * 0.5 + 0.5;
            float diffuse = wrap * wrap;

            // Soft quadratic falloff (light fades out as it gets to the edge of the radius)
            float attenuation = 1.0 - (distance / lightRadii[i]);
            attenuation *= attenuation; 

            totalIllumination += lightColors[i] * diffuse * attenuation;
        }
    }

    finalColor = vec4(baseColor * totalIllumination, texelColor.a * colDiffuse.a);
}