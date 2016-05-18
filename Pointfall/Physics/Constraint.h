
//////////////////////////////////////////////////////////////////////////
///
/// Copyright (c)
///	@author		Nnanna Kama
///	@date		11/01/2015
///	@brief		
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

#ifndef KS_CONSTRAINT_H
#define KS_CONSTRAINT_H

//namespace ks{

	typedef void(*SatisfyFn)(float* positions, float* velocities, const float* sizes, int count, const struct Constraint& ct);

	enum ConstraintType
	{
		ct_invalid = 0,
		ct_distance,	// clothing
		ct_shape,		// rigid, plastic
		ct_density,		// fluid
		ct_volume,		// inflatables
		ct_contact,		// friction
		ct_custom,
		ct_count
	};

	struct Constraint
	{
		ConstraintType mType;	// constraint lists can easily be sorted by type

		void Satisfy(float* pos3, float* vel3, const float* sizes, int count) const;

		union
		{
			struct
			{
				float restLength;
				float compressionStiffness;
				float stetchStiffness;
			}distance;

			struct
			{
				float restVolume;
				float negVolumeStiffness;
				float posVolumeStiffness;
			}volume;

			struct
			{
				float stiffness;  // strength of constraint defined in [0,1] range
				bool allowStretch;
			}shape;

			struct
			{
				float value;
				bool handleBoundary;
			}density;

			struct
			{
				float restitution;
			}contact;

			struct
			{
				SatisfyFn customSatisfy;
			}custom;
		};

	};

//}



#endif