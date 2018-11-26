//Version number
#version 400

out vec4 FragColor;

in vec2 TexCoords;
uniform sampler2D texMap;

void main()
{
	FragColor = vec4(texture(texMap, TexCoords));
}