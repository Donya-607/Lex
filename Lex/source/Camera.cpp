#include "camera.h"

#include <algorithm>
#include <array>
#include <string>

#include "Common.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Useful.h"
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
	
	Donya::Vector3 dir = focus - pos; dir.Normalize();
	posture = Donya::Quaternion::LookAt( dir );
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
	XMStoreFloat4x4( &projection, XMMatrixOrthographicLH( width, height, mostNear, mostFar ) );
	return GetProjectionMatrix();
}
XMMATRIX Camera::SetPerspectiveProjectionMatrix( float aspectRatio )
{
	return SetPerspectiveProjectionMatrix( scopeAngle, aspectRatio, 0.1f, 1000.0f );
}
XMMATRIX Camera::SetPerspectiveProjectionMatrix( float scopeAngle, float aspectRatio, float mostNear, float mostFar )
{
	XMStoreFloat4x4( &projection, XMMatrixPerspectiveFovLH( scopeAngle, aspectRatio, mostNear, mostFar ) );
	return GetProjectionMatrix();
}

XMMATRIX Camera::CalcViewMatrix() const
{
	XMMATRIX R{};
	{
		Donya::Quaternion invRot = posture.Conjugate();
		XMFLOAT4X4 rotate = invRot.RequireRotationMatrix();

		R = XMLoadFloat4x4( &rotate );
	}

	XMMATRIX T = XMMatrixTranslation( -pos.x, -pos.y, -pos.z );

	return { T * R };
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

	// Use only Orbit-mode or Pan-mode.
	// Returns true if move-amount is too large.
	auto IsMouseLoopedH = [&]()
	{
		if ( moveMode == Mode::None ) { return false; }
		// else

		float judgeSize = Common::ScreenWidthF()  * 0.8f;
		
		if ( fabsf( mouse.current.x - mouse.prev.x ) < judgeSize ) { return false; }
		// else
		return true;
	};
	auto IsMouseLoopedV = [&]()
	{
		if ( moveMode == Mode::None ) { return false; }
		// else

		float judgeSize = Common::ScreenHeightF() * 0.8f;

		if ( fabsf( mouse.current.y - mouse.prev.y ) < judgeSize ) { return false; }
		// else
		return true;
	};

	float diff = 1.0f;
	if ( IsMouseLoopedH() )
	{
		float center = Common::HalfScreenWidthF();
		mouse.prev.x = mouse.current.x;
		mouse.prev.x += ( mouse.current.x < center ) ? -diff : diff;
	}
	if ( IsMouseLoopedV() )
	{
		float center = Common::HalfScreenHeightF();
		mouse.prev.y = mouse.current.y;
		mouse.prev.y += ( mouse.current.y < center ) ? -diff : diff;
	}
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
			if ( Donya::IsMouseHoveringImGuiWindow() ) { break; }
			// else

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

	// Norm() != 1
	if ( !ZeroEqual( posture.Length() - 1.0f ) )
	{
		posture.Normalize();
	}
}

void Camera::Zoom()
{
	if ( Donya::IsMouseHoveringImGuiWindow() ) { return; }
	// else

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

void Camera::OrbitAround()
{
	if ( mouse.prev == mouse.current ) { return; }
	// else

	constexpr float ROTATION_SPEED = 1.0f;

	/* // TODO:I wanna change the way of calculation of rotation-amount. that calculate by world-space mouse-position.
	Donya::Vector3 from	= ToWorldPos( mouse.prev	);
	Donya::Vector3 to	= ToWorldPos( mouse.current	);
	Donya::Vector3 diff = to - from;
	*/

	Donya::Vector2 diff = mouse.current - mouse.prev;
	
	// Rotate posture.
	if ( !ZeroEqual( diff.x ) )
	{
		float radian = ToRadian( diff.x * ROTATION_SPEED );
		Donya::Vector3 up = Donya::Vector3::Up(); // world-space axis.

		Donya::Quaternion rotate = Donya::Quaternion::Make( up, radian );
		posture = rotate * posture;
	}
	if ( !ZeroEqual( diff.y ) )
	{
		float radian = ToRadian( diff.y * ROTATION_SPEED );
		Donya::Vector3 right = posture.RotateVector( Donya::Vector3::Right() ); // camera-space axis.

		Donya::Quaternion rotate = Donya::Quaternion::Make( right, radian );
		posture = rotate * posture;
	}

	// Calculate front-vector. thereby I can decide camera-position.
	Donya::Vector3  front = posture.RotateVector( Donya::Vector3::Front() );
	if ( ZeroEqual( front.LengthSq() ) ) { return; }
	// else

	front *= radius;
	pos = focus + ( -front );
}

void Camera::Pan()
{
	if ( mouse.prev == mouse.current ) { return; }
	// else

	// If you want move to right, the camera(myself) must move to left.
	Donya::Vector2 old{ mouse.prev };
	Donya::Vector2 now( mouse.current );
	old.x *= -1;
	now.x *= -1;

	Donya::Vector3 from	= ToWorldPos( old );
	Donya::Vector3 to	= ToWorldPos( now );

	Donya::Vector3 moveVec = to - from;

	pos		+= moveVec;
	focus	+= moveVec;
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
		
		Multiply( &virtualPos, posture.RequireRotationMatrix() );

		worldPos = virtualPos;
	}

	float rayLength{}; // The "a" of reference site.
	{
		Donya::Vector3 anyPoint  = Donya::Vector3::Zero();
		Donya::Vector3 virNormal = pos - focus;
		float dotSample = Donya::Vector3::Dot( { anyPoint - pos }, virNormal );
		float dotTarget = Donya::Vector3::Dot( { worldPos - pos }, virNormal );

		rayLength = dotSample / dotTarget;
	}

	Donya::Vector3 onPlanePos = pos + ( Donya::Vector3{ worldPos - pos } * rayLength );
	return onPlanePos;
}

#if USE_IMGUI

void Camera::ShowParametersToImGui()
{
#if USE_IMGUI

	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"カメラ" ) )
		{
			if ( ImGui::TreeNode( u8"操作方法" ) )
			{
				std::string key		{ u8"\"R\" キー : 位置と注視点をリセット. " };
				std::string zoom	{ u8"マウスホイール : ズームイン・アウト." };
				std::string rotate	{ u8"左クリック(＋移動) : 注視点を軸に回転移動." };
				std::string pan		{ u8"ホイール押し込み（＋移動） or\n右クリック(＋移動) : 位置と注視点を移動." };

				auto ShowString = []( const std::string &str )
				{
					ImGui::Text( str.c_str() );
				};

				ShowString( key		);
				ShowString( zoom	);
				ShowString( rotate	);
				ShowString( pan		);

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( "Parameter" ) )
			{
				std::string vec3Info{ "[X:%5.3f][Y:%5.3f][Z:%5.3f]" };
				std::string vec4Info{ "[X:%5.3f][Y:%5.3f][Z:%5.3f][W:%5.3f]" };
				auto ShowVec3 = [&vec3Info]( std::string name, const Donya::Vector3 &param )
				{
					ImGui::Text( ( name + vec3Info ).c_str(), param.x, param.y, param.z );
				};
				auto ShowVec4 = [&vec4Info]( std::string name, const Donya::Vector4 &param )
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

			ImGui::TreePop();
		}

		ImGui::End();
	}

#endif // USE_IMGUI
}

#endif // USE_IMGUI