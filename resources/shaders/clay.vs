#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;      // Model-View-Projection matrix (Camera)
uniform mat4 matModel; // Model matrix

// Custom uniform for shadow mapping
uniform mat4 lightSpaceMatrix; // The view-projection matrix of the Sun

// Output to fragment shader
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragPosition;
out vec3 fragNormal;
out vec4 fragPosLightSpace; // NEW: Where is this vertex relative to the Sun?

void main()
{
    // Pass standard variables to the fragment shader
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    
    // Calculate world position and normal
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    
    // Inverse transpose is used to correctly transform normals if the model is scaled
    mat3 normalMatrix = transpose(inverse(mat3(matModel)));
    fragNormal = normalize(normalMatrix * vertexNormal);

    // NEW: Calculate the position of this vertex from the Sun's point of view
    fragPosLightSpace = lightSpaceMatrix * vec4(fragPosition, 1.0);

    // Calculate final vertex position for the camera
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}