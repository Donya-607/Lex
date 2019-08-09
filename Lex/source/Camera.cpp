#include "camera.h"

#include <algorithm>
#include <array>
#include <string>

#include "Common.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Quaternion.h"
#include "UseImgui.h"

#undef min
#undef max

using namespace DirectX;

Camera::Camera() :
	moveMode( Mode::None ),
	scopeAngle( ToRadian( 30.0f ) ),
	pos(), focus(), velocity(),
	projection()
{
	ResetPerspectiveProjection();
}
Camera::Camera( float scopeAngle ) :
	moveMode( Mode::None ),
	scopeAngle( scopeAngle ),
	pos(), focus(), velocity(),
	projection()
{
	ResetPerspectiveProjection();
}

void Camera::SetToHomePosition( Donya::Vector3 homePosition, Donya::Vector3 homeFocus )
{
	pos		= homePosition;
	focus	= homeFocus;
}

void Camera::SetScopeAngle( float scope )
{
	scopeAngle = scope;
}

void Camera::ResetOrthographicProjection()
{
	SetOrthographicProjectionMatrix( 16.0f, 9.0f, 0.01f, 1000.0f );
}
void Camera::ResetPerspectiveProjection()
{
	float aspectRatio = Common::ScreenWidthF() / Common::ScreenHeightF();
	float mostNear = 0.1f;
	float mostFar = 1000.0f;

	SetPerspectiveProjectionMatrix( scopeAngle, aspectRatio, mostNear, mostFar );
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

void Camera::SetVelocity()
{

}

void Camera::Move( const Donya::Vector3 &targetPos )
{
	switch ( moveMode )
	{
	case Mode::None:		Zoom();			break;
	case Mode::OrbitAround:	OrbitAround();	break;
	case Mode::Pan:			Pan();			break;
	}
}

void Camera::Zoom()
{
	velocity = pos - focus;

	float rot = scast<float>( Donya::Mouse::GetMouseWheelRot() );
	float speed = 1.0f;

	velocity.z += rot * speed;
	if ( velocity == ( pos - focus ) ) { return; }
	// else

	pos = focus + velocity;
}

void Camera::OrbitAround()
{

}

void Camera::Pan()
{
	float speed = 0.1f;
	Donya::Vector3 velo{ 0.0f, 0.0f, 0.0f };

	if ( Donya::Keyboard::Press( 'Z' ) )
	{
		velo.z = speed;
	}
	if ( Donya::Keyboard::Press( 'X' ) )
	{
		velo.z = -speed;
	}

	pos += velo;
	focus += velo;
}
