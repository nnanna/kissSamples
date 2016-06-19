
#ifndef KISS_MODEL
#define KISS_MODEL

#include <RenderEngine\RenderData.h>

namespace ks
{
	class Model
	{
	public:
		Model();

		~Model();

		bool loadModelFromFile(const char* file);

		bool makeCube(float pSize);

		bool makeQuad(float pSize);

		ks32 getPositionSize() const;

		void computeNormals();

		void compileModel(PrimType prim = eTriangles);

		const float* getCompiledVertices() const;

		const ksUShort* getCompiledIndices(PrimType prim = eTriangles) const;
		
		ks32 getCompiledVertexSize() const;

		ks32 getCompiledVertexCount() const;

		ks32 getCompiledIndexCount(PrimType prim = eTriangles) const;

		ks32 getCompiledNormalOffset() const;

		PrimType getPrimType() const;
	
	private:

		float*	mCustomVB;

		ks32		mVertexCount;
		ks32		mIndexCount;
		ks32		mVertexSize;		// this is actually the size of the vertex position data
		PrimType	mPrimType;
	};
}

#endif