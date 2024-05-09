#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D textTexture;
uniform vec3 textColor;

void main()
{
    // Sample from the font texture atlas
    vec4 sampledColor = texture(textTexture, TexCoord);

    // Set the color of the text
    FragColor = vec4(textColor, sampledColor.a);
}
