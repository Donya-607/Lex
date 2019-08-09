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
	ChangeMode();

	SetVelocity();

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

void Camera::ChangeMode()
{
	enum PressState { Left = 0, Middle = 1, Right = 2 };
	const std::array<bool, 3> isPressed
	{
		Donya::Keyboard::Press( VK_LBUTTON ),
		Donya::Keyboard::Press( VK_MBUTTON ),
		Donya::Keyboard::Press( VK_RBUTTON )
	};

	switch ( moveMode )
	{
	case Mode::None:
		{
			// The mouse-button state is same as keyboard's state.

			if ( isPressed[Left] )
			{
				moveMode = Mode::OrbitAround;
			}
			if ( isPressed[Middle] )
			{
				moveMode = Mode::Pan;
			}

		}
		break;
	case Mode::OrbitAround:
		{
			if ( !isPressed[Left] )
			{
				moveMode = Mode::None;
			}
		}
		break;
	case Mode::Pan:
		{
			if ( !isPressed[Middle] )
			{
				moveMode = Mode::None;
			}
		}
		break;
	}
}

void Camera::SetVelocity()
{
	static Donya::Vector2 prevMouse{};
	static Donya::Vector2 currentMouse{};

	prevMouse = currentMouse;

	Donya::Mouse::GetMouseCoord( &currentMouse.x, &currentMouse.y );

	auto SetLessOrGreater =
	[]( float *pOut, float judgeNo )
	{
		if ( judgeNo < 0.0f )
		{
			*pOut = -1.0f;
		}
		else
		if ( 0.0f < judgeNo )
		{
			*pOut = 1.0f;
		}
	};

	Donya::Vector2 diff = currentMouse - prevMouse;
	SetLessOrGreater( &velocity.x, diff.x );
	SetLessOrGreater( &velocity.y, diff.y );
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

	int rot = Donya::Mouse::GetMouseWheelRot();
	if ( !rot ) { return; }
	// else
	
	rot *= -1; // I want the distance to get closer as "rot" gets bigger.

	constexpr float MAGNI = 0.1f;
	float addition = scast<float>( rot ) * MAGNI;

	velocity *= 1.0f + addition;
	if ( ZeroEqual( velocity.LengthSq() ) ) { return; }
	// else

	pos = focus + velocity;
}

void Camera::OrbitAround()
{

}

void Camera::Pan()
{
	constexpr float SPEED = 0.1f;
	velocity *= SPEED;

	velocity.x *= -1; // Move-direction fit to mouse.
	velocity.z = 0.0f;

	pos		+= velocity;
	focus	+= velocity;
}
