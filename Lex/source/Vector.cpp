#include "Vector.h"

#include "Common.h"
#include "Useful.h" // Use ToDegree().

namespace Donya
{
	Vector2 Vector2::Normalize()
	{
		float length = Length();

		if ( ZeroEqual( length ) ) { return *this; }
		// else

		x /= length;
		y /= length;

		return *this;
	}
	float Vector2::Radian() const
	{
		return atan2f( y, x );
	}
	float Vector2::Degree() const
	{
		return ToDegree( Radian() );
	}
	bool Vector2::IsZero() const
	{
		return ( ZeroEqual( LengthSq() ) ) ? true : false;
	}
	bool operator == ( const Vector2 &L, const Vector2 &R )
	{
		return Vector2{ L - R }.IsZero();
	}

	Vector3 Vector3::Normalize()
	{
		float length = Length();

		if ( ZeroEqual( length ) ) { return *this; }
		// else

		x /= length;
		y /= length;
		z /= length;

		return *this;
	}
	bool Vector3::IsZero() const
	{
		return ( ZeroEqual( LengthSq() ) ) ? true : false;
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