#version 120
//#extension GL_ARB_draw_buffers : require
//#extension GL_EXT_gpu_shader4 : require

in vec3 in_pos;

uniform mat4 modelViewProj;
uniform vec3 Kd = vec3( 1.0, 1.0, 1.0);

void main()
{
	gl_FragColor = vec4(Kd, 1);
	//gl_FragData[0] = vec4( position.xyz, 0.0);
}