#include "camera.h"

#include <algorithm>
#include <array>
#include <string>

#include "Common.h"
#include "Keyboard.h"
#include "UseImgui.h"

#undef min
#undef max

using namespace DirectX;

void RotateX( DirectX::XMFLOAT3 *target, float radian )
{
	/*
	1     0      0      0
	0 cos(Rx)-sin(Rx)   0
	0 sin(Rx) cos(Rx)   0
	0     0      0      1
	*/

	float cos = cosf( radian );
	float sin = sinf( radian );

	target->x = ( target->x * 1 ) + ( target->y * 0		) + ( target->z * 0		) + ( 1 * 0 );
	target->y = ( target->x * 0 ) + ( target->y * cos	) + ( target->z *-sin 	) + ( 1 * 0 );
	target->z = ( target->x * 0 ) + ( target->y * sin	) + ( target->z * cos 	) + ( 1 * 0 );
}
void RotateY( DirectX::XMFLOAT3 *target, float radian )
{
	/*
	cos(Ry)  0   sin(Ry)   0
	0        1      0      0
	-sin(Ry) 0   cos(Ry)   0
	0        0      0      1
	*/

	float cos = cosf( radian );
	float sin = sinf( radian );

	target->x = ( target->x * cos	) + ( target->y * 0 ) + ( target->z * sin	) + ( 1 * 0 );
	target->y = ( target->x * 0		) + ( target->y * 1 ) + ( target->z * 0		) + ( 1 * 0 );
	target->z = ( target->x *-sin	) + ( target->y * 0 ) + ( target->z * cos	) + ( 1 * 0 );
}
void RotateZ( DirectX::XMFLOAT3 *target, float radian )
{
	/*
	cos(Rz)  -sin(Rz)  0      0
	sin(Rz)  cos(Rz)   0      0
	0        0         1      0
	0        0         0      1
	*/

	float cos = cosf( radian );
	float sin = sinf( radian );

	target->x = ( target->x * cos	) + ( target->y *-sin	) + ( target->z * 0 ) + ( 1 * 0 );
	target->y = ( target->x * sin	) + ( target->y * cos	) + ( target->z * 0 ) + ( 1 * 0 );
	target->z = ( target->x * 0		) + ( target->y * 0		) + ( target->z * 1 ) + ( 1 * 0 );
}
void RotateZYX( DirectX::XMFLOAT3 *target, float radian )
{
	RotateZ( target, radian );
	RotateY( target, radian );
	RotateX( target, radian );
}

Camera::Camera() :
	scopeAngle( ToRadian( 30.0f ) ),
	pos(), focus(),
	projection()
{
	ResetPerspectiveProjection();
}
Camera::Camera( float scopeAngle ) :
	scopeAngle( scopeAngle ),
	pos(), focus(),
	projection()
{
	ResetPerspectiveProjection();
}

void Camera::SetHomePosition( Donya::Vector3 homePosition, Donya::Vector3 homeFocus )
{
	pos		= homePosition;
	focus	= homeFocus;
}

void Camera::SetScopeAngle( float scope )
{
	scopeAngle = scope;
}

XMMATRIX Camera::SetOrthographicProjectionMatrix( float width, float height, float mostNear, float mostFar )
{
	XMStoreFloat4x4( &projection, DirectX::XMMatrixOrthographicLH( width, height, mostNear, mostFar ) );
	return GetProjectionMatrix();
}

XMMATRIX Camera::SetPerspectiveProjectionMatrix( float aspectRatio )
{
	return SetPerspectiveProjectionMatrix( scopeAngle, aspectRatio, 0.1f, 1000.0f );
}
XMMATRIX Camera::SetPerspectiveProjectionMatrix( float scopeAngle, float aspectRatio, float mostNear, float mostFar )
{
	XMStoreFloat4x4( &projection, DirectX::XMMatrixPerspectiveFovLH( scopeAngle, aspectRatio, mostNear, mostFar ) );
	return GetProjectionMatrix();
}

XMMATRIX Camera::GetViewMatrix() const
{
	XMVECTOR vEye	= DirectX::XMVectorSet( pos.x, pos.y, pos.z, 1.0f );
	XMVECTOR vFocus	= DirectX::XMVectorSet( focus.x, focus.y, focus.z, 1.0f );
	XMVECTOR vUp	= DirectX::XMVectorSet( 0, 1.0f, 0, 0 );

	return DirectX::XMMatrixLookAtLH( vEye, vFocus, vUp );
}

XMMATRIX Camera::GetProjectionMatrix() const
{
	return XMLoadFloat4x4( &projection );
}

void Camera::Update( const Donya::Vector3 &targetPos )
{
	Move( targetPos );
}

// Interpolate is not used in current implement.
/*
void Camera::Interpolate()
{
	constexpr size_t SIZE = 1;
	const std::array<DirectX::XMFLOAT3 *, SIZE> DEST =
	{
		&destPos,
		&destFocus
	};
	const std::array<DirectX::XMFLOAT3 *, SIZE> POS =
	{
		&pos,
		&focus
	};

	for ( size_t i = 0; i < SIZE; ++i )
	{
		DirectX::XMFLOAT3 diff = Sub( *DEST[i], *POS[i] );
		if ( fabsf( Length( diff ) ) < FLT_EPSILON )
		{
			*POS[i] = *DEST[i];
			return;
		}
		// else

		diff = Multi( diff, chaseSpeed );

		*POS[i] = Add( *POS[i], diff );
	}
}
*/

void Camera::Move( const Donya::Vector3 &targetPos )
{
	float speed = 0.1f;
	Donya::Vector3 velo{ 0.0f, 0.0f, 0.0f };

	if ( Donya::Keyboard::Press( 'Z' ) )
	{
		velo.z = -speed;
	}
	if ( Donya::Keyboard::Press( 'X' ) )
	{
		velo.z = speed;
	}

	pos += velo;
	focus += velo;
}

void Camera::ResetOrthographicProjection()
{
	SetOrthographicProjectionMatrix( 16.0f, 9.0f, 0.01f, 1000.0f );
}
void Camera::ResetPerspectiveProjection()
{
	float aspectRatio	= Common::ScreenWidthF() / Common::ScreenHeightF();
	float mostNear		= 0.1f;
	float mostFar		= 1000.0f;

	SetPerspectiveProjectionMatrix( scopeAngle, aspectRatio, mostNear, mostFar );
}
