#include "Vector.h"

#include "Useful.h" // Use ToDegree(), Equal().

using namespace DirectX;

namespace Donya
{
#pragma region Vector2

	Vector2 Vector2::Normalize()
	{
		float length = Length();

		if ( ZeroEqual( length ) ) { return *this; }
		// else

		x /= length;
		y /= length;

		return *this;
	}
	Vector2 Vector2::Normalized() const
	{
		Vector2 normalized = *this;
		normalized.Normalize();
		return normalized;
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

#pragma endregion

#pragma region Vector3

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
	Vector3 Vector3::Normalized() const
	{
		Vector3 normalized = *this;
		normalized.Normalize();
		return normalized;
	}
	bool Vector3::IsZero() const
	{
		return ( ZeroEqual( LengthSq() ) ) ? true : false;
	}
	bool operator == ( const Vector3 &L, const Vector3 &R )
	{
		if ( !ZeroEqual( L.x - R.x ) ) { return false; }
		if ( !ZeroEqual( L.y - R.y ) ) { return false; }
		if ( !ZeroEqual( L.z - R.z ) ) { return false; }
		// else
		return true;
	}

#pragma endregion

#pragma region Vector4

	Vector4 &Vector4::AssignXMVector( const XMVECTOR &V )
	{
		XMStoreFloat4( this, V );
		return *this;
	}

	Vector4 Vector4::FromXMVector( const XMVECTOR &V )
	{
		XMFLOAT4 vector{};
		XMStoreFloat4( &vector, V );
		return Vector4{ vector };
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

#pragma endregion

#pragma region Vector4x4

	/// <summary>
	/// Returns X:Row, Y:Column.
	/// </summary>
	constexpr Int2 CalcRowColumnFromIndex( unsigned int index )
	{
		constexpr unsigned int ROW_COUNT = 4U;
		return Donya::Int2
		{
			static_cast<int>( index % ROW_COUNT ),
			static_cast<int>( index / ROW_COUNT )
		};
	}
	const float &Vector4x4::operator [] ( unsigned int index ) const &
	{
		Int2 indices = CalcRowColumnFromIndex( index );
		return m[indices.x][indices.y];
	}
	float &Vector4x4::operator [] ( unsigned int index ) &
	{
		Int2 indices = CalcRowColumnFromIndex( index );
		return m[indices.x][indices.y];
	}
	float Vector4x4::operator [] ( unsigned int index ) const &&
	{
		Int2 indices = CalcRowColumnFromIndex( index );
		return m[indices.x][indices.y];
	}
	float &Vector4x4::operator () ( unsigned int row, unsigned int column )
	{
		constexpr unsigned int ROW_COUNT	= 4U;
		constexpr unsigned int COLUMN_COUNT	= 4U;
		if ( ROW_COUNT <= row || COLUMN_COUNT <= column )
		{
			_ASSERT_EXPR( 0, L"Error : out of range at Vector4x4::operator() access." );
			return operator[]( 0 );
		}
		// else

		return m[row][column];
	}

	Vector4x4 &Vector4x4::AssignMatrix( const XMMATRIX &M )
	{
		XMStoreFloat4x4( this, M );
		return *this;
	}
	XMMATRIX  Vector4x4::ToMatrix() const
	{
		return XMLoadFloat4x4( this );
	}

#pragma region Arithmetic
	Vector4x4 Vector4x4::Mul( const Vector4x4 &R ) const
	{
		XMMATRIX LHS = ToMatrix();
		XMMATRIX RHS = R.ToMatrix();
		
		return FromMatrix( LHS * RHS );
	}
	Vector4   Vector4x4::Mul( const Vector4 &V ) const
	{
		return Vector4::FromXMVector
		(
			XMVector4Transform
			(
				V.ToXMVector(),
				ToMatrix()
			)
		);
	}
	Vector4   Vector4x4::Mul( const Vector3 &V, float fourthParam ) const
	{
		return Mul( Vector4{ V.x,V.y,V.z, fourthParam } );
	}

	Vector4x4 &Vector4x4::operator *= ( const Vector4x4 &R )
	{
		*this = Mul( R );
		return *this;
	}
#pragma endregion

	Vector4x4 Vector4x4::Inverse() const
	{
		return FromMatrix
		(
			XMMatrixInverse( nullptr, ToMatrix() )
		);
	}
	Vector4x4 Vector4x4::Transpose() const
	{
		return FromMatrix
		(
			XMMatrixTranspose( ToMatrix() )
		);
	}
	Vector4x4 Vector4x4::OrthographicLH( const Vector2 &view, float zNear, float zFar ) const
	{
		return FromMatrix
		(
			XMMatrixOrthographicLH( view.x, view.y, zNear, zFar )
		);
	}
	Vector4x4 Vector4x4::PerspectiveFovLH( float FOV, float aspect, float zNear, float zFar ) const
	{
		return FromMatrix
		(
			XMMatrixPerspectiveFovLH( FOV, aspect, zNear, zFar )
		);
	}
	
	Vector4x4 Vector4x4::FromMatrix( const XMMATRIX &M )
	{
		XMFLOAT4X4 matrix{};
		XMStoreFloat4x4( &matrix, M );
		return Vector4x4{ matrix };
	}

	Vector4x4 Vector4x4::MakeScaling( const Vector3 &scale )
	{
		return FromMatrix
		(
			XMMatrixScaling
			(
				scale.x,
				scale.y,
				scale.z
			)
		);
	}
	Vector4x4 Vector4x4::MakeRotationEuler( const Vector3 &eulerRadian )
	{
		return FromMatrix
		(
			XMMatrixRotationRollPitchYaw
			(
				eulerRadian.x,
				eulerRadian.y,
				eulerRadian.z
			)
		);
	}
	Vector4x4 Vector4x4::MakeRotationAxis( const Vector3 &axis, float angleRadian )
	{
		return FromMatrix
		(
			XMMatrixRotationAxis
			(
				axis.ToXMVector( 0.0f ),
				angleRadian
			)
		);
	}
	Vector4x4 Vector4x4::MakeRotationNormalAxis( const Vector3 &nAxis, float angleRadian )
	{
		return FromMatrix
		(
			XMMatrixRotationNormal
			(
				nAxis.ToXMVector( 0.0f ),
				angleRadian
			)
		);
	}
	Vector4x4 Vector4x4::MakeTranslation( const Vector3 &offset )
	{
		return FromMatrix
		(
			XMMatrixTranslation( offset.x, offset.y, offset.z )
		);
	}

	bool operator == ( const Vector4x4 &L, const Vector4x4 &R )
	{
		constexpr unsigned int ELEMENT_COUNT = 15U;
		for ( unsigned int i = 0; i < ELEMENT_COUNT; ++i )
		{
			if ( !ZeroEqual( L[i] - R[i] ) )
			{
				return false;
			}
		}
		return true;
	}

#pragma endregion
}