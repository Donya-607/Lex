#include "Quaternion.h"

#include <array>

#include "Useful.h"	// Using Donya::Equal()

using namespace Donya;
using namespace DirectX;

namespace Donya
{
	Quaternion::Quaternion() : x( 0.0f ), y( 0.0f ), z( 0.0f ), w( 1.0f )
	{

	}
	Quaternion::Quaternion( float x, float y, float z, float w ) : x( x ), y( y ), z( z ), w( w )
	{

	}

#pragma region Arithmetic

	Quaternion &Quaternion::operator += ( const Quaternion &R )
	{
		x += R.x;
		y += R.y;
		z += R.z;
		w += R.w;

		return *this;
	}

	Quaternion &Quaternion::operator -= ( const Quaternion &R )
	{
		x -= R.x;
		y -= R.y;
		z -= R.z;
		w -= R.w;

		return *this;
	}

	Quaternion &Quaternion::operator *= ( float scalar )
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		w *= scalar;

		return *this;
	}
	Quaternion &Quaternion::operator *= ( const Quaternion &R )
	{
		float xx = ( w * R.x ) + ( x * R.w ) + ( y * R.z ) - ( z * R.y );
		float yy = ( w * R.y ) - ( x * R.z ) + ( y * R.w ) + ( z * R.x );
		float zz = ( w * R.z ) + ( x * R.y ) - ( y * R.x ) + ( z * R.w );
		float ww = ( w * R.w ) - ( x * R.x ) - ( y * R.y ) - ( z * R.z );

		x = xx;
		y = yy;
		z = zz;
		w = ww;

		return *this;
	}
	Quaternion &Quaternion::operator *= ( const Vector3 &R )
	{
		float xx = ( w * R.x ) + ( y * R.z ) - ( z * R.y );
		float yy = ( w * R.y ) - ( x * R.z ) + ( z * R.x );
		float zz = ( w * R.z ) + ( x * R.y ) - ( y * R.x );
		float ww = (-x * R.x ) - ( y * R.y ) - ( z * R.z );

		x = xx;
		y = yy;
		z = zz;
		w = ww;

		return *this;
	}

	Quaternion &Quaternion::operator /= ( float scalar )
	{
		float invScalar = 1.0f / scalar;
		return ( *this *= invScalar );
	}

