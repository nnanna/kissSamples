
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
#include <Maths\ks_Maths.inl>
#include <insertion.h>
#include <Service.h>
#include <Concurrency\JobScheduler.hpp>
#include <Concurrency\Semaphore.h>
#include <Concurrency\ReadWriteLock.h>
#include <Concurrency\BitLock.h>
#include <Memory\ThreadStackAllocator.h>
#include <unordered_map>
#if USE_STL_SORT
#include <algorithm>
#endif

static ksU32 gNumCollisions(0);

enum BatchState
{
	bs_wait,
	bs_ready,
	bs_green
};


namespace ks {

	using namespace mem;

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
			mVelocities = mPositions = nullptr;
			mIndex = mCapacity = mRevision = 0;
		}

		void init(StackBuffer& stack, size_t pNumElements)
		{
			mVelocities			= (vec3*)stack.allocate(pNumElements * sizeof(vec3));
			mPositions			= (vec3*)stack.allocate(pNumElements * sizeof(vec3));
			mSortedDistances	= (DistanceIndex*)stack.allocate(pNumElements * sizeof(DistanceIndex));

			mCapacity			= pNumElements;
		}

		bool in_progress() const	{ return reading() || solving(); }

		bool reading() const		{ return mCapacity > 0 && mIndex < mCapacity; }

		bool solving() const		{ return mIndex != 0; }

		ksU32 capacity() const		{ return mCapacity; }

		void asyncQuerySort()
		{
			DistanceIndex dist;
			while ( reading() )
			{
				if (mIndex == mRevision)
				{
					THREAD_SWITCH;
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
			std::sort(mSortedDistances, mSortedDistances + mIndex, ascending_sort_predicate<DistanceIndex>());
#endif
		}

		ksU32 resolveCollisions(vec3* pResults, float elapsed)
		{
#define COLLISION_RADIUS_SQ				0.2f		// hardcode for now. @TODO
#define MAX_PER_ENTITY_COLLISIONS		2
			ksU32 num_collisions(0);
			DistanceIndex di, dk;
			vec3 impulse_v, norm;
			for (ksU32 i = 0, k = 1; k < mCapacity; ++i, ++k)
			{
				di = *(mSortedDistances + i);
				dk = *(mSortedDistances + k);

				while ( (di.distance - dk.distance) <= COLLISION_RADIUS_SQ )	// don't need no fabs(), descending order sorted
				{
					norm = mPositions[di.index] - mPositions[dk.index];
					if (norm.LengthSq() <= COLLISION_RADIUS_SQ)
					{
						norm.FastNormalize();

						impulse_v	= -norm;
						impulse_v	*= norm.Dot( mVelocities[dk.index] - mVelocities[di.index] );
						impulse_v	/= COLLISION_RADIUS_SQ;			// just because

						accumulate( pResults, di.index, -impulse_v );
						accumulate( pResults, dk.index,  impulse_v );
						++num_collisions;
					}

					if (++k >= mCapacity || k - i > MAX_PER_ENTITY_COLLISIONS)
						break;

					dk = *(mSortedDistances + k);
				}
				k = i + 1;
			}

			return num_collisions;
		}

		void accumulate(vec3* pResults, ksU32 index, const vec3& val)
		{
			mResultsGuard.Lock128(index);
			pResults[ index ] += val;
			mResultsGuard.Unlock128(index);
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
		{}
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

	void async_context::SyncConstraints()
	{
		mSolver.mConstraintRunning.Wait();
	}

	void async_context::SubmitQuery(ksU32 pResultIndex, const vec3& pPos, const vec3& pVel)
	{
		while (mSolver.capacity() == 0)
		{
			THREAD_SWITCH;			// wait for solver workspace to be allocated
		}

		mSolver.mPositions[pResultIndex]	= pPos;
		mSolver.mVelocities[pResultIndex]	= pVel;
		++mSolver.mRevision;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Global Solver
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct GlobalSolver
	{
#define SOLVER_MEM_CAPACITY						5 * MEGABYTE

		StackBuffer FrameBuffer(ksU32 pCapacity)
		{
			int index(-1);
			{
				auto wGuard = mRWLock.Write();
				if ((SOLVER_MEM_CAPACITY - mSize) > pCapacity)
				{
					index = mSize;
					mSize += pCapacity;
				}
				else	// cycle round
				{
					index = 0;
					mSize = pCapacity;
				}
			}

			StackBuffer b(nullptr, 0);
			if (index >= 0)
			{
				b = StackBuffer(mBuffer + index, pCapacity);
			}
			else
			{
				KS_ASSERT(!"Out of memory");
			}

			return b;
		}

		void Reset()							{ mQueries.clear(); mNumQueries = 0; }

		void Submit(LocalSolver& pLS, vec3* pResults, float elapsed)
		{
			while (mCompletionMarker == bs_wait)
			{
				THREAD_SWITCH;
			}
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

			ksU32 numQueries = atomic_increment(&mNumQueries);
			{
				auto wGuard = mRWLock.Write();
				mQueries.push_back(lq);
			}

			if (numQueries > 1)
			{
				Service<JobScheduler>::Get()->QueueJob(
					[this, elapsed, numQueries]() -> ksU32
					{
						return resolveInterQueryCollisions( elapsed, numQueries );
					},
					"GlobalSolver");
			}
		}

		void AwaitCompletion()
		{
			if (atomic_compare_and_swap(&mCompletionMarker, bs_green, bs_wait) == bs_green)
			{
				int completedQueries	= 1;			// the first query doesn't count
				int numQueries			= mNumQueries;

				while (numQueries - completedQueries > 0)
				{
					mCompleted.wait();
					atomic_increment((unsigned*)&completedQueries);
					numQueries = mNumQueries;
				}

				Reset();
				atomic_set(&mCompletionMarker, bs_ready);

				DEBUG_PRINT("num collisions: %d\n", gNumCollisions);
				gNumCollisions = 0;
			}
		}

		void BeginBatch()
		{
			//AwaitCompletion();	// best to delay sync till new local queries are ready -> in AwaitQueryCompletion()
		}

		void EndBatch()
		{
			mCompletionMarker = bs_green;
		}


		static GlobalSolver& Get();

		GlobalSolver() : mSize(0), mCompletionMarker(0)
		{
			mBuffer			= new char[ SOLVER_MEM_CAPACITY ];
			mQueries.reserve(16);
		}

		~GlobalSolver()
		{
			delete[] mBuffer;
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

		ksU32 resolveInterQueryCollisions(float elapsed, ksU32 pQueryIndex)
		{
#define G_COLLISION_RADIUS_SQ		1.2f
#define G_MAX_PER_ENTITY_COLLISIONS	2
#define G_MAX_TOTAL_COLLISIONS		8000
#define G_IMPULSE_FACTOR			0.4f
			vec3 norm, impulse_v;
			ksU32 total_collisions(0);

			local_query qj;
			{
				auto rGuard = mRWLock.Read();
				qj = mQueries[pQueryIndex - 1];
			}

			for (ksU32 i = 0; i + 1 < pQueryIndex; ++i)
			{
				local_query qi;
				{
					auto rGuard = mRWLock.Read();
					qi = mQueries[i];
				}

				int iindex = qi.capacity;
				while (iindex-- > 0 && total_collisions < G_MAX_TOTAL_COLLISIONS)
				{
					struct lbound_distance_pred
					{
						lbound_distance_pred(const DistanceIndex& pCtx) : mCtx(pCtx)
						{}

						inline bool operator()(const DistanceIndex& p) const
						{
							return p.distance > mCtx.distance - G_COLLISION_RADIUS_SQ;
						}
						const DistanceIndex& mCtx;
					};
					int jindex = ks::binary_find(qj.sortedDistance, qj.capacity, lbound_distance_pred(qi.sortedDistance[iindex]));
					++jindex;	// so we can straight-up decrement it in the while loop.
					ksU32 j_collisions(0);
					float dist(0.f);
					while (--jindex >= 0 && dist < G_COLLISION_RADIUS_SQ && j_collisions < G_MAX_PER_ENTITY_COLLISIONS)
					{
						dist = qi.sortedDistance[iindex].distance - qj.sortedDistance[jindex].distance;
						dist = dist < 0.f ? -dist : dist;
						if (dist < G_COLLISION_RADIUS_SQ)
						{
							const ksU32 i = qi.sortedDistance[iindex].index;
							const ksU32 j = qj.sortedDistance[jindex].index;
							norm = qi.positions[i] - qj.positions[j];
							if (norm.LengthSq() < G_COLLISION_RADIUS_SQ)
							{
								norm.FastNormalize();

								impulse_v = -norm;
								impulse_v *= norm.Dot(qj.velocities[j] - qi.velocities[i]);
								impulse_v /= G_IMPULSE_FACTOR;		// exaggerate

								qi.local_solver->accumulate(qi.results, i, -impulse_v);
								qj.local_solver->accumulate(qj.results, j, impulse_v);

								++j_collisions;
								++total_collisions;
							}
						}
					}
				}
			}
			mCompleted.signal();

			atomic_add(&gNumCollisions, total_collisions);
			return total_collisions;
		}

		ReadWriteLock		mRWLock;
		char*				mBuffer;
		ksU32				mSize;
		ksU32				mNumQueries;
		ksU32				mCompletionMarker;
		Array<local_query>	mQueries;
		Semaphore			mCompleted;
	};

	static GlobalSolver	sGlobalSolver;

	GlobalSolver& GlobalSolver::Get()
	{
		return sGlobalSolver;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CollisionSolver
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	async_context CollisionSolver::BeginAsync(Array<vec3>& pForceResults, ksU32 pNumElements, float elapsed, const ConstraintConfig* pConstraint )
	{
		JobScheduler* scheduler = Service<JobScheduler>::Get();
		LocalSolver& ls			= *LocalSolver::Acquire( &pForceResults );

		KS_ASSERT(pNumElements <= pForceResults.size());

		auto solver = [&pForceResults, &ls, pNumElements, elapsed]() -> ksU32
		{
			auto& gSolver		= GlobalSolver::Get();
			StackBuffer stack	= gSolver.FrameBuffer( LocalSolver::AllocSize(pNumElements) );
			ls.init(stack, pNumElements);

			ls.asyncQuerySort();

			gSolver.Submit(ls, pForceResults.data(), elapsed);

			ksU32 numCollisions = ls.resolveCollisions(pForceResults.data(), elapsed);

			ls.mConstraintRunning.Wait();

			return numCollisions;
		};

		auto on_complete = [ &ls ](ksU32 num_collisions)
		{
			atomic_add(&gNumCollisions, num_collisions);
			KS_ASSERT(ls.reading() == false);
			ls.reset();
			ls.IdleWatch.signal();
		};

		AwaitQueryCompletion( ls );

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
						THREAD_SWITCH;											// stall for more submissions
				}
				return collisions;
			};

			auto on_constraint_complete = [&ls](ksU32 num_collisions)
			{
				atomic_add(&gNumCollisions, num_collisions);
				ls.mConstraintRunning.Notify();
			};

			scheduler->QueueJob(constraint_solver, on_constraint_complete, "ConstraintSolver");
		}

		scheduler->QueueJob(solver, on_complete, "CollisionSolver");

		return async_context(ls);
	}

	void CollisionSolver::AwaitQueryCompletion(Array<vec3>& pForceResults)
	{
		LocalSolver& ls = *LocalSolver::Acquire(&pForceResults);
		AwaitQueryCompletion( ls );
	}

	void CollisionSolver::AwaitQueryCompletion(LocalSolver& pSolver)
	{
		while (pSolver.in_progress())
			pSolver.IdleWatch.wait();

		GlobalSolver::Get().AwaitCompletion();
	}

	void CollisionSolver::BeginBatch()
	{
		GlobalSolver::Get().BeginBatch();
	}

	void CollisionSolver::EndBatch()
	{
		GlobalSolver::Get().EndBatch();
	}
}