#pragma once

#include <cstdint> // Use std::uint32_t.
#include <cereal/cereal.hpp>

#include "Vector.h"

namespace Donya
{
	/// <summary>
	/// The member's order is (x, y, z, w). The default-constructor generate identity.
	/// </summary>
	class Quaternion
	{
	public:
		/// <summary>
		/// Use at LookAt(). The specified direction moves so as not to change.
		/// </summary>
		enum class Freeze
		{
			None,
			Up,
			Right,
			Front
		};
	public:
		float x{};
		float y{};
		float z{};
		float w{};
	public:
		constexpr Quaternion() : x( 0.0f ), y( 0.0f ), z( 0.0f ), w( 1.0f ) {}
		constexpr Quaternion( float x, float y, float z, float w ) : x( x ), y( y ), z( z ), w( w ) {}
		// Copy, operator = are defaulted.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive( CEREAL_NVP( x ), CEREAL_NVP( y ), CEREAL_NVP( z ), CEREAL_NVP( w ) );
		}
	public:
	#pragma region Arithmetic
		constexpr Quaternion Add( const Quaternion &R ) const
		{
			return Quaternion
			{
				x + R.x,
				y + R.y,
				z + R.z,
				w + R.w
			};
		}
		constexpr Quaternion Sub( const Quaternion &R ) const
		{
			return Quaternion
			{
				x - R.x,
				y - R.y,
				z - R.z,
				w - R.w
			};
		}
		constexpr Quaternion Mul( float scalar ) const
		{
			return Quaternion
			{
				x * scalar,
				y * scalar,
				z * scalar,
				w * scalar
			};
		}
		constexpr Quaternion Mul( const Quaternion &R ) const
		{
			return Quaternion
			{
				( w * R.x ) + ( x * R.w ) + ( y * R.z ) - ( z * R.y ),
				( w * R.y ) - ( x * R.z ) + ( y * R.w ) + ( z * R.x ),
				( w * R.z ) + ( x * R.y ) - ( y * R.x ) + ( z * R.w ),
				( w * R.w ) - ( x * R.x ) - ( y * R.y ) - ( z * R.z )
			};
		}
		constexpr Quaternion Mul( const Donya::Vector3 &R ) const
		{
			return Quaternion
			{
				( w * R.x ) + ( y * R.z ) - ( z * R.y ),
				( w * R.y ) - ( x * R.z ) + ( z * R.x ),
				( w * R.z ) + ( x * R.y ) - ( y * R.x ),
				( -x * R.x ) - ( y * R.y ) - ( z * R.z )
			};
		}
		constexpr Quaternion Div( float scalar ) const
		{
			// I should explicit.
			return { Mul( 1.0f / scalar ) };
		}

		/// <summary>
		/// The negation is applied to all four components. The myself(Q) and return-value(-Q) are equivalent.
		/// </summary>
		constexpr Quaternion operator - () const
		{
			return Quaternion{ -x, -y, -z, -w };
		}

		Quaternion &operator += ( const Quaternion &R );
		Quaternion &operator -= ( const Quaternion &R );
		Quaternion &operator *= ( float scalar );
		Quaternion &operator *= ( const Quaternion &R );
		Quaternion &operator *= ( const Donya::Vector3 &R );
		Quaternion &operator /= ( float scalar );
	// region Arithmetic
	#pragma endregion
	public:
		/// <summary>
		/// |Q| = sqrt( q0^2 + q1^2 + q2^2 + q3^2 ) = sqrt( QQ* ) = sqrt( Q*Q )
		/// </summary>
		float Length() const;
		/// <summary>
		/// |Q| = sqrt( q0^2 + q1^2 + q2^2 + q3^2 ) = sqrt( QQ* ) = sqrt( Q*Q )
		/// </summary>
		constexpr float LengthSq() const { return ( x * x ) + ( y * y ) + ( z * z ) + ( w * w ); }
		/// <summary>
		/// |Q| = sqrt( q0^2 + q1^2 + q2^2 + q3^2 ) = sqrt( QQ* ) = sqrt( Q*Q )
		/// </summary>
		float Norm() const;

