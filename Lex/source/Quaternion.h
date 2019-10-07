#pragma once

#include "Vector.h"

namespace Donya
{
	/// <summary>
	/// The default-constructor generate identity.
	/// </summary>
	class Quaternion
	{
	public:
		float x{};
		float y{};
		float z{};
		float w{};
	public:
		Quaternion();
		Quaternion( float x, float y, float z, float w );
		// Copy, operator = are defaulted.
	public:
	#pragma region Arithmetic
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
		float LengthSq() const;
		/// <summary>
		/// |Q| = sqrt( q0^2 + q1^2 + q2^2 + q3^2 ) = sqrt( QQ* ) = sqrt( Q*Q )
		/// </summary>
		float Norm() const;

		/// <summary>
		/// Q /= |Q| (0 ;gt |Q|)
		/// </summary>
		void Normalize();

		/// <summary>
		/// Q* = s - v
		/// </summary>
		Quaternion Conjugate() const;

		/// <summary>
		/// Q-1 = Q* / |Q|^2
		/// </summary>
		Quaternion Inverse() const;

		/// <summary>
		/// 
		/// </summary>
		Donya::Vector3 GetAxis() const;

		/// <summary>
		/// Returns angles are radian.
		/// </summary>
		Donya::Vector3 GetEulerAngles() const;

		/// <summary>
		/// Returns = Q * V * Q*
		/// </summary>
		Donya::Vector3 RotateVector( const Donya::Vector3 &target ) const;

		/// <summary>
		/// The fourth-elements are same to identity.
		/// </summary>
		DirectX::XMFLOAT4X4 RequireRotationMatrix() const;
	public:
		/// <summary>
		/// 
		/// </summary>
		static float Dot( const Quaternion &L, const Quaternion &R );

		/// <summary>
		/// |Q| = sqrt( q0^2 + q1^2 + q2^2 + q3^2 ) = sqrt( QQ* ) = sqrt( Q*Q )
		/// </summary>
		static float Length( const Quaternion & );
		/// <summary>
		/// |Q| = sqrt( q0^2 + q1^2 + q2^2 + q3^2 ) = sqrt( QQ* ) = sqrt( Q*Q )
		/// </summary>
		static float LengthSq( const Quaternion & );
		/// <summary>
		/// |Q| = sqrt( q0^2 + q1^2 + q2^2 + q3^2 ) = sqrt( QQ* ) = sqrt( Q*Q )
		/// </summary>
		static float Norm( const Quaternion & );

		/// <summary>
		/// Q = s - v
		/// </summary>
		static Quaternion Conjugate( const Quaternion & );

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
		/// Create a quaternion of rotates in the "lookDirection".
		/// </summary>
		static Quaternion LookAt( const Donya::Vector3 &lookDirection );

		/// <summary>
		/// Returns Quaternion{ 0.0f, 0.0f, 0.0f, 1.0f }.
		/// </summary>
		static Quaternion Identity();

		/// <summary>
		/// Q-1 = Q* / |Q|^2
		/// </summary>
		static Quaternion Inverse( const Quaternion & );

		/// <summary>
		/// The time is 0.0f ~ 1.0f.
		/// </summary>
		static Quaternion Slerp( const Quaternion &beginNormalized, const Quaternion &endNormalized, float time );

		/// <summary>
		/// 
		/// </summary>
		static Donya::Vector3 GetAxis( const Quaternion & );

		/// <summary>
		/// Returns angles are radian.
		/// </summary>
		static Donya::Vector3 GetEulerAngles( const Quaternion & );

		/// <summary>
		/// Returns = Q * V * Q*
		/// </summary>
		static Donya::Vector3 RotateVector( const Quaternion &R, const Donya::Vector3 &target );

		/// <summary>
		/// The fourth-elements are same to identity.
		/// </summary>
		static DirectX::XMFLOAT4X4 RequireRotationMatrix( const Quaternion & );
	};
#pragma region Arithmetic
	static Quaternion operator + ( const Quaternion &L, const Quaternion &R )		{ return Quaternion( L ) += R;		}

	static Quaternion operator - ( const Quaternion &L, const Quaternion &R )		{ return Quaternion( L ) -= R;		}

	static Quaternion operator * ( const Quaternion &L, float scalar )				{ return Quaternion( L ) *= scalar;	}
	static Quaternion operator * ( const Quaternion &L, const Quaternion &R )		{ return Quaternion( L ) *= R;		}
	static Quaternion operator * ( const Quaternion &L, const Donya::Vector3 &R )	{ return Quaternion( L ) *= R;		}
	Quaternion operator * ( const Donya::Vector3 &L, const Quaternion &R );

	static Quaternion operator / ( const Quaternion &L, float scalar )				{ return Quaternion( L ) /= scalar; }
// region Arithmetic
#pragma endregion

	bool operator == ( const Quaternion &L, const Quaternion &R );
	static bool operator != ( const Quaternion &L, const Quaternion &R ) { return !( L == R ); }
}