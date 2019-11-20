#include "Quaternion.h"

#include <array>

#include "Useful.h"	// Using Donya::ZeroEqual()

using namespace Donya;
using namespace DirectX;

namespace Donya
{
#pragma region Arithmetic

	Quaternion &Quaternion::operator += ( const Quaternion &R )
	{
		*this = Add( R );
		return *this;
	}
	Quaternion &Quaternion::operator -= ( const Quaternion &R )
	{
		*this = Sub( R );
		return *this;
	}
	Quaternion &Quaternion::operator *= ( float scalar )
	{
		*this = Mul( scalar );
		return *this;
	}
	Quaternion &Quaternion::operator *= ( const Quaternion &R )
	{
		*this = Mul( R );
		return *this;
	}
	Quaternion &Quaternion::operator *= ( const Vector3 &R )
	{
		*this = Mul( R );
		return *this;
	}
	Quaternion &Quaternion::operator /= ( float scalar )
	{
		*this = Div( scalar );
		return *this;
	}

// region Arithmetic
#pragma endregion

	float Quaternion::Length() const
	{
		return sqrtf( LengthSq() );
	}
	float Quaternion::Norm() const
	{
		return Length();
	}

	void Quaternion::Normalize()
	{
		float len = Length();
		if ( ZeroEqual( len ) ) { return; }
		// else
		*this /= len;
	}
	Quaternion Quaternion::Normalized() const
	{
		Quaternion normalized = *this;
		normalized.Normalize();
		return normalized;
	}

	Quaternion Quaternion::Inverse() const
	{
		float norm = Length();

		if ( ZeroEqual( norm - 1.0f ) )
		{
			// norm == 1
			return Conjugate();
		}
		// else

		return ( Conjugate() / ( norm * norm ) );
	}

	Vector3 Quaternion::GetEulerAngles() const
	{
		// from https://stackoverflow.com/questions/15187309/quaternion-euler?rq=1

		Quaternion unitQ = *this;
		unitQ.Normalize();

		constexpr float RANGE	= 0.4999f; // 0.4999f or 0.5f - EPSILON
		constexpr float HALF_PI	= PI / 2.0f;

		Vector3 v{};

		float test = unitQ.x * unitQ.y + unitQ.z * unitQ.w;
		if ( RANGE < test )		// Singularity at north pole.
		{
			v.y = 2.0f * atan2f( unitQ.x, unitQ.w );
			v.z = HALF_PI;
			v.x = 0.0F;
			return v;
		}
		// else
		if ( test < -RANGE )	// Singularity at south pole.
		{
			v.y = -2.0f * atan2f( unitQ.x, unitQ.w );
			v.z = -HALF_PI;
			v.x = 0.0F;
			return v;
		}
		// else

		float sqX = unitQ.x * unitQ.x;
		float sqY = unitQ.y * unitQ.y;
		float sqZ = unitQ.z * unitQ.z;

		v.y = atan2f( ( 2.0f * unitQ.y * unitQ.w ) - ( 2.0f * unitQ.x * unitQ.z ), 1.0f - ( 2.0f * sqY ) - ( 2.0f * sqZ ) );
		v.z = asinf(  2.0f * test );
		v.x = atan2f( ( 2.0f * unitQ.x * unitQ.w ) - ( 2.0f * unitQ.y * unitQ.z ), 1.0f - ( 2.0f * sqX ) - ( 2.0f * sqZ ) );
		return v;
	}

	Quaternion Quaternion::RotateBy( const Quaternion &Q )
	{
		*this = Rotated( Q );
		return *this;
	}

	float Quaternion::Length( const Quaternion &Q )
	{
		return Q.Length();
	}
	float Quaternion::Norm( const Quaternion &Q )
	{
		return Q.Length();
	}

	Quaternion Quaternion::Make( float pitch, float yaw, float roll )
	{
		// The radians should be halved.
		pitch	*= 0.5f;
		yaw		*= 0.5f;
		roll	*= 0.5f;

		float cP = cosf( pitch	);
		float sP = sinf( pitch	);

		float cY = cosf( yaw	);
		float sY = sinf( yaw	);

		float cR = cosf( roll	);
		float sR = sinf( roll	);

		// from https://stackoverflow.com/questions/15187309/quaternion-euler?rq=1
		// ZYX.

		Quaternion q{};
		q.x = cY * sP * cR + sY * cP * sR;
		q.y = sY * cP * cR - cY * sP * sR;
		q.z = cY * cP * sR - sY * sP * cR;
		q.w = cY * cP * cR + sY * sP * sR;
		return q;
	}

