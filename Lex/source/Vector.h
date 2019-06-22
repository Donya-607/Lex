#ifndef INCLUDED_VECTOR_H_
#define INCLUDED_VECTOR_H_

#include <DirectXMath.h>

namespace Donya
{

#pragma region Vector2

	struct Vector2 : public DirectX::XMFLOAT2
	{
	public:
		Vector2() : XMFLOAT2() {}
		Vector2( float x = 0, float y = 0 ) : XMFLOAT2( x, y ) {}
		Vector2( const XMFLOAT2 &ref ) : XMFLOAT2( ref ) {}
		Vector2( const XMFLOAT2&&ref ) : XMFLOAT2( ref ) {}
		Vector2( const Vector2  &ref ) : XMFLOAT2( ref ) {}
		Vector2( const Vector2 &&ref ) noexcept : XMFLOAT2( ref ) {}
		Vector2 & operator = ( const XMFLOAT2 &ref ) { *this = ref; return *this; }
		Vector2 & operator = ( const XMFLOAT2&&ref ) { *this = ref; return *this; }
		Vector2 & operator = ( const Vector2  &ref ) { *this = ref; return *this; }
		Vector2 & operator = ( const Vector2 &&ref ) noexcept { *this = ref; return *this; }
		~Vector2() = default;
	public:
		inline Vector2 operator - () const { return Vector2{ -x, -y }; }
		inline Vector2 operator += ( float scalar )
		{
			x += scalar;
			y += scalar;
			return *this;
		}
		inline Vector2 operator += ( const Vector2  &R )
		{
			x += R.x;
			y += R.y;
			return *this;
		}
		inline Vector2 operator += ( const XMFLOAT2 &R )
		{
			x += R.x;
			y += R.y;
			return *this;
		}
		inline Vector2 operator -= ( float scalar )
		{
			x -= scalar;
			y -= scalar;
			return *this;
		}
		inline Vector2 operator -= ( const Vector2  &R )
		{
			x -= R.x;
			y -= R.y;
			return *this;
		}
		inline Vector2 operator -= ( const XMFLOAT2 &R )
		{
			x -= R.x;
			y -= R.y;
			return *this;
		}
		inline Vector2 operator *= ( float scalar )
		{
			x *= scalar;
			y *= scalar;
			return *this;
		}
		inline Vector2 operator /= ( float scalar )
		{
			x /= scalar;
			y /= scalar;
			return *this;
		}
	public:
		// using sqrtf().
		inline float Length()   const { return sqrtf( LengthSq() ); }
		inline float LengthSq() const { return ( x * x ) + ( y * y ); }
		Vector2 Normalize();
	public:
		inline float Dot( const Vector2  &R ) const
		{
			return ( x * R.x ) + ( y * R.y );
		}
		inline float Dot( const XMFLOAT2 &R ) const
		{
			return ( x * R.x ) + ( y * R.y );
		}
		inline float Cross( const Vector2  &R ) const
		{
			return ( x * R.y ) - ( y * R.x );
		}
		inline float Cross( const XMFLOAT2 &R ) const
		{
			return ( x * R.y ) - ( y * R.x );
		}
	public:
		inline static float Dot( const Vector2  &L, const Vector2  &R ) { return L.Dot( R ); }
		inline static float Dot( const XMFLOAT2 &L, const XMFLOAT2 &R ) { return Vector2( L ).Dot( R ); }
		inline static float Cross( const Vector2  &L, const Vector2  &R ) { return L.Cross( R ); }
		inline static float Cross( const XMFLOAT2 &L, const XMFLOAT2 &R ) { return Vector2( L ).Cross( R ); }
	};

	inline Vector2 operator + ( const Vector2 &L, float scalar )		{ return ( Vector2( L ) += scalar ); }
	inline Vector2 operator - ( const Vector2 &L, float scalar )		{ return ( Vector2( L ) -= scalar ); }
	inline Vector2 operator + ( const Vector2 &L, const Vector2 &R )	{ return ( Vector2( L ) += R ); }
	inline Vector2 operator - ( const Vector2 &L, const Vector2 &R )	{ return ( Vector2( L ) -= R ); }
	inline Vector2 operator * ( const Vector2 &L, float scalar )		{ return ( Vector2( L ) *= scalar ); }
	inline Vector2 operator * ( float scalar, const Vector2 &R )		{ return ( Vector2( R ) *= scalar ); }
	inline Vector2 operator / ( const Vector2 &L, float scalar )		{ return ( Vector2( L ) /= scalar ); }

	bool		operator == ( const Vector2 &L, const Vector2 &R );
	inline bool	operator != ( const Vector2 &L, const Vector2 &R )		{ return !( L == R ); }

#pragma endregion

#pragma region Vector3

	struct Vector3 : public DirectX::XMFLOAT3
	{
	public:
		Vector3() : XMFLOAT3() {}
		Vector3( float x = 0, float y = 0, float z = 0 ) : XMFLOAT3( x, y, z ) {}
		Vector3( const XMFLOAT3 &ref ) : XMFLOAT3( ref ) {}
		Vector3( const XMFLOAT3&&ref ) : XMFLOAT3( ref ) {}
		Vector3( const Vector3  &ref ) : XMFLOAT3( ref ) {}
		Vector3( const Vector3 &&ref ) noexcept : XMFLOAT3( ref ) {}
		Vector3 & operator = ( const XMFLOAT3 &ref ) { *this = ref; return *this; }
		Vector3 & operator = ( const XMFLOAT3&&ref ) { *this = ref; return *this; }
		Vector3 & operator = ( const Vector3  &ref ) { *this = ref; return *this; }
		Vector3 & operator = ( const Vector3 &&ref ) noexcept { *this = ref; return *this; }
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
		Vector3 Normalize();
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

