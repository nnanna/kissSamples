
//////////////////////////////////////////////////////////////////////////
///
/// Copyright (c)
///	@author		Nnanna Kama
///	@date		11/01/2015
///
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
/// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
/// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////

#define USE_STL_SORT	1	// faster for huge datasets but slower in debug mode

#include "AsyncSolver.h"
#include "Constraint.h"
#include "CollisionDefaults.h"
#include <AppLayer\InputListener.h>
#include <Maths\ks_Maths.inl>
#include <insertion.h>
#include <Service.h>
#include <Concurrency\JobScheduler.hpp>
#include <Concurrency\JobGroup.h>
#include <Concurrency\Semaphore.h>
#include <Concurrency\ReadWriteLock.h>
#include <Concurrency\BitLock.h>
#include <Memory\ThreadStackAllocator.h>
#include <Memory\FrameAllocator.h>
#include <Scripting\ScriptFactory.h>
#include <Profiling\Trace.h>
#include <unordered_map>
#if USE_STL_SORT
#include <algorithm>
#endif

static ksU32 gNumCollisions(0);
CollisionDefaults gCD;
CollisionDefaults gLocalDefaults(0.2f, 2);

ks::ScriptFactory gScriptLoader(nullptr);

namespace ks {

	using namespace mem;

	struct BatchWorker
	{
		template<typename _FN>
		void QueueJob(_FN&& pFunctor, const char* pName, ksU32 pEndMargin = 0)
		{
			if (sJobStream)
			{
				if (pEndMargin != 0)
					sJobStream->QueueAtEnd(ks::move(pFunctor), pName, pEndMargin);
				else
					sJobStream->Add(ks::move(pFunctor), pName);
			}
			else
				Service<JobScheduler>::Get()->QueueJob(ks::move(pFunctor), pName);
		}

		static JobGroup* sJobStream;
	}BatchWorker;
	JobGroup* BatchWorker::sJobStream = nullptr;

	static ReadWriteLock sRWLock;
	static std::unordered_map<size_t, LocalSolver*> sLocalSolverMap;

	struct DistanceIndex		// the next best thing to space partitioning
	{
		float distance;
		ksU32 index;

		inline bool operator< (const DistanceIndex& other) const	{ return distance < other.distance; }
		inline bool operator> (const DistanceIndex& other) const	{ return distance > other.distance; }
	};

	struct LocalSolver
	{
		static LocalSolver* Acquire(void* pClientAddress)
		{
			size_t key = size_t(pClientAddress);
			
			auto rGuard = sRWLock.Read();
			auto itr = sLocalSolverMap.find(key);
			if (itr == sLocalSolverMap.end())
			{
				rGuard.Release();
				auto wGuard = sRWLock.Write();
				sLocalSolverMap[key] = new LocalSolver();		// mem leak @TODO
				itr = sLocalSolverMap.find(key);
			}

			return itr->second;
		}

		void reset()
		{
			mIndex = mCapacity = mRevision = 0;
			mVelocities = mPositions = nullptr;
		}

		void init(StackBuffer& stack, size_t pNumElements)
		{
			mVelocities			= (vec3*)stack.allocate(pNumElements * sizeof(vec3));
			mPositions			= (vec3*)stack.allocate(pNumElements * sizeof(vec3));
			mSortedDistances	= (DistanceIndex*)stack.allocate(pNumElements * sizeof(DistanceIndex));

			mCapacity			= pNumElements;
		}

		bool reading() const		{ return mCapacity > 0 && mIndex < mCapacity; }

		ksU32 capacity() const		{ return mCapacity; }

		void asyncQuerySort()
		{
			DistanceIndex dist;
			while ( reading() )
			{
				if (mIndex == mRevision)
				{
					ksYieldThread;
				}
				while (mIndex < mRevision)
				{
					dist.distance = mPositions[mIndex].LengthSq();
					dist.index = mIndex;
#if USE_STL_SORT
					mSortedDistances[mIndex] = dist;
#else
					binary_insert_sorted(mSortedDistances, mIndex, dist, descending_sort_predicate<DistanceIndex>());
#endif
					++mIndex;
				}
			}
#if USE_STL_SORT
			TRACE_BEGIN("sort query", sorttrace);
			std::sort(mSortedDistances, mSortedDistances + mIndex, ascending_sort_predicate<DistanceIndex>());
			TRACE_END(sorttrace);
#endif
		}

