#pragma once

#include <cstdint> // Use for std::uint32_t.
#include <DirectXMath.h>

#include "cereal/cereal.hpp"

namespace Donya
{

#pragma region Vector2

	struct Vector2 : public DirectX::XMFLOAT2
	{
	public:
		constexpr Vector2() : XMFLOAT2() {}
		constexpr Vector2( float x, float y ) : XMFLOAT2( x, y ) {}
		constexpr Vector2( const XMFLOAT2 &ref ) : XMFLOAT2( ref ) {}
		constexpr Vector2( const XMFLOAT2 &&ref ) : XMFLOAT2( ref ) {}
		constexpr Vector2( const Vector2 &ref ) : XMFLOAT2( ref ) {}
		constexpr Vector2( const Vector2 &&ref ) noexcept : XMFLOAT2( ref ) {}
		Vector2 &operator = ( float scalar ) noexcept { x = y = scalar;       return *this; }
		Vector2 &operator = ( const XMFLOAT2 &ref ) noexcept { x = ref.x; y = ref.y; return *this; }
		Vector2 &operator = ( const XMFLOAT2 &&ref ) noexcept { x = ref.x; y = ref.y; return *this; }
		Vector2 &operator = ( const Vector2 &ref ) noexcept { x = ref.x; y = ref.y; return *this; }
		Vector2 &operator = ( const Vector2 &&ref ) noexcept { x = ref.x; y = ref.y; return *this; }
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( x ), CEREAL_NVP( y ) );
		}
	public:
		constexpr XMFLOAT2 XMFloat() const
		{
			// HACK:Can I prevent slice by this ?
			return static_cast<XMFLOAT2>( *this );
		}
	public:
		/// <summary>
		/// Multiply each element.
		/// </summary>
		constexpr Vector2 Product( const Vector2 &R ) const
		{
			return Vector2
			{
				x * R.x,
				y * R.y
			};
		}
		constexpr Vector2 operator - () const { return Vector2{ -x, -y }; }
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
		constexpr float LengthSq() const { return ( x * x ) + ( y * y ); }
		Vector2 Normalize();
		Vector2 Normalized() const;

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
		constexpr float Dot( const Vector2 &R ) const
		{
			return ( x * R.x ) + ( y * R.y );
		}
		constexpr float Dot( const XMFLOAT2 &R ) const
		{
			return ( x * R.x ) + ( y * R.y );
		}
		constexpr float Cross( const Vector2 &R ) const
		{
			return ( x * R.y ) - ( y * R.x );
		}
		constexpr float Cross( const XMFLOAT2 &R ) const
		{
			return ( x * R.y ) - ( y * R.x );
		}
	public:
		/// <summary>
		/// Multiply each element.
		/// </summary>
		static constexpr Vector2 Product( const Vector2 &L, const Vector2 &R )
		{
			return L.Product( R );
		}

		static float Dot( const Vector2 &L, const Vector2 &R ) { return L.Dot( R ); }
		static float Dot( const XMFLOAT2 &L, const XMFLOAT2 &R ) { return Vector2( L ).Dot( R ); }
		static float Cross( const Vector2 &L, const Vector2 &R ) { return L.Cross( R ); }
		static float Cross( const XMFLOAT2 &L, const XMFLOAT2 &R ) { return Vector2( L ).Cross( R ); }
		static Vector2 Right() { return Vector2{ 1.0f, 0.0f }; }
		static Vector2 Up() { return Vector2{ 0.0f, 1.0f }; }
		static Vector2 Zero() { return Vector2{ 0.0f, 0.0f }; }
	};

	static Vector2	operator + ( const Vector2 &L, float scalar ) { return ( Vector2( L ) += scalar ); }
	static Vector2	operator - ( const Vector2 &L, float scalar ) { return ( Vector2( L ) -= scalar ); }
	static Vector2	operator + ( const Vector2 &L, const Vector2 &R ) { return ( Vector2( L ) += R ); }
	static Vector2	operator - ( const Vector2 &L, const Vector2 &R ) { return ( Vector2( L ) -= R ); }
	static Vector2	operator * ( const Vector2 &L, float scalar ) { return ( Vector2( L ) *= scalar ); }
	static Vector2	operator * ( float scalar, const Vector2 &R ) { return ( Vector2( R ) *= scalar ); }
	static Vector2	operator / ( const Vector2 &L, float scalar ) { return ( Vector2( L ) /= scalar ); }

	bool			operator == ( const Vector2 &L, const Vector2 &R );
	static bool		operator != ( const Vector2 &L, const Vector2 &R ) { return !( L == R ); }

	static constexpr float	Dot( const Vector2 &L, const Vector2 &R ) { return L.Dot( R ); }
	static constexpr float	Dot( const DirectX::XMFLOAT2 &L, const DirectX::XMFLOAT2 &R ) { return Vector2( L ).Dot( R ); }
	static constexpr float	Cross( const Vector2 &L, const Vector2 &R ) { return L.Cross( R ); }
	static constexpr float	Cross( const DirectX::XMFLOAT2 &L, const DirectX::XMFLOAT2 &R ) { return Vector2( L ).Cross( R ); }

