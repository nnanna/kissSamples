
#ifndef KS_PARTICLES_H
#define KS_PARTICLES_H

#pragma once;

#include <Maths/ks_Maths.h>
#include <Array.h>

namespace ks {

#define PARTICLE_GL_POINT_SIZE		5

//#if PREFER_STL_VECTOR
//
//#include <vector>
//	template <typename T>
//	using fxvector = std::vector
//#else
//
//#include <Array.h>
//	template <typename T>
//	using fxvector = typename ks::Array
//#endif

	struct Particles
	{
		Array<vec3>		positions;
		Array<vec3>		velocities;
		Array<vec3>		forces;
		Array<float>	durations;
		Array<float>	elapsed;
		//Array<ksU32>	seeds;		// use to auto-generate colour etc @TODO

		Particles(ksU32 pCapacity);

		void expire(ksU32 pIndex);
		void resize(ksU32 pSize);

		ksU32 live_count() const;
		void  set_live_count(ksU32 pCount);
		ksU32 capacity() const;
	private:
		ksU32	mLiveCount;
	};


	struct Emitter
	{
		Emitter();

		vec3	mWorldPos;
		vec3	mEmissionVelocity;
		ksU32	mEmissionRate;
		float	mEmissionBuffer;
		ksU32	mMaxParticles;
		ksU32	mFXID;
	};


	struct ParticleController
	{
		ParticleController();

		void prune(Particles& pParticles, float elapsed) const;
		void emit(Emitter& pEmitter, Particles& pParticles, float elapsed) const;

		void step(Particles& pParticles, float elapsed) const;


		vec4		SizeDurationRange;		// (x,y)-> size range; (z,w)-> duration range
		//vec4	ColourRange;
		vec3	InitVelocityRange;
		vec3	BaseAcceleration;
	};

}


#endif