#ifndef INCLUDED_COMMON_H_
#define INCLUDED_COMMON_H_

#include	<assert.h>

#define		scast				static_cast
constexpr	float PI		=	3.14159265359f;
constexpr	float EPSILON	=	1.192092896e-07F;	// smallest such that 1.0+FLT_EPSILON != 1.0

#define		ArraySize( x )		( sizeof( x ) / ( sizeof( x[0] ) ) )
#define		ToRadian( x )		( x *  0.01745f )	// ( PI / 180 )
#define		ToDegree( x )		( x * 57.29577f )	// ( 180 / PI )
#define		ZeroEqual( x )		( -EPSILON < x && x < EPSILON )

#define		DEBUG_MODE			( defined( DEBUG ) | defined( _DEBUG ) )

namespace Common
{
	int		ScreenWidth();
	float	ScreenWidthF();
	long	ScreenWidthL();

	int		ScreenHeight();
	float	ScreenHeightF();
	long	ScreenHeightL();

	int		HalfScreenWidth();
	float	HalfScreenWidthF();
	long	HalfScreenWidthL();

	int		HalfScreenHeight();
	float	HalfScreenHeightF();
	long	HalfScreenHeightL();
}

#endif //INCLUDED_COMMON_H_