	Quaternion Quaternion::Make( const Vector3 &nAxis, float radTheta )
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

	Quaternion MakeLookAtRotation( const Donya::Vector3 &nFront, const Donya::Vector3 &nLookDir )
	{
		if ( nFront == nLookDir ) { return Quaternion::Identity(); }
		// else
		
		float cosTheta = Donya::Vector3::Dot( nFront, nLookDir );
		cosTheta = std::max( -1.0f, std::min( 1.0f, cosTheta ) );	// Prevent NaN.
		
		auto IsVectorInverse = []( float dot )
		{
			return ( dot <= ( -1.0f + EPSILON ) ) ? true : false;
		};
		if ( IsVectorInverse( cosTheta ) )
		{
			// The "lookDirection" is inverse to front.
			// Find horizontal-axis by cross to forward, right, and up vector.

			constexpr std::array<Donya::Vector3, 3> AXES
			{
				Donya::Vector3::Front(),
				Donya::Vector3::Right(),
				Donya::Vector3::Up()
			};

			Donya::Vector3 rotAxis{};
			for ( const auto AXIS : AXES )
			{
				rotAxis = Donya::Vector3::Cross( nFront, AXIS );
				if ( !rotAxis.IsZero() ) { break; }
			}

			if ( rotAxis.IsZero() ) { return Donya::Quaternion::Identity(); }
			// else

			rotAxis.Normalize();
			return Donya::Quaternion::Make( rotAxis, ToRadian( 180.0f ) );
		}
		// else

		const float rotAngle = acosf( cosTheta );
		Donya::Vector3 rotAxis = Donya::Vector3::Cross( nFront, nLookDir ).Normalized();

		return ( ZeroEqual( rotAngle ) )
		? Donya::Quaternion::Identity()
		: Donya::Quaternion::Make( rotAxis, rotAngle );
	}
	Quaternion Quaternion::LookAt( const Quaternion &orientation, const Donya::Vector3 &lookDirection, bool retRotatedQuat )
	{
		if ( lookDirection.IsZero() ) { return Identity(); }
		// else

	#if FROM_ROTATION_MATRIX
		Vector3 front = lookDirection;							front.Normalize();
		Vector3 right = Vector3::Cross( Vector3::Up(), front );	right.Normalize();
		Vector3 up = Vector3::Cross( front, right );			up.Normalize();

		XMFLOAT4X4 matrix{}; XMStoreFloat4x4( &matrix, XMMatrixIdentity() );
		matrix._11 = right.x;	matrix._12 = right.y;	matrix._13 = right.z;
		matrix._21 = up.x;		matrix._22 = up.y;		matrix._23 = up.z;
		matrix._31 = front.x;	matrix._32 = front.y;	matrix._33 = front.z;

		return Make( matrix );
	#else
		Quaternion rotation = MakeLookAtRotation
		(
			orientation.LocalFront(),
			lookDirection.Normalized()
		);
		return ( retRotatedQuat ) ? orientation.Rotated( rotation ) : rotation;
	#endif // FROM_ROTATION_MATRIX
	}
	Quaternion Quaternion::LookAt( const Donya::Vector3 &front, const Donya::Vector3 &lookDirection )
	{
		if ( lookDirection.IsZero() ) { return Identity(); }
		// else
		return MakeLookAtRotation
		(
			front.Normalized(),
			lookDirection.Normalized()
		);
	}

	Quaternion Quaternion::Inverse( const Quaternion &Q )
	{
		return Q.Inverse();
	}

	Quaternion Quaternion::Slerp( const Quaternion &nBegin, const Quaternion &nEnd, float time )
	{
		float dot	= Dot( nBegin, nEnd );
		if ( dot < -1.0f || 1.0f < dot )
		{
			// The acos() is returns NaN if received value of outside range of [-1 ~ +1].
			dot = std::max( -1.0f, std::min( 1.0f, dot ) );
		}

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

	Vector3 Quaternion::GetEulerAngles( const Quaternion &Q )
	{
		return Q.GetEulerAngles();
	}
	
	bool operator == ( const Quaternion &L, const Quaternion &R )
	{
		if ( !ZeroEqual( L.x - R.x ) ) { return false; }
		if ( !ZeroEqual( L.y - R.y ) ) { return false; }
		if ( !ZeroEqual( L.z - R.z ) ) { return false; }
		if ( !ZeroEqual( L.w - R.w ) ) { return false; }
		// else
		return true;
	}
}