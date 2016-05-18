
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Nnanna Kama : Simple MATERIAL STRUCT:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "Material.h"
#include <Maths/ks_Maths.inl>
#include "SimpleShaderContainer.h"


Material::Material() : ShaderContainer(NULL), Shininess(0.01f)
{
	Ambient[0] = Diffuse[0] = Emissive[0] = Specular[0] = 0.3f;
	Ambient[1] = Diffuse[1] = Emissive[1] = Specular[1] = 0.3f;
	Ambient[2] = Diffuse[2] = Emissive[2] = Specular[2] = 0.3f;
}

Material::Material(SimpleShaderContainer* pShader) : ShaderContainer(pShader), Shininess(0.01f)
{
	Ambient[0] = Diffuse[0] = Emissive[0] = Specular[0] = 0.3f;
	Ambient[1] = Diffuse[1] = Emissive[1] = Specular[1] = 0.3f;
	Ambient[2] = Diffuse[2] = Emissive[2] = Specular[2] = 0.3f;
}

void Material::SetEmissive(float x, float y, float z)
{
	Emissive[0] = x; Emissive[1] = y; Emissive[2] = z;
}
void Material::SetDiffuse(float x, float y, float z)
{
	Diffuse[0] = x; Diffuse[1] = y; Diffuse[2] = z;
}
void Material::SetAmbient(float x, float y, float z)
{
	Ambient[0] = x; Ambient[1] = y; Ambient[2] = z;
}
void Material::SetSpecular(float x, float y, float z)
{
	Specular[0] = x; Specular[1] = y; Specular[2] = z;
}



void Material::SetShaderParams()
{
	if (ShaderContainer)
	{
		ShaderContainer->setVectorParameter(SP_EMISSIVE, Emissive);
		ShaderContainer->setVectorParameter(SP_AMBIENT, Ambient);
		ShaderContainer->setVectorParameter(SP_DIFFUSE, Diffuse);
		ShaderContainer->setVectorParameter(SP_SPECULAR, Specular);
		ShaderContainer->setFloatParameter(SP_SHININESS, Shininess);
	}
}



///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////////////
void Material::setBrassMaterial(Material* mat)
{
	mat->SetEmissive(0.0f, 0.0f, 0.0f);
	mat->SetAmbient(0.33f, 0.22f, 0.03f);
	mat->SetDiffuse(0.78f, 0.57f, 0.11f);
	mat->SetSpecular(0.99f, 0.91f, 0.81f);
	mat->Shininess = 27.8f;
}

void Material::setRedPlasticMaterial(Material* mat)
{
	mat->SetEmissive(0.0f, 0.0f, 0.0f);
	mat->SetAmbient(0.0f, 0.0f, 0.0f);
	mat->SetDiffuse(0.5f, 0.0f, 0.0f);
	mat->SetSpecular(0.7f, 0.6f, 0.6f);
	mat->Shininess = 32.0f;
}

void Material::setEmissiveLightColorOnly(Material* mat, const ks::vec3& pEmissiveCol)
{
	mat->SetEmissive(pEmissiveCol.x, pEmissiveCol.y, pEmissiveCol.z);
}