/*
	Nnanna Kama
	An attempt to organise all the macros used in this code
	into one giant pile of s...
*/


#pragma once


#ifndef VOID_RETURN_IF_NULL
#define VOID_RETURN_IF_NULL(x)	{ if( (x)==0 ) return; }
#endif


#ifndef BOOL_RETURN_IF_NULL
#define BOOL_RETURN_IF_NULL(x)	{ if( (x)==0 ) return false; }
#endif


#ifndef SAFE_DELETE
#define SAFE_DELETE(x)	{ if( (x)!=0 ) {delete (x); (x)=0;} }
#endif


#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x) { if( (x)!=0 ) { delete [] (x); x=0; }	}
#endif
