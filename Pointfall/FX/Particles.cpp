
#include "Particles.h"
#include <Maths\ks_Maths.inl>
#include <Physics\AsyncSolver.h>
#include <Physics\Constraint.h>

namespace ks
{

	void customParticleFloorConstraint(float* positions, float* velocities, const float* sizes, int count, const Constraint& ct)
	{
#define PARTICLE_CONSTANT_RADIUS	0.2f
#define PARTICLE_IDLE_CUTOFF		0.001f

		// simple col-det & response - only considers plane at origin
		static RNG rng;
		static ks_thread_local float tlsRefl[3];
		static ks_thread_local float tlsDamping[3] = { 0.995f, 0.4f, 0.995f };	// x,z planar friction, (y) restitution
		vec3* pos			= (vec3*)positions;
		vec3* vel			= (vec3*)velocities;
		vec3& refl			= *(vec3*)&tlsRefl;
		vec3& floor_damping	= *(vec3*)&tlsDamping;

		for (int i = 0; i < count; ++i, ++vel)
		{
			if (pos[i].y <= PARTICLE_CONSTANT_RADIUS && vel->LengthSq() > PARTICLE_IDLE_CUTOFF)
			{
				vel->reflect(vec3::UNIT_Y, refl);
				*vel = refl;
				floor_damping.y = rng.GetFloatBetween(0.2f, 0.4f) - pos[i].y;
				*vel *= floor_damping;
				pos[i].y = PARTICLE_CONSTANT_RADIUS;
			}
		}
	}

	Constraint CreateParticleFloorConstraint()
	{
		Constraint ct;
		ct.mType = ct_custom;
		ct.custom.customSatisfy = &customParticleFloorConstraint;
		return ct;
	}

	static Constraint sParticleFloorConstraint = CreateParticleFloorConstraint();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Particles
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Particles::Particles(ksU32 pCapacity) : mLiveCount(0)
	{
		resize(pCapacity);
	}

	void Particles::expire(ksU32 pIndex)
	{
		KS_ASSERT(mLiveCount > 0);
		--mLiveCount;
		if (pIndex != mLiveCount)
		{
			positions[pIndex]	= positions[mLiveCount];
			velocities[pIndex]	= velocities[mLiveCount];
			forces[pIndex]		= forces[mLiveCount];
			durations[pIndex]	= durations[mLiveCount]; 
			elapsed[pIndex]		= elapsed[mLiveCount];
			//seeds[pIndex]		= seeds[mLiveCount];
		}
	}

	void Particles::resize(ksU32 pSize)
	{
		positions.resize(pSize);
		velocities.resize(pSize);
		forces.resize(pSize, vec3::ZERO_VEC);
		durations.resize(pSize, 0);
		elapsed.resize(pSize, 0);
		//seeds.resize(pSize);
	}

	ksU32 Particles::live_count() const				{ return mLiveCount; }

	void Particles::set_live_count(ksU32 pCount)	{ if (pCount < capacity()) mLiveCount = pCount; }

	ksU32 Particles::capacity() const				{ return positions.size(); }


	/////////////////////////////////////////////////////////////////////////////////////////
	// Emitter
	/////////////////////////////////////////////////////////////////////////////////////////
	Emitter::Emitter() : mEmissionBuffer(0.f), mFXID(0)
	{}

	/////////////////////////////////////////////////////////////////////////////////////////
	// ParticleController
	/////////////////////////////////////////////////////////////////////////////////////////
	ParticleController::ParticleController()
		: SizeDurationRange(0.f, 1.f, 0.9f, 1.3f)
		//, ColourRange(0.f,0.f,0.f,0.f)
		, InitVelocityRange(0.7f, 0.7f, 0.7f)
		, BaseAcceleration(0.f, -9.8f, 0.f)
	{}

	void ParticleController::prune(Particles& pParticles, float elapsed) const
	{
		for (ksU32 i = 0; i < pParticles.live_count(); ++i)
		{
			if (pParticles.elapsed[i] > pParticles.durations[i])
			{
				pParticles.expire(i);
				--i;
			}
			else
				pParticles.elapsed[i] += elapsed;
		}
	}

	void ParticleController::emit(Emitter& pEmitter, Particles& pParticles, float elapsed) const
	{
		ksU32 begin					= pParticles.live_count();
		pEmitter.mEmissionBuffer	+= (float)pEmitter.mEmissionRate * elapsed;
		ksU32 end					= begin + ksU32(pEmitter.mEmissionBuffer);
		pEmitter.mEmissionBuffer	-= (end - begin);

		if (end >= pParticles.capacity())
		{
			end = pParticles.capacity() - 1;
			pEmitter.mEmissionBuffer = 0;
		}


		RNG rng;
		vec3 offsets;
		for (ksU32 i = begin; i < end; ++i)
		{
			pParticles.positions[i]		= pEmitter.mWorldPos;

			rng.GetV3PlusMinus(offsets, InitVelocityRange);
			pParticles.velocities[i]	= pEmitter.mEmissionVelocity + offsets;
			pParticles.elapsed[i]		= 0.f;
			pParticles.durations[i]		= rng.GetFloatBetween(SizeDurationRange.z, SizeDurationRange.w);
		}

		pParticles.set_live_count(end);
	}

	void ParticleController::step( Particles& pParticles, float elapsed ) const
	{
		const ksU32 numParticles	= pParticles.live_count();
		CollisionSolver::ConstraintConfig ccfg;
		{
			ccfg.constraint		= &sParticleFloorConstraint;
			ccfg.numElements	= numParticles;
			ccfg.rPositions		= (float*)pParticles.positions.data(); 
			ccfg.rVelocities	= (float*)pParticles.velocities.data(); 
		}
		async_context ctx			= CollisionSolver::BeginAsync(pParticles.forces, numParticles, elapsed, &ccfg);

		float halfstep				= elapsed * 0.5f;
		const vec3 half_a_t			= BaseAcceleration * halfstep;
		vec3 vel, half_accel, pos;

		for (ksU32 i = 0; i < numParticles; ++i)
		{
			vel = pParticles.velocities[i];
			pos = pParticles.positions[i];
			half_accel = pParticles.forces[i];
			
			half_accel *= halfstep;
			half_accel += half_a_t;

			vel += half_accel;
			pos += (vel * elapsed);
			vel += half_accel;

			//sParticleFloorConstraint.Satisfy(&pos.x, &vel.x, nullptr, 1);	// TODO: make batched & async
			
			ctx.SubmitQuery( i, pos, vel );
			//pParticles.velocities[i] = vel;
			//pParticles.positions[i] = pos;
		}
	}
}

