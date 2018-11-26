#version 330 core

layout (location = 0) in vec3 in_Position;

uniform mat4 projection;
uniform mat4 modelview;

uniform mat4 DepthBiasMVP;

out vec4 ShadowCoord;

void main()
{
    gl_Position = projection * modelview * vec4(in_Position, 1.0f);
	
	// Same, but with the light's view matrix
	ShadowCoord = DepthBiasMVP * vec4(in_Position, 1);
}