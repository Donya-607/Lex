#ifndef INCLUDED_COMMON_H_
#define INCLUDED_COMMON_H_

#define   scast				static_cast
#define   DEBUG_MODE		( defined( DEBUG ) | defined( _DEBUG ) )

constexpr float PI		=	3.14159265359f;
constexpr float EPSILON	=	1.192092896e-07F;	// smallest such that 1.0+FLT_EPSILON != 1.0

template<typename T, size_t size>
constexpr size_t ArraySize( const T ( & )[size] ) { return size; }

constexpr float ToRadian( float degree	) { return degree *  0.01745f/* PI / 180 */;	}
constexpr float ToDegree( float radian	) { return radian * 57.29577f/* 180 / PI */;	}

constexpr bool ZeroEqual( float x		) { return ( -EPSILON < x && x < EPSILON );		}

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