#pragma endregion

#pragma region Vector3

	struct Vector3 : public DirectX::XMFLOAT3
	{
	public:
		constexpr Vector3() : XMFLOAT3() {}
		constexpr Vector3( float x, float y, float z ) : XMFLOAT3( x, y, z ) {}
		constexpr Vector3( const XMFLOAT3 &ref ) : XMFLOAT3( ref ) {}
		constexpr Vector3( const XMFLOAT3 &&ref ) : XMFLOAT3( ref ) {}
		constexpr Vector3( const Vector3 &ref ) : XMFLOAT3( ref ) {}
		constexpr Vector3( const Vector3 &&ref ) noexcept : XMFLOAT3( ref ) {}
		constexpr Vector3( const Vector2 &xy, float z ) : XMFLOAT3( xy.x, xy.y, z ) {}
		Vector3 &operator = ( float scalar ) noexcept { x = y = z = scalar;              return *this; }
		Vector3 &operator = ( const XMFLOAT3 &ref ) noexcept { x = ref.x; y = ref.y; z = ref.z; return *this; }
		Vector3 &operator = ( const XMFLOAT3 &&ref ) noexcept { x = ref.x; y = ref.y; z = ref.z; return *this; }
		Vector3 &operator = ( const Vector3 &ref ) noexcept { x = ref.x; y = ref.y; z = ref.z; return *this; }
		Vector3 &operator = ( const Vector3 &&ref ) noexcept { x = ref.x; y = ref.y; z = ref.z; return *this; }
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( x ), CEREAL_NVP( y ), CEREAL_NVP( z ) );
		}
	public:
		constexpr XMFLOAT3 XMFloat() const
		{
			// HACK:Can I prevent slice by this ?
			return static_cast<XMFLOAT3>( *this );
		}
	public:
		/// <summary>
		/// Multiply each element.
		/// </summary>
		constexpr Vector3 Product( const Vector3 &R ) const
		{
			return Vector3
			{
				x * R.x,
				y * R.y,
				z * R.z
			};
		}
		constexpr Vector3 operator - () const { return Vector3{ -x, -y, -z }; }
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
		constexpr float LengthSq() const { return ( x * x ) + ( y * y ) + ( z * z ); }
		Vector3 Normalize();
		Vector3 Normalized() const;

		/// <summary>
		/// Is Zero-vector?
		/// </summary>
		bool IsZero() const;
	public:
		constexpr float Dot( const Vector3 &R ) const
		{
			return ( x * R.x ) + ( y * R.y ) + ( z * R.z );
		}
		constexpr float Dot( const XMFLOAT3 &R ) const
		{
			return ( x * R.x ) + ( y * R.y ) + ( z * R.z );
		}
		constexpr Vector3 Cross( const Vector3 &R ) const
		{
			return Vector3
			{
				( y * R.z ) - ( z * R.y ),
				( z * R.x ) - ( x * R.z ),
				( x * R.y ) - ( y * R.x )
			};
		}
		constexpr Vector3 Cross( const XMFLOAT3 &R ) const
		{
			return Vector3
			{
				( y * R.z ) - ( z * R.y ),
				( z * R.x ) - ( x * R.z ),
				( x * R.y ) - ( y * R.x )
			};
		}
		DirectX::XMVECTOR ToXMVector( float fourthParam ) const
		{
			return DirectX::XMVectorSet( x, y, z, fourthParam );
		}
	public:
		/// <summary>
		/// Multiply each element.
		/// </summary>
		static constexpr Vector3	Product( const Vector3 &L, const Vector3 &R )
		{
			return L.Product( R );
		}

		static constexpr float		Dot( const Vector3 &L, const Vector3 &R ) { return L.Dot( R ); }
		static constexpr float		Dot( const XMFLOAT3 &L, const XMFLOAT3 &R ) { return Vector3( L ).Dot( R ); }
		static constexpr Vector3	Cross( const Vector3 &L, const Vector3 &R ) { return L.Cross( R ); }
		static constexpr Vector3	Cross( const XMFLOAT3 &L, const XMFLOAT3 &R ) { return Vector3( L ).Cross( R ); }
		static constexpr Vector3	Front() { return Vector3{ 0.0f, 0.0f, 1.0f }; }
		static constexpr Vector3	Right() { return Vector3{ 1.0f, 0.0f, 0.0f }; }
		static constexpr Vector3	Up() { return Vector3{ 0.0f, 1.0f, 0.0f }; }
		static constexpr Vector3	Zero() { return Vector3{ 0.0f, 0.0f, 0.0f }; }
		static DirectX::XMVECTOR	ToXMVector( const Vector3 &V, float fourthParam )
		{
			return V.ToXMVector( fourthParam );
		}
	};

	static Vector3	operator + ( const Vector3 &L, float scalar ) { return ( Vector3( L ) += scalar ); }
	static Vector3	operator - ( const Vector3 &L, float scalar ) { return ( Vector3( L ) -= scalar ); }
	static Vector3	operator + ( const Vector3 &L, const Vector3 &R ) { return ( Vector3( L ) += R ); }
	static Vector3	operator - ( const Vector3 &L, const Vector3 &R ) { return ( Vector3( L ) -= R ); }
	static Vector3	operator * ( const Vector3 &L, float scalar ) { return ( Vector3( L ) *= scalar ); }
	static Vector3	operator * ( float scalar, const Vector3 &R ) { return ( Vector3( R ) *= scalar ); }
	static Vector3	operator / ( const Vector3 &L, float scalar ) { return ( Vector3( L ) /= scalar ); }

	bool			operator == ( const Vector3 &L, const Vector3 &R );
	static bool		operator != ( const Vector3 &L, const Vector3 &R ) { return !( L == R ); }

	static constexpr float		Dot( const Vector3 &L, const Vector3 &R ) { return L.Dot( R ); }
	static constexpr float		Dot( const DirectX::XMFLOAT3 &L, const DirectX::XMFLOAT3 &R ) { return Vector3( L ).Dot( R ); }
	static constexpr Vector3	Cross( const Vector3 &L, const Vector3 &R ) { return L.Cross( R ); }
	static constexpr Vector3	Cross( const DirectX::XMFLOAT3 &L, const DirectX::XMFLOAT3 &R ) { return Vector3( L ).Cross( R ); }

