#version 330 core
in vec3 TexCoords;

out vec4 FragColor;

uniform samplerCube skybox1; // First skybox texture
uniform samplerCube skybox2; // Second skybox texture
uniform float blendFactor;   // Blending factor

void main()
{    
    vec4 texColor1 = texture(skybox1, TexCoords);
    vec4 texColor2 = texture(skybox2, TexCoords);
    FragColor = mix(texColor1, texColor2, blendFactor);
}