		/// <summary>
		/// Q /= |Q| (0 &lt; |Q|)
		/// </summary>
		void Normalize();
		/// <summary>
		/// Q /= |Q| (0 &lt; |Q|)
		/// </summary>
		Quaternion Unit() const;

		/// <summary>
		/// Q* = s - v
		/// </summary>
		constexpr Quaternion Conjugate() const { return Quaternion{ -x, -y, -z, w }; }

		/// <summary>
		/// Q-1 = Q* / |Q|^2
		/// </summary>
		Quaternion Inverse() const;

		/// <summary>
		/// 
		/// </summary>
		constexpr float Dot( const Quaternion &R ) const
		{
			return ( x * R.x ) + ( y * R.y ) + ( z * R.z ) + ( w * R.w );
		}

		/// <summary>
		/// Judges whether is same rotation quaternion to argument. may be rotation is same between different quaternion.
		/// </summary>
		constexpr bool IsSameRotation( const Quaternion &R ) const
		{
			float diff = Dot( R ) - 1.0f;
			return ( -FLT_EPSILON < diff && diff < FLT_EPSILON ) ? true : false;
		}

		/// <summary>
		/// 
		/// </summary>
		constexpr Donya::Vector3 GetAxis() const { return Vector3{ x, y, z }; }

		/// <summary>
		/// Returns angles are radian.
		/// </summary>
		Donya::Vector3 GetEulerAngles() const;

		/// <summary>
		/// Returns = Q * V * Q*
		/// </summary>
		constexpr Donya::Vector3 RotateVector( const Donya::Vector3 &target ) const
		{
			const Quaternion V{ target.x, target.y, target.z, 0.0f };
			const Quaternion C = Conjugate();

		#if C_V_Q
			Quaternion rotated{};
			rotated = C.Mul( V );			// R = C * V
			rotated = rotated.Mul( *this );	// R = R * Q
		#else // Q_V_C
			Quaternion rotated{};
			rotated = Mul( V );				// R = Q * V
			rotated = rotated.Mul( C );		// R = R * C
		#endif // C_V_Q or Q_V_C

			return rotated.GetAxis();
		}

		/// <summary>
		/// Returns quaternion is rotated by "Q". &lt;Q * V * Q*&gt;
		/// </summary>
		constexpr Quaternion Rotated( const Quaternion &Q ) const
		{
			return Q.Mul( *this );
		}

		/// <summary>
		/// Rotate myself by argument. then return result. &lt;Q * V * Q*&gt;
		/// </summary>
		Quaternion RotateBy( const Quaternion & );

		/// <summary>
		/// Returns unit up vector of rotated by myself.
		/// </summary>
		constexpr Donya::Vector3 LocalUp() const
		{
			return RotateVector( Donya::Vector3::Up() );
		}
		/// <summary>
		/// Returns unit right vector of rotated by myself.
		/// </summary>
		constexpr Donya::Vector3 LocalRight() const
		{
			return RotateVector( Donya::Vector3::Right() );
		}
		/// <summary>
		/// Returns unit front vector of rotated by myself.
		/// </summary>
		constexpr Donya::Vector3 LocalFront() const
		{
			return RotateVector( Donya::Vector3::Front() );
		}

		/// <summary>
		/// [TRUE:returnsRotatedQuaternion] Create a quaternion that looking at "lookDirection".<para></para>
		/// [FALSE:returnsRotatedQuaternion] Create a quaternion that rotate from "orientation" to "lookDirection".
		/// </summary>
		Quaternion LookAt( const Donya::Vector3 &lookDirection, Freeze freezeDirection = Freeze::None, bool returnsRotatedQuaternion = true ) const;

