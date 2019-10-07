#pragma once

#include <cstdint> // use for std::uint32_t
#include <DirectXMath.h>

#include "cereal/cereal.hpp"

namespace Donya
{

#pragma region Vector2

	struct Vector2 : public DirectX::XMFLOAT2
	{
	public:
		Vector2() : XMFLOAT2() {}
		Vector2( float x, float y ) : XMFLOAT2( x, y ) {}
		Vector2( const XMFLOAT2 &ref  ) : XMFLOAT2( ref ) {}
		Vector2( const XMFLOAT2 &&ref ) : XMFLOAT2( ref ) {}
		Vector2( const Vector2  &ref  ) : XMFLOAT2( ref ) {}
		Vector2( const Vector2  &&ref ) noexcept : XMFLOAT2( ref ) {}
		Vector2 &operator = ( float scalar ) noexcept { x = y = scalar; return *this; }
		Vector2 &operator = ( const XMFLOAT2 &ref  ) noexcept { x = ref.x; y = ref.y; return *this; }
		Vector2 &operator = ( const XMFLOAT2 &&ref ) noexcept { x = ref.x; y = ref.y; return *this; }
		Vector2 &operator = ( const Vector2 &ref   ) noexcept { x = ref.x; y = ref.y; return *this; }
		Vector2 &operator = ( const Vector2 &&ref  ) noexcept { x = ref.x; y = ref.y; return *this; }
		~Vector2() = default;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( x ), CEREAL_NVP( y ) );
		}
	public:
		Vector2 operator - () const { return Vector2{ -x, -y }; }
		Vector2 operator += ( float scalar )
		{
			x += scalar;
			y += scalar;
			return *this;
		}
		Vector2 operator += ( const Vector2 &R )
		{
			x += R.x;
			y += R.y;
			return *this;
		}
		Vector2 operator += ( const XMFLOAT2 &R )
		{
			x += R.x;
			y += R.y;
			return *this;
		}
		Vector2 operator -= ( float scalar )
		{
			x -= scalar;
			y -= scalar;
			return *this;
		}
		Vector2 operator -= ( const Vector2 &R )
		{
			x -= R.x;
			y -= R.y;
			return *this;
		}
		Vector2 operator -= ( const XMFLOAT2 &R )
		{
			x -= R.x;
			y -= R.y;
			return *this;
		}
		Vector2 operator *= ( float scalar )
		{
			x *= scalar;
			y *= scalar;
			return *this;
		}
		Vector2 operator /= ( float scalar )
		{
			x /= scalar;
			y /= scalar;
			return *this;
		}
	public:
		// using sqrtf().
		float Length()   const { return sqrtf( LengthSq() ); }
		float LengthSq() const { return ( x * x ) + ( y * y ); }
		Vector2 Normalize();

		/// <summary>
		/// Returns [-pi ~ +pi].
		/// </summary>
		float Radian() const;
		/// <summary>
		/// Returns [-180.0f ~ +180.0f].
		/// </summary>
		float Degree() const;

		/// <summary>
		/// Is Zero-vector?
		/// </summary>
		bool IsZero() const;
	public:
		float Dot( const Vector2  &R ) const
		{
			return ( x * R.x ) + ( y * R.y );
		}
		float Dot( const XMFLOAT2 &R ) const
		{
			return ( x * R.x ) + ( y * R.y );
		}
		float Cross( const Vector2  &R ) const
		{
			return ( x * R.y ) - ( y * R.x );
		}
		float Cross( const XMFLOAT2 &R ) const
		{
			return ( x * R.y ) - ( y * R.x );
		}
	public:
		static float Dot( const Vector2  &L, const Vector2  &R ) { return L.Dot( R ); }
		static float Dot( const XMFLOAT2 &L, const XMFLOAT2 &R ) { return Vector2( L ).Dot( R ); }
		static float Cross( const Vector2  &L, const Vector2  &R ) { return L.Cross( R ); }
		static float Cross( const XMFLOAT2 &L, const XMFLOAT2 &R ) { return Vector2( L ).Cross( R ); }
		static Vector2 Right()	{ return Vector2{ 1.0f, 0.0f }; }
		static Vector2 Up()		{ return Vector2{ 0.0f, 1.0f }; }
		static Vector2 Zero()	{ return Vector2{ 0.0f, 0.0f }; }
	};

	static Vector2	operator + ( const Vector2 &L, float scalar )					{ return ( Vector2( L ) += scalar ); }
	static Vector2	operator - ( const Vector2 &L, float scalar )					{ return ( Vector2( L ) -= scalar ); }
	static Vector2	operator + ( const Vector2 &L, const Vector2 &R )				{ return ( Vector2( L ) += R ); }
	static Vector2	operator - ( const Vector2 &L, const Vector2 &R )				{ return ( Vector2( L ) -= R ); }
	static Vector2	operator * ( const Vector2 &L, float scalar )					{ return ( Vector2( L ) *= scalar ); }
	static Vector2	operator * ( float scalar, const Vector2 &R )					{ return ( Vector2( R ) *= scalar ); }
	static Vector2	operator / ( const Vector2 &L, float scalar )					{ return ( Vector2( L ) /= scalar ); }

	bool			operator == ( const Vector2 &L, const Vector2 &R );
	static bool		operator != ( const Vector2 &L, const Vector2 &R )				{ return !( L == R ); }

	static float	Dot( const Vector2 &L, const Vector2 &R )						{ return L.Dot( R ); }
	static float	Dot( const DirectX::XMFLOAT2 &L, const DirectX::XMFLOAT2 &R )	{ return Vector2( L ).Dot( R ); }
	static float	Cross( const Vector2 &L, const Vector2 &R )						{ return L.Cross( R ); }
	static float	Cross( const DirectX::XMFLOAT2 &L, const DirectX::XMFLOAT2 &R )	{ return Vector2( L ).Cross( R ); }

