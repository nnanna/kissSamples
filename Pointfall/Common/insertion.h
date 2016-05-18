

#ifndef KS_INSERTION_H
#define KS_INSERTION_H

#include <defines.h>
#include <type_traits>

namespace ks
{

	template<typename T> struct ascending_sort_predicate
	{
		inline bool operator()(const T& pA, const T& pB) const	{ return pA > pB; }
	};
	
	template<typename T> struct descending_sort_predicate
	{
		inline bool operator()(const T& pA, const T& pB) const	{ return pA < pB; }
	};

	template<typename T>
	void inline data_shift(T& pData, u32 pDestBegin, u32 pSourceBegin, u32 pLen);

	template<typename T>
	void inline data_shift(typename std::enable_if< std::is_array<T>::value, T>::type & pData, u32 pDestBegin, u32 pSourceBegin, u32 pLen)
	{
		memmove(pData.data() + pDestBegin, pData.data() + pSourceBegin, pLen);
	}

	template<typename T>
	void inline data_shift(typename std::enable_if< !std::is_array<T>::value, T>::type & pData, u32 pDestBegin, u32 pSourceBegin, u32 pLen)
	{
		memmove(pData + pDestBegin, pData + pSourceBegin, pLen);
	}

	template<class TContainer, typename T, class _Pred>
	int binary_insert_sorted(TContainer& pDest, const u32 pTail, const T& pItem, const _Pred& _COND)
	{
		int start = 0;
		int end = pTail;
		int mod2 = end & 1;				// = end % 2
		int mid = (end >> 1) + mod2;
		int found = -1;

		while (found < 0 && mid > start)
		{
			if (_COND(pItem, pDest[mid - 1]))
			{
				if (_COND(pItem, pDest[mid]) == false)
				{
					found = mid;
					break;
				}
				else	// jump forwards
				{
					start = mid;
				}
			}
			else	// jump backwards
			{
				end = mid - 1;
			}

			mid = end - start;
			mod2 = mid & 1;
			mid = start + (mid >> 1) + mod2;	// (mid / 2) + (m % 2)
		}

		if (found < 0)
			found = end;

		if (pTail > u32(found))
			data_shift<TContainer>(pDest, found + 1, found, (pTail - found) * sizeof(T));

		pDest[found] = pItem;

		return found;
	}

	template<class TContainer, class _Pred>
	int binary_find(const TContainer& pDest, const u32 pSize, const _Pred& _COND)
	{
		int start = 0;
		int end = pSize - 1;
		int mod2 = end & 1;
		int mid = (end >> 1) + mod2;
		int found = -1;

		while (found < 0 && mid > start)
		{
			if (_COND(pDest[mid - 1]))
			{
				if (_COND(pDest[mid]) == false)
				{
					found = mid - 1;
					break;
				}
				else
				{
					start = mid;
				}
			}
			else
			{
				end = mid - 1;
			}

			mid = end - start;
			mod2 = mid & 1;
			mid = start + (mid >> 1) + mod2;
		}

		return found;
	}

}


#endif