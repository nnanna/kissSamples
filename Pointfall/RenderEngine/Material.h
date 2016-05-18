
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Nnanna Kama : Simple MATERIAL STRUCT:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef MATERIAL_H
#define MATERIAL_H

class SimpleShaderContainer;
namespace ks
{
	struct vec3;
}


struct Material
{
	float	Ambient[3];
	float	Diffuse[3];
	float	Emissive[3];
	float	Specular[3];
	float	Shininess;

	SimpleShaderContainer*	ShaderContainer;

	Material();

	Material(SimpleShaderContainer* pShader);

	void SetEmissive(float x, float y, float z);
	void SetDiffuse(float x, float y, float z);
	void SetAmbient(float x, float y, float z);
	void SetSpecular(float x, float y, float z);

	void SetShaderParams();


	static void setBrassMaterial(Material* mat);

	static void setRedPlasticMaterial(Material* mat);

	static void setEmissiveLightColorOnly(Material* mat, const ks::vec3& pEmissiveCol);

};



#endif