#pragma endregion

#pragma region Vector3

	struct Vector3 : public DirectX::XMFLOAT3
	{
	public:
		Vector3() : XMFLOAT3() {}
		Vector3( float x, float y, float z ) : XMFLOAT3( x, y, z ) {}
		Vector3( const XMFLOAT3 &ref  ) : XMFLOAT3( ref ) {}
		Vector3( const XMFLOAT3 &&ref ) : XMFLOAT3( ref ) {}
		Vector3( const Vector3  &ref  ) : XMFLOAT3( ref ) {}
		Vector3( const Vector3  &&ref ) noexcept : XMFLOAT3( ref ) {}
		Vector3 &operator = ( float scalar ) noexcept { x = y = z = scalar; return *this; }
		Vector3 &operator = ( const XMFLOAT3 &ref  ) noexcept { x = ref.x; y = ref.y; z = ref.z; return *this; }
		Vector3 &operator = ( const XMFLOAT3 &&ref ) noexcept { x = ref.x; y = ref.y; z = ref.z; return *this; }
		Vector3 &operator = ( const Vector3  &ref  ) noexcept { x = ref.x; y = ref.y; z = ref.z; return *this; }
		Vector3 &operator = ( const Vector3  &&ref ) noexcept { x = ref.x; y = ref.y; z = ref.z; return *this; }
		~Vector3() = default;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( x ), CEREAL_NVP( y ), CEREAL_NVP( z ) );
		}
	public:
		Vector3 operator - () const { return Vector3{ -x, -y, -z }; }
		Vector3 operator += ( float scalar )
		{
			x += scalar;
			y += scalar;
			z += scalar;
			return *this;
		}
		Vector3 operator += ( const Vector3 &R )
		{
			x += R.x;
			y += R.y;
			z += R.z;
			return *this;
		}
		Vector3 operator += ( const XMFLOAT3 &R )
		{
			x += R.x;
			y += R.y;
			z += R.z;
			return *this;
		}
		Vector3 operator -= ( float scalar )
		{
			x -= scalar;
			y -= scalar;
			z -= scalar;
			return *this;
		}
		Vector3 operator -= ( const Vector3 &R )
		{
			x -= R.x;
			y -= R.y;
			z -= R.z;
			return *this;
		}
		Vector3 operator -= ( const XMFLOAT3 &R )
		{
			x -= R.x;
			y -= R.y;
			z -= R.z;
			return *this;
		}
		Vector3 operator *= ( float scalar )
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			return *this;
		}
		Vector3 operator /= ( float scalar )
		{
			x /= scalar;
			y /= scalar;
			z /= scalar;
			return *this;
		}
	public:
		// using sqrtf().
		float Length()   const { return sqrtf( LengthSq() ); }
		float LengthSq() const { return ( x * x ) + ( y * y ) + ( z * z ); }
		Vector3 Normalize();

		/// <summary>
		/// Is Zero-vector?
		/// </summary>
		bool IsZero() const;
	public:
		float Dot( const Vector3  &R ) const
		{
			return ( x * R.x ) + ( y * R.y ) + ( z * R.z );
		}
		float Dot( const XMFLOAT3 &R ) const
		{
			return ( x * R.x ) + ( y * R.y ) + ( z * R.z );
		}
		Vector3 Cross( const Vector3  &R ) const
		{
			return Vector3
			{
				( y * R.z ) - ( z * R.y ),
				( z * R.x ) - ( x * R.z ),
				( x * R.y ) - ( y * R.x )
			};
		}
		Vector3 Cross( const XMFLOAT3 &R ) const
		{
			return Vector3
			{
				( y * R.z ) - ( z * R.y ),
				( z * R.x ) - ( x * R.z ),
				( x * R.y ) - ( y * R.x )
			};
		}
	public:
		static float Dot( const Vector3  &L, const Vector3  &R ) { return L.Dot( R ); }
		static float Dot( const XMFLOAT3 &L, const XMFLOAT3 &R ) { return Vector3( L ).Dot( R ); }
		static Vector3 Cross( const Vector3  &L, const Vector3  &R ) { return L.Cross( R ); }
		static Vector3 Cross( const XMFLOAT3 &L, const XMFLOAT3 &R ) { return Vector3( L ).Cross( R ); }
		static Vector3 Front()	{ return Vector3{ 0.0f, 0.0f, 1.0f }; }
		static Vector3 Right()	{ return Vector3{ 1.0f, 0.0f, 0.0f }; }
		static Vector3 Up()		{ return Vector3{ 0.0f, 1.0f, 0.0f }; }
		static Vector3 Zero()	{ return Vector3{ 0.0f, 0.0f, 0.0f }; }
	};

	static Vector3	operator + ( const Vector3 &L, float scalar )					{ return ( Vector3( L ) += scalar ); }
	static Vector3	operator - ( const Vector3 &L, float scalar )					{ return ( Vector3( L ) -= scalar ); }
	static Vector3	operator + ( const Vector3 &L, const Vector3 &R )				{ return ( Vector3( L ) += R ); }
	static Vector3	operator - ( const Vector3 &L, const Vector3 &R )				{ return ( Vector3( L ) -= R ); }
	static Vector3	operator * ( const Vector3 &L, float scalar )					{ return ( Vector3( L ) *= scalar ); }
	static Vector3	operator * ( float scalar, const Vector3 &R )					{ return ( Vector3( R ) *= scalar ); }
	static Vector3	operator / ( const Vector3 &L, float scalar )					{ return ( Vector3( L ) /= scalar ); }

	bool			operator == ( const Vector3 &L, const Vector3 &R );
	static bool		operator != ( const Vector3 &L, const Vector3 &R )				{ return !( L == R ); }

	static float	Dot( const Vector3 &L, const Vector3 &R )						{ return L.Dot( R );				}
	static float	Dot( const DirectX::XMFLOAT3 &L, const DirectX::XMFLOAT3 &R )	{ return Vector3( L ).Dot( R );		}
	static Vector3	Cross( const Vector3 &L, const Vector3 &R )						{ return L.Cross( R );				}
	static Vector3	Cross( const DirectX::XMFLOAT3 &L, const DirectX::XMFLOAT3 &R )	{ return Vector3( L ).Cross( R );	}