		/// <summary>
		/// The fourth-elements are same to identity.
		/// </summary>
		constexpr DirectX::XMFLOAT4X4 RequireRotationMatrix() const
		{
			DirectX::XMFLOAT4X4 m{};

			m._11 = 1.0f - ( 2.0f * y * y ) - ( 2.0f * z * z );
			m._12 = ( 2.0f * x * y ) + ( 2.0f * w * z );
			m._13 = ( 2.0f * x * z ) - ( 2.0f * w * y );
			m._14 = 0.0f;

			m._21 = ( 2.0f * x * y ) - ( 2.0f * w * z );
			m._22 = 1.0f - ( 2.0f * x * x ) - ( 2.0f * z * z );
			m._23 = ( 2.0f * y * z ) + ( 2.0f * w * x );
			m._24 = 0.0f;

			m._31 = ( 2.0f * x * z ) + ( 2.0f * w * y );
			m._32 = ( 2.0f * y * z ) - ( 2.0f * w * x );
			m._33 = 1.0f - ( 2.0f * x * x ) - ( 2.0f * y * y );
			m._34 = 0.0f;

			m._41 = 0.0f;
			m._42 = 0.0f;
			m._43 = 0.0f;
			m._44 = 1.0f;

			return m;
		}
	public:
		/// <summary>
		/// 
		/// </summary>
		static constexpr float Dot( const Quaternion &L, const Quaternion &R )
		{
			return L.Dot( R );
		}

		/// <summary>
		/// Judges whether two quaternions are same rotation. may be rotation is same between different quaternion.
		/// </summary>
		static constexpr bool IsSameRotation( const Quaternion &L, const Quaternion &R )
		{
			return L.IsSameRotation( R );
		}

		/// <summary>
		/// |Q| = sqrt( q0^2 + q1^2 + q2^2 + q3^2 ) = sqrt( QQ* ) = sqrt( Q*Q )
		/// </summary>
		static float Length( const Quaternion & );
		/// <summary>
		/// |Q| = sqrt( q0^2 + q1^2 + q2^2 + q3^2 ) = sqrt( QQ* ) = sqrt( Q*Q )
		/// </summary>
		static constexpr float LengthSq( const Quaternion &Q )
		{
			return Q.LengthSq();
		}
		/// <summary>
		/// |Q| = sqrt( q0^2 + q1^2 + q2^2 + q3^2 ) = sqrt( QQ* ) = sqrt( Q*Q )
		/// </summary>
		static float Norm( const Quaternion & );

		/// <summary>
		/// Q = s - v
		/// </summary>
		static constexpr Quaternion Conjugate( const Quaternion &Q )
		{
			return Q.Conjugate();
		}

		/// <summary>
		/// Make quaternion from euler angles(radian) : pitch(rotate axis is x), yaw(rotate axis is y), roll(rotate axis is z).<para></para>
		/// The order of synthesis the angles is "ZYX".
		/// </summary>
		static Quaternion Make( float pitch, float yaw, float roll );
		/// <summary>
		/// Make quaternion from rotation-axis(please normalize) and rotation-theta(radian).
		/// </summary>
		static Quaternion Make( const Donya::Vector3 &normalizedAxis, float radianTheta );
		/// <summary>
		/// Make quaternion from rotation-matrix.<para></para>
		/// If passed wrong matrix, I returns Quaternion::Identity().
		/// </summary>
		static Quaternion Make( const DirectX::XMFLOAT4X4 &rotationMatrix );

		/// <summary>
		/// [TRUE:returnsRotatedQuaternion] Create a quaternion that looking at "lookDirection".<para></para>
		/// [FALSE:returnsRotatedQuaternion] Create a quaternion that rotate from "orientation" to "lookDirection".
		/// </summary>
		static Quaternion LookAt( const Quaternion &orientation, const Donya::Vector3 &lookDirection, Freeze freezeDirection = Freeze::None, bool returnsRotatedQuaternion = true );
		/// <summary>
		/// Create a quaternion that looking at "lookDirection".
		/// </summary>
		static Quaternion LookAt( const Donya::Vector3 &front, const Donya::Vector3 &lookDirection );

