/*
	Nnanna Kama
	Simple class for registering & fetching class services that are globally used. Singletons basically.
*/

#pragma once


template<class T>
class Service
{
public:
	static void Register( T* pService )		{ mService = pService; }

	static void Destroy()					{ delete mService; }

	static T* Get()							{ return mService; }

private:
	static T*	mService;
};



template<class T>
T*	Service<T>::mService					= nullptr;