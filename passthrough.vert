#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 in_Position;

// Output data ; will be interpolated for each fragment.
out vec2 UV;

void main(){
	gl_Position =  vec4(in_Position,1);
	UV = (in_Position.xy+vec2(1,1))/2.0;
}