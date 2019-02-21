//#extension GL_EXT_gpu_shader4 : require

layout (location = 1) in vec3 in_pos;

uniform mat4 modelViewProj;
uniform vec3 Kd = vec3( 1.0, 1.0, 1.0);


#if VERT_SHADER_ENABLED
void main()
{
	gl_Position = vec4(in_pos.xyz, 1) * modelViewProj;
}
#endif


#if FRAG_SHADER_ENABLED
out vec4 outColor;
void main()
{
	outColor = vec4(Kd, 1);
	//gl_FragData[0] = vec4( position.xyz, 0.0);
}
#endif