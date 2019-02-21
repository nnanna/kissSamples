//#extension GL_EXT_gpu_shader4 : require

//all these are in eye-space
varying vec3 l_pos;
varying vec3 l_nor;

layout (location = 1) in vec3 in_pos;
layout (location = 2) in vec3 in_nor;


uniform mat4 modelViewProj;
uniform vec3 globalAmbient = vec3( 1.0, 1.0, 1.0);
uniform vec3 lightColor = vec3( 1.0, 1.0, 1.0);
uniform vec3 lightPosition = vec3( 1.0, 1.0, 1.0);
uniform vec3 eyePosition = vec3( 1.0, 1.0, 1.0);
uniform vec3 Ke = vec3( 1.0, 1.0, 1.0);
uniform vec3 Ka = vec3( 1.0, 1.0, 1.0);
uniform vec3 Kd = vec3( 1.0, 1.0, 1.0);
uniform vec3 Ks = vec3( 1.0, 1.0, 1.0);
uniform float shininess = 0.0f;

#if VERT_SHADER_ENABLED
void main()
{
	gl_Position = vec4(in_pos.xyz, 1) * modelViewProj;

	l_pos = in_pos.xyz;
	l_nor = in_nor;
}
#endif


#if FRAG_SHADER_ENABLED
out vec4 outColor;
void main()
{
	vec3 P = l_pos;
	vec3 N = l_nor;

	// Compute emissive term
	vec3 emissive = Ke;

	// Compute ambient term
	vec3 ambient = Ka * globalAmbient;

	// Compute the diffuse term
	vec3 L = normalize(lightPosition - P);
	float diffuseLight = max(dot(N, L), 0);
	vec3 diffuse = Kd * lightColor * diffuseLight;

	// Compute the specular term
	vec3 V = normalize(eyePosition - P);
	vec3 H = normalize(L + V);
	float specularLight = pow(max(dot(N, H), 0), shininess);
	if (diffuseLight <= 0)
	{
		specularLight = 0;
	}
	vec3 specular = Ks * lightColor * specularLight;

	outColor = vec4(emissive + ambient + diffuse + specular, 1);
	//outColor = vec4(N.xyz, 1);
	//gl_FragData[0] = vec4( position.xyz, 0.0);
}
#endif