#pragma endregion

#pragma region Vector4

	struct Vector4 : public DirectX::XMFLOAT4
	{
	public:
		constexpr Vector4() : XMFLOAT4() {}
		constexpr Vector4( float x, float y, float z, float w ) : XMFLOAT4( x, y, z, w ) {}
		constexpr Vector4( const XMFLOAT4 &ref ) : XMFLOAT4( ref ) {}
		constexpr Vector4( const XMFLOAT4 &&ref ) : XMFLOAT4( ref ) {}
		constexpr Vector4( const Vector4 &ref ) : XMFLOAT4( ref ) {}
		constexpr Vector4( const Vector4 &&ref ) noexcept : XMFLOAT4( ref ) {}
		constexpr Vector4( const Vector3 &xyz, float w ) : XMFLOAT4( xyz.x, xyz.y, xyz.z, w ) {}
		Vector4 &operator = ( float scalar ) noexcept { x = y = z = w = scalar; return *this; }
		Vector4 &operator = ( const XMFLOAT4 &ref ) noexcept { x = ref.x; y = ref.y; z = ref.z; w = ref.w; return *this; }
		Vector4 &operator = ( const XMFLOAT4 &&ref ) noexcept { x = ref.x; y = ref.y; z = ref.z; w = ref.w; return *this; }
		Vector4 &operator = ( const Vector4 &ref ) noexcept { x = ref.x; y = ref.y; z = ref.z; w = ref.w; return *this; }
		Vector4 &operator = ( const Vector4 &&ref ) noexcept { x = ref.x; y = ref.y; z = ref.z; w = ref.w; return *this; }
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, const std::uint32_t version )
		{
			archive( CEREAL_NVP( x ), CEREAL_NVP( y ), CEREAL_NVP( z ), CEREAL_NVP( w ) );
		}
	public:
		constexpr XMFLOAT4 XMFloat() const
		{
			// HACK:Can I prevent slice by this ?
			return static_cast<XMFLOAT4>( *this );
		}
	public:
		/// <summary>
		/// Multiply each element.
		/// </summary>
		constexpr Vector4 Product( const Vector4 &R ) const
		{
			return Vector4
			{
				x * R.x,
				y * R.y,
				z * R.z,
				w * R.w
			};
		}
		constexpr Vector4 operator - () const { return Vector4{ -x, -y, -z, -w }; }
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
		/// <summary>
		/// Returns reference of this.
		/// </summary>
		Vector4 &AssignXMVector( const DirectX::XMVECTOR &V );
		DirectX::XMVECTOR ToXMVector() const
		{
			return DirectX::XMVectorSet( x, y, z, w );
		}
	public:
		/// <summary>
		/// Multiply each element.
		/// </summary>
		static constexpr Vector4 Product( const Vector4 &L, const Vector4 &R )
		{
			return L.Product( R );
		}

		static Vector4 Zero() { return Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }; }
		static Vector4 FromXMVector( const DirectX::XMVECTOR &V );
		static DirectX::XMVECTOR ToXMVector( const Vector4 &V ) { return V.ToXMVector(); }
	};

	static Vector4	operator + ( const Vector4 &L, float scalar ) { return ( Vector4( L ) += scalar ); }
	static Vector4	operator - ( const Vector4 &L, float scalar ) { return ( Vector4( L ) -= scalar ); }
	static Vector4	operator + ( const Vector4 &L, const Vector4 &R ) { return ( Vector4( L ) += R ); }
	static Vector4	operator - ( const Vector4 &L, const Vector4 &R ) { return ( Vector4( L ) -= R ); }
	static Vector4	operator * ( const Vector4 &L, float scalar ) { return ( Vector4( L ) *= scalar ); }
	static Vector4	operator * ( float scalar, const Vector4 &R ) { return ( Vector4( R ) *= scalar ); }
	static Vector4	operator / ( const Vector4 &L, float scalar ) { return ( Vector4( L ) /= scalar ); }

	bool			operator == ( const Vector4 &L, const Vector4 &R );
	static bool		operator != ( const Vector4 &L, const Vector4 &R ) { return !( L == R ); }

