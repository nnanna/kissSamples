
//////////////////////////////////////////////////////////////////////////
///
/// Copyright (c)
///	@author		Nnanna Kama
///	@date		11/01/2015
///	@brief		Type storage interface for correct forwarding of anonymous jobs
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

#ifndef ASYNC_SOLVER_H
#define ASYNC_SOLVER_H

#include <defines.h>
#include <Array.h>


struct Constraint;

namespace ks
{
	struct vec3;
	struct LocalSolver;
	class JobGroup;

	struct ConstraintConfig
	{
		Constraint* constraint;
		float*	rPositions;
		float*	rVelocities;
		ksU32	numElements;
	};

	struct async_context
	{
		async_context(LocalSolver& pSolver);
		void SubmitQuery(ksU32 pResultIndex, const vec3& pPos, const vec3& pVel);
		LocalSolver& Data();
	
	private:
		LocalSolver&	mSolver;
	};

	class CollisionSolver
	{
	public:
		static async_context BeginAsync( Array<vec3>& pForceResults, ksU32 pNumElements, float elapsed, const ConstraintConfig* pConstraint = nullptr );
		static void AwaitQueryCompletion( Array<vec3>& pForceResults );
		static void BeginBatch(JobGroup* pJobStream = nullptr);
		static void EndBatch();
	private:
		static void AwaitQueryCompletion(LocalSolver& pSolver);
	};
}

#endif