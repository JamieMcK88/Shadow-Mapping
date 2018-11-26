// lightMap fragment shader
#version 330

// Some drivers require the following
precision highp float;

out vec4 outColour;

smooth in vec3 lightTexCoord;
uniform samplerCube lightMap;
 
void main(void) {   
	// Fragment colour
	outColour = texture(lightMap, lightTexCoord);
}