#pragma endregion

#pragma region Vector4x4

	/// <summary>
	/// This class is wrapper of DirectX::XMFLOAT4X4, DirectX::XMMATRIX.<para></para>
	/// Row-major.<para></para>
	/// The default-constructor is make identity.
	/// </summary>
	struct Vector4x4 : public DirectX::XMFLOAT4X4
	{
	public:
		constexpr Vector4x4() : XMFLOAT4X4( 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 ) {}
		constexpr Vector4x4
		(
			float _11, float _12, float _13, float _14,
			float _21, float _22, float _23, float _24,
			float _31, float _32, float _33, float _34,
			float _41, float _42, float _43, float _44
		) :
			XMFLOAT4X4
			(
				_11, _12, _13, _14,
				_21, _22, _23, _24,
				_31, _32, _33, _34,
				_41, _42, _43, _44
			)
		{
		}
		constexpr Vector4x4( const XMFLOAT4X4 &ref ) : XMFLOAT4X4( ref ) {}
		constexpr Vector4x4( const XMFLOAT4X4 &&ref ) : XMFLOAT4X4( ref ) {}
		Vector4x4 &operator = ( const XMFLOAT4X4 &ref ) noexcept
		{
			_11 = ref._11;	_12 = ref._12;	_13 = ref._13;	_14 = ref._14;
			_21 = ref._21;	_22 = ref._22;	_23 = ref._23;	_24 = ref._24;
			_31 = ref._31;	_32 = ref._32;	_33 = ref._33;	_34 = ref._34;
			_41 = ref._41;	_42 = ref._42;	_43 = ref._43;	_44 = ref._44;
			return *this;
		}
		Vector4x4 &operator = ( const XMFLOAT4X4 &&ref ) noexcept
		{
			_11 = ref._11;	_12 = ref._12;	_13 = ref._13;	_14 = ref._14;
			_21 = ref._21;	_22 = ref._22;	_23 = ref._23;	_24 = ref._24;
			_31 = ref._31;	_32 = ref._32;	_33 = ref._33;	_34 = ref._34;
			_41 = ref._41;	_42 = ref._42;	_43 = ref._43;	_44 = ref._44;
			return *this;
		}
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, const std::uint32_t version )
		{
		#define NVP CEREAL_NVP // Lazy typing.
			archive
			(
				NVP( _11 ), NVP( _12 ), NVP( _13 ), NVP( _14 ),
				NVP( _21 ), NVP( _22 ), NVP( _23 ), NVP( _24 ),
				NVP( _31 ), NVP( _32 ), NVP( _33 ), NVP( _34 ),
				NVP( _41 ), NVP( _42 ), NVP( _43 ), NVP( _44 )
			);
		#undef NVP
		}
	public:
		/// <summary>
		/// Access to element. the index is 0-based, 0~15. row-major.
		/// </summary>
		const float &operator [] ( unsigned int index ) const &;
		/// <summary>
		/// Access to element. the index is 0-based, 0~15. row-major.
		/// </summary>
		float &operator [] ( unsigned int index ) &;
		/// <summary>
		/// Access to element. the index is 0-based, 0~15. row-major.
		/// </summary>
		float		operator  [] ( unsigned int index ) const &&;
		/// <summary>
		/// Access to element. the index is 0-based, 0~3. row-major.
		/// </summary>
		float &operator () ( unsigned int row, unsigned int column );
	public:
		constexpr XMFLOAT4X4 XMFloat() const
		{
			// HACK:Can I prevent slice by this ?
			return static_cast<XMFLOAT4X4>( *this );
		}
		/// <summary>
		/// Returns reference of this.
		/// </summary>
		Vector4x4 &AssignMatrix( const DirectX::XMMATRIX &M );
		DirectX::XMMATRIX ToMatrix() const;
	public:
	#pragma region Arithmetic
		Vector4x4 Mul( const Vector4x4 &R ) const;
		Vector4   Mul( const Vector4 &vector ) const;
		Vector4   Mul( const Vector3 &vector, float fourthParam ) const;

		Vector4x4 &operator *= ( const Vector4x4 &R );
	#pragma endregion
	public:
		Vector4x4 Inverse()   const;
		Vector4x4 Transpose() const;
		Vector4x4 OrthographicLH( const Vector2 &viewSize, float zNear, float zFar ) const;
		Vector4x4 PerspectiveFovLH( float FOVAngleRadianY, float aspectRatio, float zNear, float zFar ) const;
	public:
		static Vector4x4 FromMatrix( const DirectX::XMMATRIX &M );
		static DirectX::XMMATRIX ToMatrix( const Vector4x4 &V )
		{
			return V.ToMatrix();
		}
	public:
		static constexpr Vector4x4 Identity()
		{
			return Vector4x4
			{
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			};
		}

		static Vector4x4 MakeScaling( const Vector3 &scale );
		static Vector4x4 MakeScaling( float eachScale )
		{
			return MakeScaling( Vector3{ eachScale, eachScale, eachScale } );
		}

		/// <summary>
		/// Make rotation matrix in Vector4x4 from euler angles(radian).
		/// </summary>
		static Vector4x4 MakeRotationEuler( const Vector3 &eulerRadian );
		/// <summary>
		/// Make rotation matrix in Vector4x4 from "rotation axis" and angle(radian).
		/// </summary>
		static Vector4x4 MakeRotationAxis( const Vector3 &axis, float angleRadian );
		/// <summary>
		/// Make rotation matrix in Vector4x4 from "normalized rotation axis" and angle(radian).
		/// </summary>
		static Vector4x4 MakeRotationNormalAxis( const Vector3 &nAxis, float angleRadian );
		/// <summary>
		/// Make rotation matrix in Vector4x4 from "RightAxis", "UpAxis" and "FrontAxis". these axis should be normalized.
		/// </summary>
		constexpr static Vector4x4 MakeRotationOrthogonalAxis( const Vector3 &rightAxis, const Vector3 &upAxis, const Vector3 &frontAxis )
		{
			return Vector4x4
			{
				rightAxis.x,	rightAxis.y,	rightAxis.z,	0.0f,
				upAxis.x,		upAxis.y,		upAxis.z,		0.0f,
				frontAxis.x,	frontAxis.y,	frontAxis.z,	0.0f,
				0.0f,			0.0f,			0.0f,			1.0f
			};
		}

		static Vector4x4 MakeTranslation( const Vector3 &offset );
		static Vector4x4 MakeTranslation( float ofsX, float ofsY, float ofsZ )
		{
			return MakeTranslation( { ofsX, ofsY, ofsZ } );
		}
	};

	static Vector4x4	operator * ( const Vector4x4 &lhs, const Vector4x4 &rhs ) { return lhs.Mul( rhs ); }
	static Vector4		operator * ( const Vector4x4 &matrix, const Vector4 &vector ) { return matrix.Mul( vector ); }
	static Vector4		operator * ( const Vector4 &vector, const Vector4x4 &matrix ) { return matrix.Mul( vector ); }

	bool				operator == ( const Vector4x4 &L, const Vector4x4 &R );
	static bool			operator != ( const Vector4x4 &L, const Vector4x4 &R ) { return !( L == R ); }

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
		constexpr Int2() : x( 0 ), y( 0 ) {}
		constexpr Int2( int x, int y ) : x( x ), y( y ) {}
		constexpr Int2( const Int2 &ref ) : x( ref.x ), y( ref.y ) {}
		Int2 &operator = ( int scalar ) noexcept { x = y = scalar;       return *this; }
		Int2 &operator = ( const Int2 &ref ) noexcept { x = ref.x; y = ref.y; return *this; }
		Int2 &operator = ( const Int2 &&ref ) noexcept { x = ref.x; y = ref.y; return *this; }
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( x ), CEREAL_NVP( y ) );
		}
	public:
		constexpr Int2 operator - () const { return Int2{ -x, -y }; }
		Int2 operator += ( int   scalar )
		{
			x += scalar;
			y += scalar;
			return *this;
		}
		Int2 operator += ( const Int2 &R )
		{
			x += R.x;
			y += R.y;
			return *this;
		}
		Int2 operator -= ( int   scalar )
		{
			x -= scalar;
			y -= scalar;
			return *this;
		}
		Int2 operator -= ( const Int2 &R )
		{
			x -= R.x;
			y -= R.y;
			return *this;
		}
		Int2 operator *= ( int   scalar )
		{
			x *= scalar;
			y *= scalar;
			return *this;
		}
		Int2 operator /= ( int   scalar )
		{
			x /= scalar;
			y /= scalar;
			return *this;
		}
	public:
		/// <summary>
		/// Convert by static_cast.
		/// </summary>
		constexpr Vector2 Float() const
		{
			return Vector2
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

	static Int2 operator + ( const Int2 &L, int   scalar ) { return ( Int2( L ) += scalar ); }
	static Int2 operator + ( const Int2 &L, const Int2 &R ) { return ( Int2( L ) += R ); }
	static Int2 operator - ( const Int2 &L, int   scalar ) { return ( Int2( L ) -= scalar ); }
	static Int2 operator - ( const Int2 &L, const Int2 &R ) { return ( Int2( L ) -= R ); }
	static Int2 operator * ( const Int2 &L, int   scalar ) { return ( Int2( L ) *= scalar ); }
	static Int2 operator / ( const Int2 &L, int   scalar ) { return ( Int2( L ) /= scalar ); }

	static constexpr bool operator == ( const Int2 &L, const Int2 &R )
	{
		if ( L.x != R.x ) { return false; }
		if ( L.y != R.y ) { return false; }
		return true;
	}
	static constexpr bool operator != ( const Int2 &L, const Int2 &R ) { return !( L == R ); }

#pragma endregion

#pragma region Int3

	/// <summary>
	/// Have x, y, z with int type.
	/// </summary>
	struct Int3
	{
		int x{};
		int y{};
		int z{};
	public:
		constexpr Int3() : x( 0 ), y( 0 ), z( 0 ) {}
		constexpr Int3( int x, int y, int z ) : x( x ), y( y ), z( z ) {}
		constexpr Int3( const Int3 &ref ) : x( ref.x ), y( ref.y ), z( ref.z ) {}
		Int3 &operator = ( int scalar ) noexcept { x = y = z = scalar;              return *this; }
		Int3 &operator = ( const Int3 &ref ) noexcept { x = ref.x; y = ref.y; z = ref.z; return *this; }
		Int3 &operator = ( const Int3 &&ref ) noexcept { x = ref.x; y = ref.y; z = ref.z; return *this; }
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( x ), CEREAL_NVP( y ), CEREAL_NVP( z ) );
		}
	public:
		constexpr Int3 operator - () const { return Int3{ -x, -y, -z }; }
		Int3 operator += ( int   scalar )
		{
			x += scalar;
			y += scalar;
			z += scalar;
			return *this;
		}
		Int3 operator += ( const Int3 &R )
		{
			x += R.x;
			y += R.y;
			z += R.z;
			return *this;
		}
		Int3 operator -= ( int   scalar )
		{
			x -= scalar;
			y -= scalar;
			z -= scalar;
			return *this;
		}
		Int3 operator -= ( const Int3 &R )
		{
			x -= R.x;
			y -= R.y;
			z -= R.z;
			return *this;
		}
		Int3 operator *= ( int   scalar )
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			return *this;
		}
		Int3 operator /= ( int   scalar )
		{
			x /= scalar;
			y /= scalar;
			z /= scalar;
			return *this;
		}
	public:
		/// <summary>
		/// Convert by static_cast.
		/// </summary>
		constexpr Vector3 Float() const
		{
			return Vector3
			{
				static_cast<float>( x ),
				static_cast<float>( y ),
				static_cast<float>( z )
			};
		}
	public:
		/// <summary>
		/// Convert by static_cast.
		/// </summary>
		static Int3 Create( const Vector3 &v )
		{
			return Int3
			{
				static_cast<int>( v.x ),
				static_cast<int>( v.y ),
				static_cast<int>( v.z )
			};
		}
	};

	static Int3 operator + ( const Int3 &L, int   scalar ) { return ( Int3( L ) += scalar ); }
	static Int3 operator + ( const Int3 &L, const Int3 &R ) { return ( Int3( L ) += R ); }
	static Int3 operator - ( const Int3 &L, int   scalar ) { return ( Int3( L ) -= scalar ); }
	static Int3 operator - ( const Int3 &L, const Int3 &R ) { return ( Int3( L ) -= R ); }
	static Int3 operator * ( const Int3 &L, int   scalar ) { return ( Int3( L ) *= scalar ); }
	static Int3 operator / ( const Int3 &L, int   scalar ) { return ( Int3( L ) /= scalar ); }

	static constexpr bool operator == ( const Int3 &L, const Int3 &R )
	{
		if ( L.x != R.x ) { return false; }
		if ( L.y != R.y ) { return false; }
		if ( L.z != R.z ) { return false; }
		return true;
	}
	static constexpr bool operator != ( const Int3 &L, const Int3 &R ) { return !( L == R ); }

