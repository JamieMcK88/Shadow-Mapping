#version 330 core

out vec4 FragColor;

in vec4 ShadowCoord;

uniform sampler2D shadowMap;

void main()
{
	float visibility = 1.0;
	if ( texture( shadowMap, ShadowCoord.xy ).z  <  ShadowCoord.z)
		visibility = 0.5;

	float depth = texture( shadowMap, ShadowCoord.xy ).r;
	FragColor = vec4(ShadowCoord.z, ShadowCoord.z, ShadowCoord.z, 1.0);
}