	bool		operator == ( const Vector3 &L, const Vector3 &R );
	inline bool operator != ( const Vector3 &L, const Vector3 &R )		{ return !( L == R ); }

#pragma endregion

#pragma region Vector4

	struct Vector4 : public DirectX::XMFLOAT4
	{
	public:
		Vector4() : XMFLOAT4() {}
		Vector4( float x = 0, float y = 0, float z = 0, float w = 0 ) : XMFLOAT4( x, y, z, w ) {}
		Vector4( const XMFLOAT4 &ref ) : XMFLOAT4( ref ) {}
		Vector4( const XMFLOAT4&&ref ) : XMFLOAT4( ref ) {}
		Vector4( const Vector4  &ref ) : XMFLOAT4( ref ) {}
		Vector4( const Vector4 &&ref ) noexcept : XMFLOAT4( ref ) {}
		Vector4 & operator = ( const XMFLOAT4 &ref ) { *this = ref; return *this; }
		Vector4 & operator = ( const XMFLOAT4&&ref ) { *this = ref; return *this; }
		Vector4 & operator = ( const Vector4  &ref ) { *this = ref; return *this; }
		Vector4 & operator = ( const Vector4 &&ref ) noexcept { *this = ref; return *this; }
		~Vector4() = default;
	public:
		inline Vector4 operator - () const { return Vector4{ -x, -y, -z }; }
		inline Vector4 operator += ( float scalar )
		{
			x += scalar;
			y += scalar;
			z += scalar;
			w += scalar;
			return *this;
		}
		inline Vector4 operator += ( const Vector4  &R )
		{
			x += R.x;
			y += R.y;
			z += R.z;
			w += R.w;
			return *this;
		}
		inline Vector4 operator += ( const XMFLOAT4 &R )
		{
			x += R.x;
			y += R.y;
			z += R.z;
			w += R.w;
			return *this;
		}
		inline Vector4 operator -= ( float scalar )
		{
			x -= scalar;
			y -= scalar;
			z -= scalar;
			w -= scalar;
			return *this;
		}
		inline Vector4 operator -= ( const Vector4  &R )
		{
			x -= R.x;
			y -= R.y;
			z -= R.z;
			w -= R.w;
			return *this;
		}
		inline Vector4 operator -= ( const XMFLOAT4 &R )
		{
			x -= R.x;
			y -= R.y;
			z -= R.z;
			w -= R.w;
			return *this;
		}
		inline Vector4 operator *= ( float scalar )
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			w *= scalar;
			return *this;
		}
		inline Vector4 operator /= ( float scalar )
		{
			x /= scalar;
			y /= scalar;
			z /= scalar;
			w /= scalar;
			return *this;
		}
	public:
//		I don't understanding the XMFLOAT4, so not implemented.
//		// using sqrtf().
//		inline float Length()   const {}
//		inline float LengthSq() const {}
//		Vector4 Normalize();
//	public:
//		inline float Dot( const Vector4  &R ) const
//		{
//			
//		}
//		inline float Dot( const XMFLOAT4 &R ) const
//		{
//			
//		}
//		inline Vector4 Cross( const Vector4  &R ) const
//		{
//			
//		}
//		inline Vector4 Cross( const XMFLOAT4 &R ) const
//		{
//			
//		}
//	public:
//		inline static float Dot( const Vector4  &L, const Vector4  &R ) { return L.Dot( R ); }
//		inline static float Dot( const XMFLOAT4 &L, const XMFLOAT4 &R ) { return Vector4( L ).Dot( R ); }
//		inline static Vector4 Cross( const Vector4  &L, const Vector4  &R ) { return L.Cross( R ); }
//		inline static Vector4 Cross( const XMFLOAT4 &L, const XMFLOAT4 &R ) { return Vector4( L ).Cross( R ); }
	};

	inline Vector4 operator + ( const Vector4 &L, float scalar )		{ return ( Vector4( L ) += scalar ); }
	inline Vector4 operator - ( const Vector4 &L, float scalar )		{ return ( Vector4( L ) -= scalar ); }
	inline Vector4 operator + ( const Vector4 &L, const Vector4 &R )	{ return ( Vector4( L ) += R ); }
	inline Vector4 operator - ( const Vector4 &L, const Vector4 &R )	{ return ( Vector4( L ) -= R ); }
	inline Vector4 operator * ( const Vector4 &L, float scalar )		{ return ( Vector4( L ) *= scalar ); }
	inline Vector4 operator * ( float scalar, const Vector4 &R )		{ return ( Vector4( R ) *= scalar ); }
	inline Vector4 operator / ( const Vector4 &L, float scalar )		{ return ( Vector4( L ) /= scalar ); }

	bool		operator == ( const Vector4 &L, const Vector4 &R );
	inline bool operator != ( const Vector4 &L, const Vector4 &R )		{ return !( L == R ); }

#pragma endregion

}

#endif // !INCLUDED_VECTOR_H_