#pragma endregion

#pragma region Int4

	/// <summary>
	/// Have x, y, z with int type.
	/// </summary>
	struct Int4
	{
		int x{};
		int y{};
		int z{};
		int w{};
	public:
		constexpr Int4() : x( 0 ), y( 0 ), z( 0 ), w( 0 ) {}
		constexpr Int4( int x, int y, int z, int w ) : x( x ), y( y ), z( z ), w( w ) {}
		constexpr Int4( const Int4 &ref ) : x( ref.x ), y( ref.y ), z( ref.z ) {}
		Int4 &operator = ( int scalar ) noexcept { x = y = z = w = scalar;                     return *this; }
		Int4 &operator = ( const Int4 &ref ) noexcept { x = ref.x; y = ref.y; z = ref.z; w = ref.w; return *this; }
		Int4 &operator = ( const Int4 &&ref ) noexcept { x = ref.x; y = ref.y; z = ref.z; w = ref.w; return *this; }
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( x ), CEREAL_NVP( y ), CEREAL_NVP( z ), CEREAL_NVP( w ) );
		}
	public:
		constexpr Int4 operator - () const { return Int4{ -x, -y, -z, -w }; }
		Int4 operator += ( int   scalar )
		{
			x += scalar;
			y += scalar;
			z += scalar;
			w += scalar;
			return *this;
		}
		Int4 operator += ( const Int4 &R )
		{
			x += R.x;
			y += R.y;
			z += R.z;
			w += R.w;
			return *this;
		}
		Int4 operator -= ( int   scalar )
		{
			x -= scalar;
			y -= scalar;
			z -= scalar;
			w -= scalar;
			return *this;
		}
		Int4 operator -= ( const Int4 &R )
		{
			x -= R.x;
			y -= R.y;
			z -= R.z;
			w -= R.w;
			return *this;
		}
		Int4 operator *= ( int   scalar )
		{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			w *= scalar;
			return *this;
		}
		Int4 operator /= ( int   scalar )
		{
			x /= scalar;
			y /= scalar;
			z /= scalar;
			w /= scalar;
			return *this;
		}
	public:
		/// <summary>
		/// Convert by static_cast.
		/// </summary>
		constexpr Vector4 Float() const
		{
			return Vector4
			{
				static_cast<float>( x ),
				static_cast<float>( y ),
				static_cast<float>( z ),
				static_cast<float>( w )
			};
		}
	public:
		/// <summary>
		/// Convert by static_cast.
		/// </summary>
		static Int4 Create( const Vector4 &v )
		{
			return Int4
			{
				static_cast<int>( v.x ),
				static_cast<int>( v.y ),
				static_cast<int>( v.z ),
				static_cast<int>( v.w )
			};
		}
	};

	static Int4 operator + ( const Int4 &L, int   scalar ) { return ( Int4( L ) += scalar ); }
	static Int4 operator + ( const Int4 &L, const Int4 &R ) { return ( Int4( L ) += R ); }
	static Int4 operator - ( const Int4 &L, int   scalar ) { return ( Int4( L ) -= scalar ); }
	static Int4 operator - ( const Int4 &L, const Int4 &R ) { return ( Int4( L ) -= R ); }
	static Int4 operator * ( const Int4 &L, int   scalar ) { return ( Int4( L ) *= scalar ); }
	static Int4 operator / ( const Int4 &L, int   scalar ) { return ( Int4( L ) /= scalar ); }

	static constexpr bool operator == ( const Int4 &L, const Int4 &R )
	{
		if ( L.x != R.x ) { return false; }
		if ( L.y != R.y ) { return false; }
		if ( L.z != R.z ) { return false; }
		if ( L.w != R.w ) { return false; }
		return true;
	}
	static constexpr bool operator != ( const Int4 &L, const Int4 &R ) { return !( L == R ); }

#pragma endregion

}
