#version 120
//#extension GL_EXT_gpu_shader4 : require

in vec3 in_pos;

uniform mat4 modelViewProj;
uniform vec3 Kd = vec3( 1.0, 1.0, 1.0);


void main()
{
	gl_Position = vec4(in_pos.xyz, 1) * modelViewProj;
}