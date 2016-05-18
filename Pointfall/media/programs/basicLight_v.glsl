#version 120
//#extension GL_EXT_gpu_shader4 : require

//all these are in eye-space
varying vec4 color;
varying vec3 l_pos;
varying vec3 l_nor;

in vec3 in_pos;
in vec3 in_nor;



uniform mat4 modelViewProj;
uniform vec3 globalAmbient = vec3( 1.0, 1.0, 1.0);
uniform vec3 lightColor = vec3( 1.0, 1.0, 1.0);
uniform vec3 lightPosition = vec3( 1.0, 1.0, 1.0);
uniform vec3 eyePosition = vec3( 1.0, 1.0, 1.0);
uniform vec3 Ke = vec3( 1.0, 1.0, 1.0);
uniform vec3 Ka = vec3( 1.0, 1.0, 1.0);
uniform vec3 Kd = vec3( 1.0, 1.0, 1.0);
uniform vec3 Ks = vec3( 1.0, 1.0, 1.0);
uniform float  shininess = 0.0f;


void main()
{
	gl_Position = vec4(in_pos.xyz, 1) * modelViewProj;

	l_pos = in_pos.xyz;
	l_nor = in_nor;
}