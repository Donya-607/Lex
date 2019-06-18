#ifndef INCLUDED_VECTOR_H_
#define INCLUDED_VECTOR_H_

#include <DirectXMath.h>

namespace Donya
{

	#pragma region Vector3

	struct Vector3 : public DirectX::XMFLOAT3
	{
	public:
		Vector3() : XMFLOAT3() {}
		Vector3( float x = 0, float y = 0, float z = 0 ) : XMFLOAT3( x, y, z ) {}
		Vector3( const XMFLOAT3 &ref ) : XMFLOAT3( ref ) {}
		Vector3( const XMFLOAT3&&ref ) : XMFLOAT3( ref ) {}
		Vector3( const Vector3  &ref ) : XMFLOAT3( ref ) {}
		Vector3( const Vector3 &&ref ) : XMFLOAT3( ref ) {}
		Vector3 & operator = ( const XMFLOAT3 &ref ) { *this = ref; return *this; }
		Vector3 & operator = ( const XMFLOAT3&&ref ) { *this = ref; return *this; }
		Vector3 & operator = ( const Vector3  &ref ) { *this = ref; return *this; }
		Vector3 & operator = ( const Vector3 &&ref ) { *this = ref; return *this; }
		~Vector3() = default;
	public:
		inline Vector3 operator - () const { return Vector3{ -x, -y, -z }; }
		inline Vector3 operator += ( float scalar )
		{
			x += scalar;
			y += scalar;
			z += scalar;
			return *this;
		}
		inline Vector3 operator += ( const Vector3  &R )
		{
			x += R.x;
			y += R.y;
			z += R.z;
			return *this;
		}
		inline Vector3 operator += ( const XMFLOAT3 &R )
		{
			x += R.x;
			y += R.y;
			z += R.z;
			return *this;
		}
		inline Vector3 operator -= ( float scalar )
		{
			x -= scalar;
			y -= scalar;
			z -= scalar;
			return *this;
		}
		inline Vector3 operator -= ( const Vector3  &R )
		{
			x -= R.x;
			y -= R.y;
			z -= R.z;
			return *this;
		}
		inline Vector3 operator -= ( const XMFLOAT3 &R )
		{
			x -= R.x;
			y -= R.y;
			z -= R.z;
			return *this;
		}
		inline Vector3 operator *= ( float scalar )
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			return *this;
		}
		inline Vector3 operator /= ( float scalar )
		{
			x /= scalar;
			y /= scalar;
			z /= scalar;
			return *this;
		}
	public:
		// using sqrtf().
		inline float Length()   const { return sqrtf( LengthSq() ); }
		inline float LengthSq() const { return ( x * x ) + ( y * y ) + ( z * z ); }
		inline Vector3 Normalize()
		{
			float length = Length();

			if ( -FLT_EPSILON < length && length < FLT_EPSILON ) { return *this; }
			// else

			x /= length;
			y /= length;
			z /= length;

			return *this;
		}
	public:
		inline float Dot( const Vector3  &R ) const
		{
			return ( x * R.x ) + ( y * R.y ) + ( z * R.z );
		}
		inline float Dot( const XMFLOAT3 &R ) const
		{
			return ( x * R.x ) + ( y * R.y ) + ( z * R.z );
		}
		inline Vector3 Cross( const Vector3  &R ) const
		{
			return Vector3
			{
				( y * R.z ) - ( z * R.y ),
				( z * R.x ) - ( x * R.z ),
				( x * R.y ) - ( y * R.z )
			};
		}
		inline Vector3 Cross( const XMFLOAT3 &R ) const
		{
			return Vector3
			{
				( y * R.z ) - ( z * R.y ),
				( z * R.x ) - ( x * R.z ),
				( x * R.y ) - ( y * R.z )
			};
		}
	public:
		inline static float Dot( const Vector3  &L, const Vector3  &R )		{ return L.Dot( R ); }
		inline static float Dot( const XMFLOAT3 &L, const XMFLOAT3 &R )		{ return Vector3( L ).Dot( R ); }
		inline static Vector3 Cross( const Vector3  &L, const Vector3  &R )	{ return L.Cross( R ); }
		inline static Vector3 Cross( const XMFLOAT3 &L, const XMFLOAT3 &R )	{ return Vector3( L ).Cross( R ); }
	};

	inline Vector3 operator + ( const Vector3 &L, float scalar )		{ return ( Vector3( L ) += scalar ); }
	inline Vector3 operator - ( const Vector3 &L, float scalar )		{ return ( Vector3( L ) -= scalar ); }
	inline Vector3 operator + ( const Vector3 &L, const Vector3 &R )	{ return ( Vector3( L ) += R ); }
	inline Vector3 operator - ( const Vector3 &L, const Vector3 &R )	{ return ( Vector3( L ) -= R ); }
	inline Vector3 operator * ( const Vector3 &L, float scalar )		{ return ( Vector3( L ) *= scalar ); }
	inline Vector3 operator * ( float scalar, const Vector3 &R )		{ return ( Vector3( R ) *= scalar ); }
	inline Vector3 operator / ( const Vector3 &L, float scalar )		{ return ( Vector3( L ) /= scalar ); }
	inline float operator * ( const Vector3 &L, const Vector3 &R )		{ return Vector3::Dot( L, R ); }

	inline bool operator == ( const Vector3 &L, const Vector3 &R )
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
	inline bool operator != ( const Vector3 &L, const Vector3 &R ) { return !( L == R ); }

	#pragma endregion

}

#endif // !INCLUDED_VECTOR_H_