		/// <summary>
		/// Returns Quaternion{ 0.0f, 0.0f, 0.0f, 1.0f }.
		/// </summary>
		static constexpr Quaternion Identity()
		{
			return Quaternion{ 0.0f, 0.0f, 0.0f, 1.0f };
		}

		/// <summary>
		/// Q-1 = Q* / |Q|^2
		/// </summary>
		static Quaternion Inverse( const Quaternion & );

		/// <summary>
		/// The "percent" is 0.0f ~ 1.0f.
		/// </summary>
		static Quaternion Slerp( const Quaternion &startNormalized, const Quaternion &lastNormalized, float percent );

		/// <summary>
		/// 
		/// </summary>
		static constexpr Donya::Vector3 GetAxis( const Quaternion &Q )
		{
			return Q.GetAxis();
		}

		/// <summary>
		/// Returns angles are radian.
		/// </summary>
		static Donya::Vector3 GetEulerAngles( const Quaternion & );

		/// <summary>
		/// Returns = Q * V * Q*
		/// </summary>
		static constexpr Donya::Vector3 RotateVector( const Quaternion &Q, const Donya::Vector3 &target )
		{
			return Q.RotateVector( target );
		}

		/// <summary>
		/// Returns rotate "L" by "R". &lt;Q * V * Q*&gt;
		/// </summary>
		static constexpr Quaternion Rotated( const Quaternion &L, const Quaternion &R )
		{
			return L.Rotated( R );
		}

		static constexpr Donya::Vector3 LocalUp( const Quaternion &Q )
		{
			return Q.LocalUp();
		}
		static constexpr Donya::Vector3 LocalRight( const Quaternion &Q )
		{
			return Q.LocalRight();
		}
		static constexpr Donya::Vector3 LocalFront( const Quaternion &Q )
		{
			return Q.LocalFront();
		}

		/// <summary>
		/// The fourth-elements are same to identity.
		/// </summary>
		static constexpr DirectX::XMFLOAT4X4 RequireRotationMatrix( const Quaternion &Q )
		{
			return Q.RequireRotationMatrix();
		}
	};
#pragma region Arithmetic
	static constexpr Quaternion operator + ( const Quaternion &L, const Quaternion &R ) { return L.Add( R ); }
	static constexpr Quaternion operator - ( const Quaternion &L, const Quaternion &R ) { return L.Sub( R ); }
	static constexpr Quaternion operator * ( const Quaternion &L, float scalar ) { return L.Mul( scalar ); }
	static constexpr Quaternion operator * ( float scalar, const Quaternion &R ) { return R.Mul( scalar ); }
	static constexpr Quaternion operator * ( const Quaternion &L, const Quaternion &R ) { return L.Mul( R ); }
	static constexpr Quaternion operator * ( const Quaternion &L, const Donya::Vector3 &R ) { return L.Mul( R ); }
	static constexpr Quaternion operator * ( const Donya::Vector3 &L, const Quaternion &R )
	{
		return Quaternion
		{
			( L.x * R.w ) + ( L.y * R.z ) - ( L.z * R.y ),
			( -L.x * R.z ) + ( L.y * R.w ) + ( L.z * R.x ),
			( L.x * R.y ) - ( L.y * R.x ) + ( L.z * R.w ),
			( -L.x * R.x ) - ( L.y * R.y ) - ( L.z * R.z )
		};
	}
	static constexpr Quaternion operator / ( const Quaternion &L, float scalar ) { return L.Div( scalar ); }
// region Arithmetic
#pragma endregion

	/// <summary>
	/// Judges whether two quaternions are the same. but may be rotation is same between different quaternion.
	/// </summary>
	bool operator == ( const Quaternion &L, const Quaternion &R );
	static bool operator != ( const Quaternion &L, const Quaternion &R ) { return !( L == R ); }
}
CEREAL_CLASS_VERSION( Donya::Quaternion, 0 )