		ksU32 resolveCollisions(vec3* pResults, float elapsed)
		{
			TRACE_FUNC();
			ksU32 num_collisions(0);
			DistanceIndex di, dk;
			vec3 impulse_v, norm;
			for (ksU32 i = 0, k = 1; k < mCapacity; ++i, ++k)
			{
				di = *(mSortedDistances + i);
				dk = *(mSortedDistances + k);

				while ( (di.distance - dk.distance) <= gLocalDefaults.COLLISION_RADIUS_SQ )	// don't need no fabs(), descending order sorted
				{
					KS_ASSERT(di.index < mCapacity && dk.index < mCapacity);
					norm = mPositions[di.index] - mPositions[dk.index];
					if (norm.LengthSq() <= gLocalDefaults.COLLISION_RADIUS_SQ)
					{
						norm.FastNormalize();

						impulse_v	= -norm;
						impulse_v	*= norm.Dot( mVelocities[dk.index] - mVelocities[di.index] );
						impulse_v /= gLocalDefaults.COLLISION_RADIUS_SQ;			// just because

						accumulate( pResults, di.index, -impulse_v );
						accumulate( pResults, dk.index,  impulse_v );
						++num_collisions;
					}

					if (++k >= mCapacity || k - i > gLocalDefaults.MAX_PER_ENTITY_COLLISIONS)
						break;

					dk = *(mSortedDistances + k);
				}
				k = i + 1;
			}

			return num_collisions;
		}

		void accumulate(vec3* pResults, ksU32 index, const vec3& val)
		{
			//mResultsGuard.Lock128(index);
			pResults[ index ] += val;
			//mResultsGuard.Unlock128(index);
		}

		static ksU32 AllocSize( ksU32 pNumElements )
		{
			return pNumElements * ( (sizeof(vec3) * 2) + sizeof(DistanceIndex) );
		}

		BitLock			mResultsGuard;
		DistanceIndex*	mSortedDistances;
		vec3*			mPositions;
		vec3*			mVelocities;
		ksU32			mRevision;
		Semaphore		IdleWatch;
		Event			mConstraintRunning;
	private:
		LocalSolver() : mVelocities(nullptr), mPositions(nullptr), mRevision(0), mCapacity(0), mIndex(0), mConstraintRunning(false)
		{
			IdleWatch.signal();		// init to signaled state so the first AwaitQueryCompletion passes.
		}
		LocalSolver(const LocalSolver&) = delete;
		LocalSolver& operator=(const LocalSolver&) = delete;
		ksU32 mCapacity;
		ksU32 mIndex;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// async_context
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	async_context::async_context(LocalSolver& pSolver) : mSolver(pSolver)
	{}

