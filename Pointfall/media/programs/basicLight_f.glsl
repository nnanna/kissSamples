#version 120
//#extension GL_ARB_draw_buffers : require
//#extension GL_EXT_gpu_shader4 : require


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
uniform float shininess = 0.0f;

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


	gl_FragColor = vec4(emissive + ambient + diffuse + specular, 1);
	//gl_FragData[0] = vec4( position.xyz, 0.0);
}