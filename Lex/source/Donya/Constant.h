#pragma once

#define scast		static_cast
#define DEBUG_MODE	( defined( DEBUG ) || defined( _DEBUG ) )

#define DELETE_COPY_AND_ASSIGN( Typename ) \
	Typename( const Typename & ) = delete; \
	const Typename &operator = ( const Typename & ) = delete;

#if DEBUG_MODE

// I referred to : https://hwada.hatenablog.com/entry/20080304/1204609100

#define _CRTDBG_MAP_ALLOC	// Specify to output the cpp file-name and line-number when detect memory-leak by malloc.
#include <crtdbg.h>
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )	// Specify to output the cpp file-name and line-number when detect memory-leak by new.

#else

#define DEBUG_NEW new

#endif // DEBUG_MODE