	void async_context::SubmitQuery(ksU32 pResultIndex, const vec3& pPos, const vec3& pVel)
	{
		mSolver.mPositions[pResultIndex]	= pPos;
		mSolver.mVelocities[pResultIndex]	= pVel;
		++mSolver.mRevision;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Global Solver
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct GlobalSolver
	{
#define SOLVER_MEM_CAPACITY						10 * MEGABYTE
#define	SOLVER_QUERY_CAPACITY					16

		StackBuffer FrameBuffer(ksU32 pCapacity)
		{
			void* buffer = mFrameAllocator.allocate(pCapacity);
			StackBuffer b(buffer, pCapacity);

			return b;
		}

		void Reset()
		{
			mNumQueries = mQueryIDs = 0;
		}

		void Submit(LocalSolver& pLS, vec3* pResults, float elapsed)
		{
			memset(pResults, 0, (pLS.capacity() * sizeof(vec3)));
			local_query lq =
			{
				pResults,
				pLS.mPositions,
				pLS.mVelocities,
				pLS.mSortedDistances,
				pLS.capacity(),
				&pLS
			};

			ksU32 queryIndex(0);
			{
				auto wLock = mRWLock.Write();
				queryIndex = mQueryIDs++;
				if (queryIndex < SOLVER_QUERY_CAPACITY)
					mQueries[queryIndex] = lq;
			}

			if(queryIndex > 0 && queryIndex < SOLVER_QUERY_CAPACITY)	// the first query doesn't need a global solver
			{
#define BW_PRIO_LOW	3
				for (ksU32 i = 0; i < queryIndex; ++i)
				{
					atomic_increment(&mNumQueries);

					BatchWorker.QueueJob( [this, elapsed, i, queryIndex]() -> ksU32
					{
						return resolveInterQueryCollisions(elapsed, i, queryIndex);
					},
					"GlobalSolver", BW_PRIO_LOW);
				}
			}
		}

		void AwaitCompletion()
		{
			int completedQueries(0);
			int numQueries = mNumQueries;

			TRACE_SCOPE("AwaitCompletion");
			while (numQueries - completedQueries > 0)
			{
				mQueryCompleted.wait();
				++completedQueries;
				numQueries = mNumQueries;
			}

			KS_ASSERT(mNumQueries == completedQueries);
			Reset();

			DEBUG_PRINT("num collisions: %d     \r", gNumCollisions);
			gNumCollisions = 0;
		}

		void BeginBatch()
		{
			const ksU32 upKey		= InputListener::getKeyUp();
			const bool reloadScript	= (upKey == 'r');
			if (mScript == nullptr || reloadScript)
			{
				mScript = gScriptLoader.Load("collision_overrides", &gCD, reloadScript);
			}
			AwaitCompletion();
		}

		void EndBatch()
		{}

		GlobalSolver() : mFrameAllocator(SOLVER_MEM_CAPACITY, true), mNumQueries(0), mQueryIDs(0), mScript(nullptr)
		{
			local_query qempty	= {};
			mQueries.resize(SOLVER_QUERY_CAPACITY, qempty);
		}

		~GlobalSolver()
		{
			gScriptLoader.Unload(mScript);
		}

	private:
		struct local_query
		{
			vec3*			results;
			vec3*			positions;
			vec3*			velocities;
			DistanceIndex*	sortedDistance;
			ksU32			capacity;
			LocalSolver*	local_solver;
		};

		ksU32 resolveInterQueryCollisions(float elapsed, ksU32 query_i, const ksU32 query_j) const
		{
			vec3 norm, impulse_v;
			ksU32 total_collisions(0);

			const local_query& qi = mQueries[query_i];
			const local_query& qj = mQueries[query_j];

			int iindex = qi.capacity;
			while (iindex-- > 0 && total_collisions < gCD.MAX_TOTAL_COLLISIONS)
			{
				float dist(0.f);
				ksU32 j_collisions(0);

				struct lbound_distance_pred
				{
					lbound_distance_pred(const DistanceIndex& pCtx) : mDistance(pCtx.distance)	{}
					inline bool operator()(const DistanceIndex& p) const						{ return p.distance > mDistance - gCD.COLLISION_RADIUS_SQ; }

					const float mDistance;
				};

				int jindex = ks::binary_find(qj.sortedDistance, qj.capacity, lbound_distance_pred(qi.sortedDistance[iindex])) + 1;	// +1 so we can straight-up decrement it below.

				while (--jindex >= 0 && dist < gCD.COLLISION_RADIUS_SQ && j_collisions < gCD.MAX_PER_ENTITY_COLLISIONS)
				{
					dist = qi.sortedDistance[iindex].distance - qj.sortedDistance[jindex].distance;
					dist = dist < 0.f ? -dist : dist;
					if (dist < gCD.COLLISION_RADIUS_SQ)						// 1D distance test
					{
						const ksU32 i = qi.sortedDistance[iindex].index;
						const ksU32 j = qj.sortedDistance[jindex].index;
						norm = qi.positions[i] - qj.positions[j];
						if (norm.LengthSq() < gCD.COLLISION_RADIUS_SQ)		// 3D distance test
						{
							norm.FastNormalize();

							impulse_v = -norm;
							impulse_v *= norm.Dot(qj.velocities[j] - qi.velocities[i]);
							impulse_v /= gCD.IMPULSE_FACTOR;		// exaggerate

							qi.local_solver->accumulate(qi.results, i, -impulse_v);
							qj.local_solver->accumulate(qj.results, j, impulse_v);

							++j_collisions;
							++total_collisions;
						}
					}
				}
			}
			mQueryCompleted.signal();

			atomic_add(&gNumCollisions, total_collisions);
			return total_collisions;
		}

		ReadWriteLock		mRWLock;
		FrameAllocator		mFrameAllocator;
		ksU32				mQueryIDs;
		ksU32				mNumQueries;
		Array<local_query>	mQueries;
		mutable Semaphore	mQueryCompleted;
		ScriptInterface*	mScript;
	}GlobalSolver;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CollisionSolver
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	async_context CollisionSolver::BeginAsync(Array<vec3>& pForceResults, ksU32 pNumElements, float elapsed, const ConstraintConfig* pConstraint )
	{
		LocalSolver& ls		= *LocalSolver::Acquire( &pForceResults );

		AwaitQueryCompletion(ls);

		StackBuffer stack	= GlobalSolver.FrameBuffer( LocalSolver::AllocSize(pNumElements) );
		ls.init(stack, pNumElements);

		KS_ASSERT(pNumElements <= pForceResults.size());

		auto solver = [&pForceResults, &ls, pNumElements, elapsed]() -> ksU32
		{
			ls.asyncQuerySort();

			GlobalSolver.Submit(ls, pForceResults.data(), elapsed);

			ksU32 numCollisions = ls.resolveCollisions(pForceResults.data(), elapsed);

			atomic_add(&gNumCollisions, numCollisions);

			// on_complete
			ls.IdleWatch.signal();

			return numCollisions;
		};

		BatchWorker.QueueJob(solver, "CollisionSolver");

		if (pConstraint)
		{
			ConstraintConfig cc = *pConstraint;
			ls.mConstraintRunning.SetState(true);
			auto constraint_solver = [cc, &ls]() -> ksU32
			{
				ksU32 index(0);
				ksU32 collisions(0);
				vec3* rPos = (vec3*)cc.rPositions;
				vec3* rVel = (vec3*)cc.rVelocities;
				while (index < cc.numElements)
				{
					const ksU32 rev = ls.mRevision;
					while (index < rev)
					{
						const ksU32 rsize = rev - index;
						memcpy(rPos + index, ls.mPositions + index, rsize * sizeof(vec3));
						memcpy(rVel + index, ls.mVelocities + index, rsize * sizeof(vec3));
						
						collisions += cc.constraint->Satisfy(&rPos[index].x, &rVel[index].x, nullptr, rsize);
						
						index = rev;
					}
					if ( index == ls.mRevision && index < cc.numElements )
						ksYieldThread;											// stall for more submissions
				}

				atomic_add(&gNumCollisions, collisions);
				ls.mConstraintRunning.Notify();
				return collisions;
			};

			BatchWorker.QueueJob(constraint_solver, "ConstraintSolver");
		}

		return async_context(ls);
	}

	void CollisionSolver::AwaitQueryCompletion(Array<vec3>& pForceResults)
	{
		LocalSolver& ls = *LocalSolver::Acquire(&pForceResults);
		AwaitQueryCompletion( ls );
	}

	void CollisionSolver::AwaitQueryCompletion(LocalSolver& pSolver)
	{
		pSolver.IdleWatch.wait();
		pSolver.mConstraintRunning.Wait();	// don't release StackBuffer or reset localsolver while constraint's still running
		pSolver.reset();
	}

	void CollisionSolver::BeginBatch(JobGroup* pJobStream /*= nullptr*/)
	{
		GlobalSolver.BeginBatch();
		BatchWorker.sJobStream = pJobStream;
	}

	void CollisionSolver::EndBatch()
	{
		GlobalSolver.EndBatch();
	}
}