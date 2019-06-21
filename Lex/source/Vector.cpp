#include "Vector.h"

namespace Donya
{
	Vector2 Vector2::Normalize()
	{
		float length = Length();

		if ( -FLT_EPSILON < length && length < FLT_EPSILON ) { return *this; }
		// else

		x /= length;
		y /= length;

		return *this;
	}
	bool operator == ( const Vector2 &L, const Vector2 &R )
	{
		float diffX = fabsf( L.x - R.x );
		float diffY = fabsf( L.y - R.y );

		if ( FLT_EPSILON <= diffX ) { return false; }
		if ( FLT_EPSILON <= diffY ) { return false; }
		// else
		return true;
	}

	Vector3 Vector3::Normalize()
	{
		float length = Length();

		if ( -FLT_EPSILON < length && length < FLT_EPSILON ) { return *this; }
		// else

		x /= length;
		y /= length;
		z /= length;

		return *this;
	}
	bool operator == ( const Vector3 &L, const Vector3 &R )
	{
		float diffX = fabsf( L.x - R.x );
		float diffY = fabsf( L.y - R.y );
		float diffZ = fabsf( L.z - R.z );

		if ( FLT_EPSILON <= diffX ) { return false; }
		if ( FLT_EPSILON <= diffY ) { return false; }
		if ( FLT_EPSILON <= diffZ ) { return false; }
		// else
		return true;
	}

	bool operator == ( const Vector4 &L, const Vector4 &R )
	{
		float diffX = fabsf( L.x - R.x );
		float diffY = fabsf( L.y - R.y );
		float diffZ = fabsf( L.z - R.z );
		float diffW = fabsf( L.w - R.w );

		if ( FLT_EPSILON <= diffX ) { return false; }
		if ( FLT_EPSILON <= diffY ) { return false; }
		if ( FLT_EPSILON <= diffZ ) { return false; }
		if ( FLT_EPSILON <= diffW ) { return false; }
		// else
		return true;
	}
}