// region Arithmetic
#pragma endregion

	float Quaternion::Length() const
	{
		return sqrtf( LengthSq() );
	}
	float Quaternion::LengthSq() const
	{
		return ( x * x ) + ( y * y ) + ( z * z ) + ( w * w );
	}

	Quaternion Quaternion::Conjugate() const
	{
		return Quaternion{ -x, -y, -z, w };
	}

	Quaternion Quaternion::Inverse() const
	{
		float norm = Length();

		if ( Equal( norm, 1.0f ) )
		{
			return Conjugate();
		}
		// else

		return ( Conjugate() / ( norm * norm ) );
	}

	Vector3 Quaternion::GetAxis() const
	{
		return Vector3{ x, y, z };
	}

	Vector3 Quaternion::RotateVector( const Donya::Vector3 &V ) const
	{
		// Q * V * Q*
		Quaternion rotated = ( ( *this ) * V ) * Conjugate();

		return rotated.GetAxis();
	}

	XMFLOAT4X4 Quaternion::RequireRotationMatrix() const
	{
		XMFLOAT4X4 m{};

		m._11 = 1.0f -	( 2.0f * y * y ) - ( 2.0f * z * z );
		m._12 =			( 2.0f * x * y ) + ( 2.0f * w * z );
		m._13 =			( 2.0f * x * z ) - ( 2.0f * w * y );
		m._14 = 0.0f;

		m._21 =			( 2.0f * x * y ) - ( 2.0f * w * z );
		m._22 = 1.0f -	( 2.0f * x * x ) - ( 2.0f * z * z );
		m._23 =			( 2.0f * y * z ) + ( 2.0f * w * x );
		m._24 = 0.0f;

		m._31 =			( 2.0f * x * z ) + ( 2.0f * w * y );
		m._32 =			( 2.0f * y * z ) - ( 2.0f * w * x );
		m._33 = 1.0f -	( 2.0f * x * x ) - ( 2.0f * y * y );
		m._34 = 0.0f;

		m._41 = 0.0f;
		m._42 = 0.0f;
		m._43 = 0.0f;
		m._44 = 1.0f;

		return m;
	}

	float Quaternion::Dot( const Quaternion &L, const Quaternion &R )
	{
		return ( L.x * R.x ) + ( L.y * R.y ) + ( L.z * R.z ) + ( L.w * R.w );
	}

	float Quaternion::Length( const Quaternion &Q )
	{
		return Q.Length();
	}
	float Quaternion::LengthSq( const Quaternion &Q )
	{
		return Q.LengthSq();
	}

	Quaternion Quaternion::Conjugate( const Quaternion &Q )
	{
		return Q.Conjugate();
	}

	Quaternion Quaternion::Make( const Vector3 nAxis, float radTheta )
	{
		float sin = sinf( radTheta * 0.5f );
		float cos = cosf( radTheta * 0.5f );

		return Quaternion
		{
			sin * nAxis.x,
			sin * nAxis.y,
			sin * nAxis.z,
			cos
		};
	}

	Quaternion Quaternion::Make( const XMFLOAT4X4 &M )
	{
		// see http://marupeke296.com/DXG_No58_RotQuaternionTrans.html

		std::array<float, 4> elements{};
		elements[0] = M._11 - M._22 - M._33 + 1.0f;
		elements[1] = M._11 + M._22 - M._33 + 1.0f;
		elements[2] = M._11 - M._22 + M._33 + 1.0f;
		elements[3] = M._11 + M._22 + M._33 + 1.0f;

		size_t biggestIndex = 0;
		for ( size_t i = 1; i < elements.size(); ++i )
		{
			if ( elements[biggestIndex] < elements[i] )
			{
				biggestIndex = i;
			}
		}

		// Prevent zero-divide.
		if ( elements[biggestIndex] <= 0.0f ) { return Quaternion::Identity(); }
		// else

		Quaternion rv{};
		std::array<float *, 4> Q{ &rv.x, &rv.y, &rv.z, &rv.w };

		float component = sqrtf( elements[biggestIndex] ) * 0.5f;
		float extract = 0.25f / component;

		enum Axis { X = 0, Y = 1, Z = 2, W = 3 };
		switch ( biggestIndex )
		{
		case X:
			*( Q[0] ) = component;
			*( Q[1] ) = ( M._12 + M._21 ) * extract;
			*( Q[2] ) = ( M._31 + M._13 ) * extract;
			*( Q[3] ) = ( M._23 - M._32 ) * extract;
			break;
		case Y:
			*( Q[0] ) = ( M._12 + M._21 ) * extract;
			*( Q[1] ) = component;
			*( Q[2] ) = ( M._23 + M._32 ) * extract;
			*( Q[3] ) = ( M._31 - M._13 ) * extract;
			break;
		case Z:
			*( Q[0] ) = ( M._31 + M._13 ) * extract;
			*( Q[1] ) = ( M._23 + M._32 ) * extract;
			*( Q[2] ) = component;
			*( Q[3] ) = ( M._12 - M._21 ) * extract;
			break;
		case W:
			*( Q[0] ) = ( M._23 - M._32 ) * extract;
			*( Q[1] ) = ( M._31 - M._13 ) * extract;
			*( Q[2] ) = ( M._12 - M._21 ) * extract;
			*( Q[3] ) = component;
			break;
		}

		return rv;
	}

	Quaternion Quaternion::Identity()
	{
		return Quaternion{ 0.0f, 0.0f, 0.0f, 1.0f };
	}

	Quaternion Quaternion::Inverse( const Quaternion &Q )
	{
		return Q.Inverse();
	}

	Quaternion Quaternion::Slerp( const Quaternion &nBegin, const Quaternion &nEnd, float time )
	{
		float dot	= Dot( nBegin, nEnd );
		float theta	= acosf( dot );

		float sin	= sinf( theta );
		if ( fabsf( sin ) < FLT_EPSILON )
		{
			// In this case, "begin" and "end" is parallel, so calculation is unnecessary.
			return nBegin;
		}
		// else

		// TODO:I should be conside case when "theta" is negative.

		float percentBegin		= sinf( theta * ( 1.0f - time ) );
		float percentEnd		= sinf( theta * time );

		Quaternion multedBegin	= ( nBegin	* percentBegin	) / sin;
		Quaternion multedEnd	= ( nEnd	* percentEnd	) / sin;

		return ( multedBegin + multedEnd );
	}

	Vector3 Quaternion::GetAxis( const Quaternion &Q )
	{
		return Q.GetAxis();
	}

	Vector3 Quaternion::RotateVector( const Quaternion &R, const Donya::Vector3 &target )
	{
		return R.RotateVector( target );
	}

	XMFLOAT4X4 Quaternion::RequireRotationMatrix( const Quaternion &Q )
	{
		return Q.RequireRotationMatrix();
	}

	Quaternion operator * ( const Vector3 &L, const Quaternion &R )
	{
		float xx = ( L.x * R.w ) + ( L.y * R.z ) - ( L.z * R.y );
		float yy = (-L.x * R.z ) + ( L.y * R.w ) + ( L.z * R.x );
		float zz = ( L.x * R.y ) - ( L.y * R.x ) + ( L.z * R.w );
		float ww = (-L.x * R.x ) - ( L.y * R.y ) - ( L.z * R.z );

		return Quaternion{ xx, yy, zz, ww };
	}

	bool operator == ( const Quaternion &L, const Quaternion &R )
	{
		if ( !Equal( L.x, R.x ) ) { return false; }
		if ( !Equal( L.y, R.y ) ) { return false; }
		if ( !Equal( L.z, R.z ) ) { return false; }
		if ( !Equal( L.w, R.w ) ) { return false; }
		// else
		return true;
	}
}