#pragma endregion

#pragma region Vector4

	struct Vector4 : public DirectX::XMFLOAT4
	{
	public:
		Vector4() : XMFLOAT4() {}
		Vector4( float x, float y, float z, float w ) : XMFLOAT4( x, y, z, w ) {}
		Vector4( const XMFLOAT4 &ref  ) : XMFLOAT4( ref ) {}
		Vector4( const XMFLOAT4 &&ref ) : XMFLOAT4( ref ) {}
		Vector4( const Vector4  &ref  ) : XMFLOAT4( ref ) {}
		Vector4( const Vector4  &&ref ) noexcept : XMFLOAT4( ref ) {}
		Vector4 &operator = ( float scalar ) noexcept { x = y = z = w = scalar; return *this; }
		Vector4 &operator = ( const XMFLOAT4 &ref  ) noexcept { x = ref.x; y = ref.y; z = ref.z; w = ref.w; return *this; }
		Vector4 &operator = ( const XMFLOAT4 &&ref ) noexcept { x = ref.x; y = ref.y; z = ref.z; w = ref.w; return *this; }
		Vector4 &operator = ( const Vector4  &ref  ) noexcept { x = ref.x; y = ref.y; z = ref.z; w = ref.w; return *this; }
		Vector4 &operator = ( const Vector4  &&ref ) noexcept { x = ref.x; y = ref.y; z = ref.z; w = ref.w; return *this; }
		~Vector4() = default;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, const std::uint32_t version )
		{
			archive( CEREAL_NVP( x ), CEREAL_NVP( y ), CEREAL_NVP( z ), CEREAL_NVP( w ) );
		}
	public:
		Vector4 operator - () const { return Vector4{ -x, -y, -z, -w }; }
		Vector4 operator += ( float scalar )
		{
			x += scalar;
			y += scalar;
			z += scalar;
			w += scalar;
			return *this;
		}
		Vector4 operator += ( const Vector4 &R )
		{
			x += R.x;
			y += R.y;
			z += R.z;
			w += R.w;
			return *this;
		}
		Vector4 operator += ( const XMFLOAT4 &R )
		{
			x += R.x;
			y += R.y;
			z += R.z;
			w += R.w;
			return *this;
		}
		Vector4 operator -= ( float scalar )
		{
			x -= scalar;
			y -= scalar;
			z -= scalar;
			w -= scalar;
			return *this;
		}
		Vector4 operator -= ( const Vector4 &R )
		{
			x -= R.x;
			y -= R.y;
			z -= R.z;
			w -= R.w;
			return *this;
		}
		Vector4 operator -= ( const XMFLOAT4 &R )
		{
			x -= R.x;
			y -= R.y;
			z -= R.z;
			w -= R.w;
			return *this;
		}
		Vector4 operator *= ( float scalar )
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			w *= scalar;
			return *this;
		}
		Vector4 operator /= ( float scalar )
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
		static Vector4 Zero() { return Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }; }
	};

	static Vector4	operator + ( const Vector4 &L, float scalar )		{ return ( Vector4( L ) += scalar ); }
	static Vector4	operator - ( const Vector4 &L, float scalar )		{ return ( Vector4( L ) -= scalar ); }
	static Vector4	operator + ( const Vector4 &L, const Vector4 &R )	{ return ( Vector4( L ) += R ); }
	static Vector4	operator - ( const Vector4 &L, const Vector4 &R )	{ return ( Vector4( L ) -= R ); }
	static Vector4	operator * ( const Vector4 &L, float scalar )		{ return ( Vector4( L ) *= scalar ); }
	static Vector4	operator * ( float scalar, const Vector4 &R )		{ return ( Vector4( R ) *= scalar ); }
	static Vector4	operator / ( const Vector4 &L, float scalar )		{ return ( Vector4( L ) /= scalar ); }
	
	bool			operator == ( const Vector4 &L, const Vector4 &R );
	static bool		operator != ( const Vector4 &L, const Vector4 &R )	{ return !( L == R ); }

