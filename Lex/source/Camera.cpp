#include "camera.h"

#include <algorithm>
#include <array>
#include <string>

#include "Common.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "UseImgui.h"

#undef min
#undef max

using namespace DirectX;

Camera::Camera() :
	moveMode( Mode::None ),
	radius(),
	scopeAngle(),
	pos(), focus(), velocity(),
	mouse(),
	posture(),
	projection()
{
	SetScopeAngle( ToRadian( 30.0f ) ); // default-value.
	ResetPerspectiveProjection();
}
Camera::Camera( float scope ) :
	moveMode( Mode::None ),
	radius(),
	scopeAngle(),
	pos(), focus(), velocity(),
	mouse(),
	posture(),
	projection()
{
	SetScopeAngle( scope );
	ResetPerspectiveProjection();
}

void Camera::SetToHomePosition( Donya::Vector3 homePosition, Donya::Vector3 homeFocus )
{
	pos		= homePosition;
	focus	= homeFocus;
	posture	= Donya::Quaternion::Identity();
}

void Camera::SetScopeAngle( float scope )
{
	scopeAngle = scope;
	CalcDistToVirtualScreen();
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
	MouseUpdate();

	ChangeMode();

	Move( targetPos );

#if DEBUG_MODE

	ShowPaametersToImGui();

#endif // DEBUG_MODE
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

void Camera::MouseUpdate()
{
	mouse.prev = mouse.current;

	Donya::Mouse::GetMouseCoord( &mouse.current.x, &mouse.current.y );

	// TODO:I should support looping mouse.
}

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
			if ( isPressed[Middle] || isPressed[Right] )
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
			if ( !( isPressed[Middle] || isPressed[Right] ) )
			{
				moveMode = Mode::None;
			}
		}
		break;
	}
}

void SetVelocity( Donya::Vector3 *pVelocity )
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
	SetLessOrGreater( &pVelocity->x, diff.x );
	SetLessOrGreater( &pVelocity->y, diff.y );
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
	radius = Donya::Vector3{ pos - focus }.Length();
}

void Camera::OrbitAround()
{
	if ( mouse.prev == mouse.current ) { return; }
	// else

	float rotateSpeed = 1.0f;
	Donya::Vector2 diff = mouse.current - mouse.prev;

	if ( !ZeroEqual( diff.x ) )
	{
		float radian = ToRadian( diff.x * rotateSpeed );
		Donya::Vector3 up = posture.RotateVector( Donya::Vector3::Up() );

		Donya::Quaternion rotate = Donya::Quaternion::Make( up, radian );
		posture *= rotate;
	}
	if ( !ZeroEqual( diff.y ) )
	{
		float radian = ToRadian( diff.y * rotateSpeed );
		Donya::Vector3 right = posture.RotateVector( Donya::Vector3::Right() );

		Donya::Quaternion rotate = Donya::Quaternion::Make( right, radian );
		posture *= rotate;
	}

	Donya::Vector3 front = posture.RotateVector( Donya::Vector3::Front() );
	front *= radius;

	pos = focus + ( -front );
}

void Multiply( XMFLOAT3 *pOutput, const XMFLOAT4X4 &M )
{
	XMFLOAT3 &V = *pOutput;

	float x = ( V.x * M._11 ) + ( V.y * M._21 ) + ( V.z * M._31 );
	float y = ( V.x * M._12 ) + ( V.y * M._22 ) + ( V.z * M._32 );
	float z = ( V.x * M._13 ) + ( V.y * M._23 ) + ( V.z * M._33 );

	V.x = x;
	V.y = y;
	V.z = z;
}

void Camera::Pan()
{
	if ( mouse.prev == mouse.current ) { return; }
	// else

	Donya::Vector3 from	= ToWorldPos( mouse.prev	);
	Donya::Vector3 to	= ToWorldPos( mouse.current	);

	Donya::Vector3 moveVec = to - from;
	moveVec.x *= -1; // If you want move to right, the camera(myself) must move to left.

	pos		+= moveVec;
	focus	+= moveVec;

	// Old behavior
	/*
	constexpr float SPEED = 0.1f;
	velocity *= SPEED;

	velocity.x *= -1; // Move-direction fit to mouse.
	velocity.z = 0.0f;

	pos		+= velocity;
	focus	+= velocity;
	*/
}

void Camera::CalcDistToVirtualScreen()
{
	// see http://marupeke296.com/ALG_No7_MoveCameraWithCursor.html

	float FOV = scopeAngle;
	FOV *= 0.5f;

	virtualDistance = Common::ScreenHeightF() / ( 2 * tanf( FOV ) );
}

Donya::Vector3 Camera::ToWorldPos( const Donya::Vector2 &screenPos )
{
	// see http://marupeke296.com/ALG_No7_MoveCameraWithCursor.html

	Donya::Vector3 worldPos{};
	{
		Donya::Vector3 virtualPos{ screenPos.x, screenPos.y, virtualDistance };

		XMFLOAT4X4 pose{};
		XMStoreFloat4x4( &pose, GetViewMatrix() );

		Multiply( &virtualPos, pose );

		worldPos = virtualPos;
	}

	float rayLength{}; // The "a" of reference site.
	{
		Donya::Vector3 virNormal = pos - focus;

		float dotSample = Donya::Vector3::Dot( { Donya::Vector3::Zero() - pos }, virNormal );
		float dotTarget = Donya::Vector3::Dot( { worldPos - pos }, virNormal );

		rayLength = dotSample / dotTarget;
	}

	Donya::Vector3 onPlanePos = pos + ( Donya::Vector3{ worldPos - pos } * rayLength );
	return onPlanePos;
}

#if DEBUG_MODE

void Camera::ShowPaametersToImGui()
{
#if USE_IMGUI

	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( "Camera" ) )
		{
			std::string vec3Info{ "[X:%5.3f][Y:%5.3f][Z:%5.3f]" };
			std::string vec4Info{ "[X:%5.3f][Y:%5.3f][Z:%5.3f][W:%5.3f]" };
			auto ShowVec3 =
			[&vec3Info]( std::string name, const Donya::Vector3 &param )
			{
				ImGui::Text( ( name + vec3Info ).c_str(), param.x, param.y, param.z );
			};
			auto ShowVec4 =
			[&vec4Info]( std::string name, const Donya::Vector4 &param )
			{
				ImGui::Text( ( name + vec4Info ).c_str(), param.x, param.y, param.z, param.w );
			};

			Donya::Vector3 up		= posture.RotateVector( Donya::Vector3::Up()	);
			Donya::Vector3 right	= posture.RotateVector( Donya::Vector3::Right()	);
			Donya::Vector3 front	= posture.RotateVector( Donya::Vector3::Front()	);

			ShowVec3( "Pos",	pos		);
			ShowVec3( "Focus",	focus	);
			ShowVec3( "Up",		up		);
			ShowVec3( "Right",	right	);
			ShowVec3( "Front",	front	);
			
			Donya::Vector4 vec4Posture
			{
				posture.x,
				posture.y,
				posture.z,
				posture.w
			};
			ShowVec4( "Posture", vec4Posture );
			ImGui::Text( "Norm[%5.3f]", posture.Length() );

			ImGui::TreePop();
		}

		ImGui::End();
	}

#endif // USE_IMGUI
}

#endif // DEBUG_MODE