#pragma endregion

#pragma region Int2

	/// <summary>
	/// Have x, y with int type.
	/// </summary>
	struct Int2
	{
		int x{};
		int y{};
	public:
		Int2() : x( 0 ), y( 0 ) {}
		Int2( int x, int y ) : x( x ), y( y ) {}
	public:
		Int2 operator += ( const Int2 &R )
		{
			x += R.x;
			y += R.y;
			return *this;
		}
		Int2 operator -= ( const Int2 &R )
		{
			x -= R.x;
			y -= R.y;
			return *this;
		}
	public:
		/// <summary>
		/// Convert by static_cast.
		/// </summary>
		Vector2 Float() const
		{
			return Donya::Vector2
			{
				static_cast<float>( x ),
				static_cast<float>( y )
			};
		}
	public:
		/// <summary>
		/// Convert by static_cast.
		/// </summary>
		static Int2 Create( const Vector2 &v )
		{
			return Donya::Int2
			{
				static_cast<int>( v.x ),
				static_cast<int>( v.y )
			};
		}
	};

	static Int2 operator + ( const Int2 &L, const Int2 &R ) { return ( Int2( L ) += R ); }
	static Int2 operator - ( const Int2 &L, const Int2 &R ) { return ( Int2( L ) -= R ); }

	static bool operator == ( const Int2 &L, const Int2 &R )
	{
		if ( L.x != R.x ) { return false; }
		if ( L.y != R.y ) { return false; }
		return true;
	}
	static bool operator != ( const Int2 &L, const Int2 &R ) { return !( L == R ); }

#pragma endregion

}
