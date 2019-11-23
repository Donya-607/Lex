#include "camera.h"

#include <algorithm>
#include <array>
#include <string>

#include "Donya/Keyboard.h"
#include "Donya/Mouse.h"
#include "Donya/Useful.h"
#include "Donya/UseImGui.h"

#include "Common.h"

#undef min
#undef max

#pragma region BaseCamera

class ICamera::BaseCamera
{
public:
	struct Destination
	{
		Donya::Vector3		pos{};
		Donya::Vector3		focus{};
		Donya::Quaternion	orientation{};
	};
protected:
	ICamera::Configuration	m;			// Member. Represent current status(i.e.not destination).
	Destination				dest;		// Use for interpolation's destination.
	Donya::Vector4x4		projection;	// Store the calculate result.
public:
	BaseCamera() : m(), dest(), projection() {}
	virtual ~BaseCamera() = default;
public:
	virtual void Init( ICamera::Configuration member ) = 0;
	virtual void Uninit() = 0;

	virtual void Update( ICamera::Controller controller ) = 0;
public:
	virtual void SetZRange					( float zNear, float zFar )
	{
		m.zNear = zNear;
		m.zFar  = zFar;
	}
	virtual void SetFOV						( float FOV )
	{
		m.FOV = FOV;
	}
	virtual void SetScreenSize				( const Donya::Vector2 &screenSize )
	{
		_ASSERT_EXPR( !ZeroEqual( screenSize.y ), L"Error : Can not set zero to the camera's screen size y." );
		m.screenSize = screenSize;
	}
	virtual void SetPosition				( const Donya::Vector3 &point )
	{
		dest.pos = point;
	}
	virtual void SetFocusPoint				( const Donya::Vector3 &point )
	{
		dest.focus = point;
	}
	virtual void SetFocusToFront			( float distance )
	{
		dest.focus = dest.orientation.LocalFront() * distance;
	}
	virtual void SetOrientation				( const Donya::Quaternion &orientation )
	{
		dest.orientation = orientation;
	}
	/// <summary>
	/// If set { 0, 0 } to the "viewSize", use registered screen size.
	/// </summary>
	virtual void SetProjectionOrthographic	( const Donya::Vector2 &viewSize = { 0.0f, 0.0f } )
	{
		Donya::Vector2 size = ( viewSize.IsZero() ) ? m.screenSize : viewSize;
		projection = Donya::Vector4x4::FromMatrix
		(
			DirectX::XMMatrixOrthographicLH( size.x, size.y, m.zNear, m.zFar )
		);
	}
	/// <summary>
	/// If set 0.0f to the "aspectRatio", calculate by registered screen size.
	/// </summary>
	virtual void SetProjectionPerspective	( float aspectRatio = 0.0f )
	{
		float aspect = ( ZeroEqual( aspectRatio ) )
		? m.screenSize.x / m.screenSize.y
		: aspectRatio;

		projection = Donya::Vector4x4::FromMatrix
		(
			DirectX::XMMatrixPerspectiveFovLH( m.FOV, aspect, m.zNear, m.zFar )
		);
	}

	virtual float				GetFOV()				const { return m.FOV; }
	virtual float				GetZNear()				const { return m.zNear; }
	virtual float				GetZFar()				const { return m.zFar; }
	virtual Donya::Vector2		GetScreenSize()			const { return m.screenSize; }
	virtual Donya::Vector3		GetPosition()			const { return dest.pos; }
	virtual Donya::Vector3		GetFocusPoint()			const { return dest.focus; }
	virtual Donya::Quaternion	GetOrientation()		const { return dest.orientation; }
	virtual Donya::Vector4x4	CalcViewMatrix()		const
	{
		Donya::Quaternion invRotation = m.orientation.Conjugate(); // Conjugate() represent inverse rotation only when using to rotation.
		Donya::Vector4x4  I_R = invRotation.RequireRotationMatrix();
		Donya::Vector4x4  I_T = Donya::Vector4x4::MakeTranslation( -m.pos );

		return ( I_T * I_R );
	}
	virtual Donya::Vector4x4	GetProjectionMatrix()	const { return projection; }
protected:
	virtual void Interpolation( float lerpFactor )
	{
		auto LerpPoint = []( const Donya::Vector3 &start, const Donya::Vector3 &last, float factor )
		{
			const Donya::Vector3 diff = last - start;
			return start + ( diff * factor );
		};

		if ( dest.pos != m.pos )
		{
			m.pos = LerpPoint( m.pos, dest.pos, lerpFactor );
		}
		if ( dest.focus != m.focus )
		{
			m.focus = LerpPoint( m.focus, dest.focus, lerpFactor );
		}

		if ( !Donya::Quaternion::IsSameRotation( dest.orientation, m.orientation ) )
		{
			m.orientation = Donya::Quaternion::Slerp( m.orientation, dest.orientation, lerpFactor );
			m.orientation.Normalize();
		}
	}
public:
#if USE_IMGUI

	virtual void ShowImGuiNode()
	{
		if ( ImGui::TreeNode( u8"現在の状態（補間中）" ) )
		{
			const std::string vec3Info{ "[X:%5.3f][Y:%5.3f][Z:%5.3f]" };
			const std::string vec4Info{ "[X:%5.3f][Y:%5.3f][Z:%5.3f][W:%5.3f]" };
			auto ShowVec3 = [&vec3Info]( std::string name, const Donya::Vector3 &param )
			{
				ImGui::Text( ( name + vec3Info ).c_str(), param.x, param.y, param.z );
			};
			auto ShowVec4 = [&vec4Info]( std::string name, const Donya::Vector4 &param )
			{
				ImGui::Text( ( name + vec4Info ).c_str(), param.x, param.y, param.z, param.w );
			};
			auto ShowQuat = [&vec4Info]( std::string name, const Donya::Quaternion &param )
			{
				ImGui::Text( ( name + vec4Info ).c_str(), param.x, param.y, param.z, param.w );
			};

			ShowVec3( "Pos:",			m.pos	);
			ShowVec3( "Focus:",			m.focus	);
			
			ShowQuat( "Orientation:",	m.orientation				);
			ImGui::Text( "Norm:[%5.3f]",m.orientation.Length()		);
			ShowVec3( "Local.Up:",		m.orientation.LocalUp()		);
			ShowVec3( "Local.Right:",	m.orientation.LocalRight()	);
			ShowVec3( "Local.Front:",	m.orientation.LocalFront()	);

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"目標地点（補間後）" ) )
		{
			auto DragVec3 = []( std::string name, Donya::Vector3 *pV )
			{
				ImGui::DragFloat3( name.c_str(), &pV->x );
			};
			auto DragVec4 = []( std::string name, Donya::Vector4 *pV )
			{
				ImGui::DragFloat4( name.c_str(), &pV->x );
			};
			auto DragQuat = []( std::string name, Donya::Quaternion *pQ )
			{
				ImGui::SliderFloat4( name.c_str(), &pQ->x, -1.0f, 1.0f );
				if ( ImGui::Button( u8"正規化" ) )
				{
					pQ->Normalize();
				}
				if ( ImGui::Button( u8"初期化" ) )
				{
					*pQ = Donya::Quaternion::Identity();
				}
			};

			DragVec3( "Pos",	&dest.pos	);
			DragVec3( "Focus",	&dest.focus	);

			DragQuat( "Orientation", &dest.orientation );

			ImGui::TreePop();
		}
	}

#endif // USE_IMGUI
};

// BaseCamera
#pragma endregion

#pragma region FreeCamera

class FreeCamera : public ICamera::BaseCamera
{
private:
	float focusDistance;
public:
	FreeCamera() : BaseCamera(), focusDistance( 1.0f )
	{}
	virtual ~FreeCamera() = default;
public:
	void Init( ICamera::Configuration member ) override
	{
		m = member;
		SetProjectionPerspective();
	}
	void Uninit() override
	{
		// No op.
	}

	void Update( ICamera::Controller controller ) override
	{
		Move( controller );
		Rotate( controller );

		Interpolation( controller.slerpPercent );
	}
private:
	void UpdateFocusPoint()
	{
		dest.focus = dest.orientation.LocalFront() * focusDistance;
	}

	void Move( ICamera::Controller controller )
	{
		if ( controller.moveInLocalSpace )
		{
			controller.moveVelocity = dest.orientation.RotateVector( controller.moveVelocity );
		}

		dest.pos += controller.moveVelocity;
	}
	void Rotate( ICamera::Controller controller )
	{
		dest.orientation.RotateBy( controller.rotation );
	}
public:
	void SetFocusPoint( const Donya::Vector3 &point ) override
	{
		// Discard this method.
	}
	void SetFocusToFront( float distance ) override
	{
		focusDistance = distance;
		UpdateFocusPoint();
	}
};

// FreeCamera
#pragma endregion

#pragma region LookCamera

class LookCamera : public ICamera::BaseCamera
{
public:
	LookCamera() : BaseCamera()
	{}
	~LookCamera() = default;
public:
	void Init( ICamera::Configuration member ) override
	{
		m = member;
		SetProjectionPerspective();
	}
	void Uninit() override
	{
		// No op.
	}

	void Update( ICamera::Controller controller ) override
	{

	}
private:

public:
#if USE_IMGUI

	void ShowImGuiNode() override
	{

	}

#endif // USE_IMGUI
};

// LookCamera
#pragma endregion

#pragma region ICamera

ICamera::ICamera() :
	pCamera( nullptr ),
	currentMode()
{}
ICamera::~ICamera() = default;

void ICamera::Init( Mode initialMode )
{
	// Set temporary default camera. Because the "ChangeMode()" require the "pCamera" is not null.
	// A default projection matrix will be setting to perspective at Init().

	constexpr Configuration DEFAULT_CONFIG
	{
		ToRadian( 30.0f ),				// FOV
		0.1f, 1000.0f,					// zNear, zFar
		{ 1920.0f, 1080.0f },			// screenSize
		{ 0.0f, 0.0f, 0.0f },			// pos
		{ 0.0f, 0.0f, 1.0f },			// focus
		Donya::Quaternion::Identity()	// orientation
	};
	pCamera = std::make_unique<FreeCamera>();
	pCamera->Init( DEFAULT_CONFIG );

	ChangeMode( initialMode );
}
void ICamera::Uninit()
{
	AssertIfNullptr();
	pCamera->Uninit();
}

void ICamera::Update( Controller ctrl )
{
	AssertIfNullptr();
	pCamera->Update( ctrl );
}

void ICamera::ChangeMode( Mode nextMode )
{
	currentMode = nextMode;

	Configuration storage = BuildCurrentConfiguration();

	pCamera->Uninit();

	switch ( nextMode )
	{
	case ICamera::Mode::Free:
		pCamera = std::make_unique<FreeCamera>();
		break;
	case ICamera::Mode::Look:
		pCamera = std::make_unique<LookCamera>();
		break;
	default: _ASSERT_EXPR( 0, L"Error : The camera was specified unexpected mode." ); return;
	}

	pCamera->Init( storage );
}

void ICamera::SetZRange( float zNear, float zFar )
{
	AssertIfNullptr();
	pCamera->SetZRange( zNear, zFar );
}
void ICamera::SetFOV( float FOV )
{
	AssertIfNullptr();
	pCamera->SetFOV( FOV );
}
void ICamera::SetScreenSize( const Donya::Vector2 &screenSize )
{
	AssertIfNullptr();
	pCamera->SetScreenSize( screenSize );
}
void ICamera::SetPosition( const Donya::Vector3 &point )
{
	AssertIfNullptr();
	pCamera->SetPosition( point );
}
void ICamera::SetFocusPoint( const Donya::Vector3 &point )
{
	AssertIfNullptr();
	pCamera->SetFocusPoint( point );
}
void ICamera::SetFocusToFront( float distance )
{
	AssertIfNullptr();
	pCamera->SetFocusToFront( distance );
}
void ICamera::SetOrientation( const Donya::Quaternion &orientation )
{
	AssertIfNullptr();
	pCamera->SetOrientation( orientation );
}
void ICamera::SetProjectionOrthographic( const Donya::Vector2 &viewSize )
{
	AssertIfNullptr();
	pCamera->SetProjectionOrthographic( viewSize );
}
void ICamera::SetProjectionPerspective( float aspectRatio )
{
	AssertIfNullptr();
	pCamera->SetProjectionPerspective( aspectRatio );
}

Donya::Vector3		ICamera::GetPosition()			const
{
	AssertIfNullptr();
	pCamera->GetPosition();
}
Donya::Vector3		ICamera::GetFocusPoint()		const
{
	AssertIfNullptr();
	pCamera->GetFocusPoint();
}
Donya::Quaternion	ICamera::GetOrientation()		const
{
	AssertIfNullptr();
	pCamera->GetOrientation();
}
Donya::Vector4x4	ICamera::CalcViewMatrix()		const
{
	AssertIfNullptr();
	pCamera->CalcViewMatrix();
}
Donya::Vector4x4	ICamera::GetProjectionMatrix()	const
{
	AssertIfNullptr();
	pCamera->GetProjectionMatrix();
}

#if USE_IMGUI

void ICamera::ShowImGuiNode()
{
	AssertIfNullptr();
	pCamera->ShowImGuiNode();
}

#endif // USE_IMGUI

void ICamera::AssertIfNullptr() const
{
	_ASSERT_EXPR( pCamera, L"Error : The camera was not initialized." );
}

ICamera::Configuration ICamera::BuildCurrentConfiguration() const
{
	AssertIfNullptr();

	Configuration config{};
	config.FOV			= pCamera->GetFOV();
	config.zNear		= pCamera->GetZNear();
	config.zFar			= pCamera->GetZFar();
	config.screenSize	= pCamera->GetScreenSize();
	config.pos			= pCamera->GetPosition();
	config.focus		= pCamera->GetFocusPoint();
	config.orientation	= pCamera->GetOrientation();
	return config;
}

// ICamera
#pragma endregion

// Donya's version.

//using namespace DirectX;
//
//Camera::Camera() :
//	focusDistance(),
//	scopeAngle(),
//	screenSize( 1920.0f, 1080.0f ), halfScreenSize(),
//	pos(), focus(), velocity(),
//	orientation(),
//	projection()
//{
//	focus = pos + Donya::Vector3::Front();
//}
//Camera::~Camera() = default;
//
//void Camera::Init( float screenWidth, float screenHeight, float scopeAngle )
//{
//	SetScreenSize( screenWidth, screenHeight );
//	SetScopeAngle( scopeAngle );
//	ResetPerspectiveProjection();
//	ResetOrientation();
//}
//
//void Camera::SetScreenSize( float newScreenWidth, float newScreenHeight )
//{
//	SetScreenSize( Donya::Vector2{ newScreenWidth, newScreenHeight } );
//}
//void Camera::SetScreenSize( Donya::Vector2 newScreenSize )
//{
//	screenSize = newScreenSize;
//	halfScreenSize = screenSize * 0.5f;
//}
//
//void Camera::SetPosition( Donya::Vector3 newPosition )
//{
//	if ( IsFocusFixed() )
//	{
//		if ( newPosition == focus ) // Prevent "pos" and "focus" the same.
//		{
//			Donya::Vector3 localFront = orientation.LocalFront();
//			localFront.Normalize();
//			newPosition -= localFront;
//		}
//
//		pos = newPosition;
//		LookAtFocus();
//	}
//	else
//	{
//		pos = newPosition;
//		SetFocusToFront();
//	}
//}
//
//void Camera::SetScopeAngle( float scope )
//{
//	scopeAngle = scope;
//}
//
//void Camera::SetFocusDistance( float distance )
//{
//	focusDistance = distance;
//	SetFocusToFront();
//}
//void Camera::SetFocusCoordinate( const Donya::Vector3 &coordinate )
//{
//	Donya::Vector3 newFocus = coordinate;
//	if ( newFocus == pos ) // Prevent "pos" and "focus" the same.
//	{
//		Donya::Vector3 localFront = orientation.LocalFront();
//		localFront.Normalize();
//		newFocus += localFront;
//	}
//
//	focus = newFocus;
//	focusDistance = 0.0f;
//
//	LookAtFocus();
//}
//
//void Camera::ResetOrthographicProjection()
//{
//	SetOrthographicProjectionMatrix( 16.0f, 9.0f, 0.01f, 1000.0f );
//}
//void Camera::ResetPerspectiveProjection()
//{
//	_ASSERT_EXPR( !ZeroEqual( screenSize.y ), L"Zero-divide detected!" );
//	float aspectRatio = screenSize.x / screenSize.y;
//	float mostNear = 0.1f;
//	float mostFar = 1000.0f;
//
//	SetPerspectiveProjectionMatrix( scopeAngle, aspectRatio, mostNear, mostFar );
//}
//
//Donya::Vector4x4 Camera::SetOrthographicProjectionMatrix( float width, float height, float mostNear, float mostFar )
//{
//	XMStoreFloat4x4( &projection, XMMatrixOrthographicLH( width, height, mostNear, mostFar ) );
//	return GetProjectionMatrix();
//}
//Donya::Vector4x4 Camera::SetPerspectiveProjectionMatrix( float aspectRatio )
//{
//	return SetPerspectiveProjectionMatrix( scopeAngle, aspectRatio, 0.1f, 1000.0f );
//}
//Donya::Vector4x4 Camera::SetPerspectiveProjectionMatrix( float scopeAngle, float aspectRatio, float mostNear, float mostFar )
//{
//	XMStoreFloat4x4( &projection, XMMatrixPerspectiveFovLH( scopeAngle, aspectRatio, mostNear, mostFar ) );
//	return GetProjectionMatrix();
//}
//
//Donya::Vector4x4 Camera::CalcViewMatrix() const
//{
//	Donya::Vector4x4 R{};
//	{
//		Donya::Quaternion invRot = orientation.Conjugate();
//		R = invRot.RequireRotationMatrix();
//	}
//
//	Donya::Vector4x4 T = Donya::Vector4x4::MakeTranslation( -pos );
//
//	return { T * R };
//}
//
//void Camera::Update( Controller controller )
//{
//	Move( controller );
//
//	Rotate( controller );
//}
//
//// Interpolate is not used in current implement.
///*
//void Camera::Interpolate()
//{
//	constexpr size_t SIZE = 1;
//	const std::array<DirectX::XMFLOAT3 *, SIZE> DEST =
//	{
//		&destPos,
//		&destFocus
//	};
//	const std::array<DirectX::XMFLOAT3 *, SIZE> POS =
//	{
//		&pos,
//		&focus
//	};
//
//	for ( size_t i = 0; i < SIZE; ++i )
//	{
//		DirectX::XMFLOAT3 diff = Sub( *DEST[i], *POS[i] );
//		if ( fabsf( Length( diff ) ) < FLT_EPSILON )
//		{
//			*POS[i] = *DEST[i];
//			return;
//		}
//		// else
//
//		diff = Multi( diff, chaseSpeed );
//
//		*POS[i] = Add( *POS[i], diff );
//	}
//}
//*/
//
//bool Camera::IsFocusFixed() const
//{
//	return ( ZeroEqual( focusDistance ) ) ? true : false;
//}
//
//void Camera::ResetOrientation()
//{
//	orientation = Donya::Quaternion::Identity();
//}
//
//void Camera::NormalizePostureIfNeeded()
//{
//	// Norm() != 1
//	if ( !ZeroEqual( orientation.Length() - 1.0f ) )
//	{
//		orientation.Normalize();
//	}
//}
//
//void Camera::SetFocusToFront()
//{
//	Donya::Vector3 localFront = orientation.RotateVector( Donya::Vector3::Front() );
//
//	localFront.Normalize();
//	localFront *= focusDistance;
//
//	focus = pos + localFront;
//}
//
//void Camera::LookAtFocus()
//{
//	Donya::Vector3 look = focus - pos;
//	look.Normalize();
//
//	orientation = Donya::Quaternion::LookAt( orientation, look );
//	NormalizePostureIfNeeded();
//}
//
//void Camera::Move( Controller controller )
//{
//	if ( !controller.moveVelocity.IsZero() )
//	{
//		Zoom( controller );
//		Pan( controller );
//	}
//
//
//
//	if ( controller.moveAtLocalSpace )
//	{
//		controller.moveVelocity = orientation.RotateVector( controller.moveVelocity );
//	}
//
//	pos += controller.moveVelocity;
//	if ( IsFocusFixed() )
//	{
//		LookAtFocus();
//	}
//	else
//	{
//		SetFocusToFront();
//	}
//
//	// Norm() != 1
//	if ( !ZeroEqual( orientation.Length() - 1.0f ) )
//	{
//		orientation.Normalize();
//	}
//}
//
//void Camera::Rotate( Controller controller )
//{
//	if ( controller.rotation.IsZero() ) { return; }
//	// else
//
//	auto GetAxis = []( const Donya::Quaternion &q, Donya::Vector3 axis, bool inLocalSpace )
//	{
//		if ( inLocalSpace )
//		{
//			axis = q.RotateVector( axis );
//		}
//
//		return axis;
//	};
//
//	Donya::Quaternion resultPosture = orientation;
//
//	if ( !ZeroEqual( controller.rotation.x ) )
//	{
//		Donya::Vector3 axis = GetAxis( resultPosture, Donya::Vector3::Up(), /* inLocalSpace = */ false );
//		Donya::Quaternion rotate = Donya::Quaternion::Make( axis, controller.rotation.x );
//
//		resultPosture = rotate * resultPosture;
//	}
//	if ( !ZeroEqual( controller.rotation.y ) )
//	{
//		Donya::Vector3 axis = GetAxis( resultPosture, Donya::Vector3::Right(), /* inLocalSpace = */ true );
//		Donya::Quaternion rotate = Donya::Quaternion::Make( axis, controller.rotation.y );
//
//		resultPosture = rotate * resultPosture;
//	}
//	if ( !ZeroEqual( controller.rotation.z ) )
//	{
//		Donya::Vector3 axis = GetAxis( resultPosture, Donya::Vector3::Front(), /* inLocalSpace = */ true );
//		Donya::Quaternion rotate = Donya::Quaternion::Make( axis, controller.rotation.z );
//
//		resultPosture = rotate * resultPosture;
//	}
//
//	// orientation = Donya::Quaternion::Slerp( orientation, resultPosture, controller.slerpPercent );
//	orientation = resultPosture;
//
//	// Norm() != 1
//	if ( !ZeroEqual( orientation.Length() - 1.0f ) )
//	{
//		orientation.Normalize();
//	}
//
//	if ( IsFocusFixed() )
//	{
//		LookAtFocus();
//	}
//	else
//	{
//		SetFocusToFront();
//	}
//
//	// Norm() != 1
//	if ( !ZeroEqual( orientation.Length() - 1.0f ) )
//	{
//		orientation.Normalize();
//	}
//}
//
//#if DEBUG_MODE
//
//void Camera::ShowParametersToImGui()
//{
//#if USE_IMGUI
//
//	if ( ImGui::BeginIfAllowed() )
//	{
//		if ( ImGui::TreeNode( u8"３Ｄ・カメラ" ) )
//		{
//			if ( ImGui::TreeNode( u8"操作方法" ) )
//			{
//				std::string key{ u8"\"R\" キー : 位置と注視点をリセット. " };
//				std::string zoom{ u8"マウスホイール : ズームイン・アウト." };
//				std::string rotate{ u8"左クリック(＋移動) : 注視点を軸に回転移動." };
//				std::string pan{ u8"ホイール押し込み（＋移動） or\n右クリック(＋移動) : 位置と注視点を移動." };
//
//				auto ShowString = []( const std::string &str )
//				{
//					ImGui::Text( str.c_str() );
//				};
//
//				ShowString( key );
//				ShowString( zoom );
//				ShowString( rotate );
//				ShowString( pan );
//
//				ImGui::TreePop();
//			}
//
//			if ( ImGui::TreeNode( "Parameter" ) )
//			{
//				std::string vec3Info{ "[X:%5.3f][Y:%5.3f][Z:%5.3f]" };
//				std::string vec4Info{ "[X:%5.3f][Y:%5.3f][Z:%5.3f][W:%5.3f]" };
//				auto ShowVec3 = [&vec3Info]( std::string name, const Donya::Vector3 &param )
//				{
//					ImGui::Text( ( name + vec3Info ).c_str(), param.x, param.y, param.z );
//				};
//				auto ShowVec4 = [&vec4Info]( std::string name, const Donya::Vector4 &param )
//				{
//					ImGui::Text( ( name + vec4Info ).c_str(), param.x, param.y, param.z, param.w );
//				};
//
//				Donya::Vector3 up = orientation.RotateVector( Donya::Vector3::Up() );
//				Donya::Vector3 right = orientation.RotateVector( Donya::Vector3::Right() );
//				Donya::Vector3 front = orientation.RotateVector( Donya::Vector3::Front() );
//
//				ShowVec3( "Pos", pos );
//				ShowVec3( "Focus", focus );
//				ShowVec3( "Up", up );
//				ShowVec3( "Right", right );
//				ShowVec3( "Front", front );
//
//				Donya::Vector4 vec4Posture
//				{
//					orientation.x,
//					orientation.y,
//					orientation.z,
//					orientation.w
//				};
//				ShowVec4( "Posture", vec4Posture );
//				ImGui::Text( "Norm[%5.3f]", orientation.Length() );
//
//				ImGui::TreePop();
//			}
//
//			ImGui::TreePop();
//		}
//
//		ImGui::End();
//	}
//
//#endif // USE_IMGUI
//}
//
//#endif // DEBUG_MODE

// Old Lex version.

//
//Camera::Camera() :
//	moveMode( Mode::None ),
//	radius(),
//	scopeAngle(),
//	pos(), focus(), velocity(),
//	mouse(),
//	orientation(),
//	projection()
//{
//	SetScopeAngle( ToRadian( 30.0f ) ); // default-value.
//	ResetPerspectiveProjection();
//}
//Camera::Camera( float scope ) :
//	moveMode( Mode::None ),
//	radius(),
//	scopeAngle(),
//	pos(), focus(), velocity(),
//	mouse(),
//	orientation(),
//	projection()
//{
//	SetScopeAngle( scope );
//	ResetPerspectiveProjection();
//}
//
//void Camera::SetToHomePosition( Donya::Vector3 homePosition, Donya::Vector3 homeFocus )
//{
//	pos		= homePosition;
//	focus	= homeFocus;
//	
//	Donya::Vector3 dir = focus - pos; dir.Normalize();
//	orientation = Donya::Quaternion::LookAt( orientation, dir );
//}
//
//void Camera::SetScopeAngle( float scope )
//{
//	scopeAngle = scope;
//	CalcDistToVirtualScreen();
//}
//
//void Camera::ResetOrthographicProjection()
//{
//	SetOrthographicProjectionMatrix( 16.0f, 9.0f, 0.01f, 1000.0f );
//}
//void Camera::ResetPerspectiveProjection()
//{
//	float aspectRatio = Common::ScreenWidthF() / Common::ScreenHeightF();
//	float mostNear = 0.1f;
//	float mostFar = 1000.0f;
//
//	SetPerspectiveProjectionMatrix( scopeAngle, aspectRatio, mostNear, mostFar );
//}
//
//Donya::Vector4x4 Camera::SetOrthographicProjectionMatrix( float width, float height, float mostNear, float mostFar )
//{
//	XMStoreFloat4x4( &projection, XMMatrixOrthographicLH( width, height, mostNear, mostFar ) );
//	return GetProjectionMatrix();
//}
//Donya::Vector4x4 Camera::SetPerspectiveProjectionMatrix( float aspectRatio )
//{
//	return SetPerspectiveProjectionMatrix( scopeAngle, aspectRatio, 0.1f, 1000.0f );
//}
//Donya::Vector4x4 Camera::SetPerspectiveProjectionMatrix( float scopeAngle, float aspectRatio, float mostNear, float mostFar )
//{
//	XMStoreFloat4x4( &projection, XMMatrixPerspectiveFovLH( scopeAngle, aspectRatio, mostNear, mostFar ) );
//	return GetProjectionMatrix();
//}
//
//Donya::Vector4x4 Camera::CalcViewMatrix() const
//{
//	Donya::Vector4x4 R{};
//	{
//		Donya::Quaternion invRot = orientation.Conjugate();
//		R = invRot.RequireRotationMatrix();
//	}
//
//	Donya::Vector4x4 T = Donya::Vector4x4::MakeTranslation( -pos );
//
//	return { T * R };
//}
//
//void Camera::Update( const Donya::Vector3 &targetPos )
//{
//	MouseUpdate();
//
//	ChangeMode();
//
//	Move( targetPos );
//}
//
// Interpolate is not used in current implement.
///*
//void Camera::Interpolate()
//{
//	constexpr size_t SIZE = 1;
//	const std::array<DirectX::XMFLOAT3 *, SIZE> DEST =
//	{
//		&destPos,
//		&destFocus
//	};
//	const std::array<DirectX::XMFLOAT3 *, SIZE> POS =
//	{
//		&pos,
//		&focus
//	};
//
//	for ( size_t i = 0; i < SIZE; ++i )
//	{
//		DirectX::XMFLOAT3 diff = Sub( *DEST[i], *POS[i] );
//		if ( fabsf( Length( diff ) ) < FLT_EPSILON )
//		{
//			*POS[i] = *DEST[i];
//			return;
//		}
//		 else
//
//		diff = Multi( diff, chaseSpeed );
//
//		*POS[i] = Add( *POS[i], diff );
//	}
//}
//*/
//
//void Camera::MouseUpdate()
//{
//	mouse.prev = mouse.current;
//
//	Donya::Mouse::Coordinate( &mouse.current.x, &mouse.current.y );
//
//	 Use only Orbit-mode or Pan-mode.
//	 Returns true if move-amount is too large.
//	auto IsMouseLoopedH = [&]()
//	{
//		if ( moveMode == Mode::None ) { return false; }
//		 else
//
//		float judgeSize = Common::ScreenWidthF()  * 0.8f;
//		
//		if ( fabsf( mouse.current.x - mouse.prev.x ) < judgeSize ) { return false; }
//		 else
//		return true;
//	};
//	auto IsMouseLoopedV = [&]()
//	{
//		if ( moveMode == Mode::None ) { return false; }
//		 else
//
//		float judgeSize = Common::ScreenHeightF() * 0.8f;
//
//		if ( fabsf( mouse.current.y - mouse.prev.y ) < judgeSize ) { return false; }
//		 else
//		return true;
//	};
//
//	float diff = 1.0f;
//	if ( IsMouseLoopedH() )
//	{
//		float center = Common::HalfScreenWidthF();
//		mouse.prev.x = mouse.current.x;
//		mouse.prev.x += ( mouse.current.x < center ) ? -diff : diff;
//	}
//	if ( IsMouseLoopedV() )
//	{
//		float center = Common::HalfScreenHeightF();
//		mouse.prev.y = mouse.current.y;
//		mouse.prev.y += ( mouse.current.y < center ) ? -diff : diff;
//	}
//}
//
//void Camera::ChangeMode()
//{
//	enum PressState { Left = 0, Middle = 1, Right = 2 };
//	const std::array<bool, 3> isPressed
//	{
//		Donya::Keyboard::Press( VK_LBUTTON ),
//		Donya::Keyboard::Press( VK_MBUTTON ),
//		Donya::Keyboard::Press( VK_RBUTTON )
//	};
//
//	switch ( moveMode )
//	{
//	case Mode::None:
//		{
//			if ( Donya::IsMouseHoveringImGuiWindow() ) { break; }
//			 else
//
//			 The mouse-button state is same as keyboard's state.
//
//			if ( isPressed[Left] )
//			{
//				moveMode = Mode::OrbitAround;
//			}
//			if ( isPressed[Middle] || isPressed[Right] )
//			{
//				moveMode = Mode::Pan;
//			}
//
//		}
//		break;
//	case Mode::OrbitAround:
//		{
//			if ( !isPressed[Left] )
//			{
//				moveMode = Mode::None;
//			}
//		}
//		break;
//	case Mode::Pan:
//		{
//			if ( !( isPressed[Middle] || isPressed[Right] ) )
//			{
//				moveMode = Mode::None;
//			}
//		}
//		break;
//	}
//}
//
//void SetVelocity( Donya::Vector3 *pVelocity )
//{
//	static Donya::Vector2 prevMouse{};
//	static Donya::Vector2 currentMouse{};
//
//	prevMouse = currentMouse;
//
//	Donya::Mouse::Coordinate( &currentMouse.x, &currentMouse.y );
//
//	auto SetLessOrGreater =
//	[]( float *pOut, float judgeNo )
//	{
//		if ( judgeNo < 0.0f )
//		{
//			*pOut = -1.0f;
//		}
//		else
//		if ( 0.0f < judgeNo )
//		{
//			*pOut = 1.0f;
//		}
//	};
//
//	Donya::Vector2 diff = currentMouse - prevMouse;
//	SetLessOrGreater( &pVelocity->x, diff.x );
//	SetLessOrGreater( &pVelocity->y, diff.y );
//}
//
//void Camera::Move( const Donya::Vector3 &targetPos )
//{
//	switch ( moveMode )
//	{
//	case Mode::None:		Zoom();			break;
//	case Mode::OrbitAround:	OrbitAround();	break;
//	case Mode::Pan:			Pan();			break;
//	}
//
//	 Norm() != 1
//	if ( !ZeroEqual( orientation.Length() - 1.0f ) )
//	{
//		orientation.Normalize();
//	}
//}
//
//void Camera::Zoom()
//{
//	if ( Donya::IsMouseHoveringImGuiWindow() ) { return; }
//	 else
//
//	velocity = pos - focus;
//
//	int rot = Donya::Mouse::WheelRot();
//	if ( !rot ) { return; }
//	 else
//	
//	rot *= -1; // I want the distance to get closer as "rot" gets bigger.
//
//	constexpr float MAGNI = 0.1f;
//	float addition = scast<float>( rot ) * MAGNI;
//
//	velocity *= 1.0f + addition;
//	if ( ZeroEqual( velocity.LengthSq() ) ) { return; }
//	 else
//
//	pos = focus + velocity;
//	radius = Donya::Vector3{ pos - focus }.Length();
//}
//
//void Multiply( XMFLOAT3 *pOutput, const Donya::Vector4x4 &M )
//{
//	XMFLOAT3 &V = *pOutput;
//
//	float x = ( V.x * M._11 ) + ( V.y * M._21 ) + ( V.z * M._31 );
//	float y = ( V.x * M._12 ) + ( V.y * M._22 ) + ( V.z * M._32 );
//	float z = ( V.x * M._13 ) + ( V.y * M._23 ) + ( V.z * M._33 );
//
//	V.x = x;
//	V.y = y;
//	V.z = z;
//}
//
//void Camera::OrbitAround()
//{
//	if ( mouse.prev == mouse.current ) { return; }
//	 else
//
//	constexpr float ROTATION_SPEED = 1.0f;
//
//	/* // TODO:I wanna change the way of calculation of rotation-amount. that calculate by world-space mouse-position.
//	Donya::Vector3 from	= ToWorldPos( mouse.prev	);
//	Donya::Vector3 to	= ToWorldPos( mouse.current	);
//	Donya::Vector3 diff = to - from;
//	*/
//
//	Donya::Vector2 diff = mouse.current - mouse.prev;
//	
//	 Rotate orientation.
//	if ( !ZeroEqual( diff.x ) )
//	{
//		float radian = ToRadian( diff.x * ROTATION_SPEED );
//		Donya::Vector3 up = Donya::Vector3::Up(); // world-space axis.
//
//		Donya::Quaternion rotate = Donya::Quaternion::Make( up, radian );
//		orientation = rotate * orientation;
//	}
//	if ( !ZeroEqual( diff.y ) )
//	{
//		float radian = ToRadian( diff.y * ROTATION_SPEED );
//		Donya::Vector3 right = orientation.RotateVector( Donya::Vector3::Right() ); // camera-space axis.
//
//		Donya::Quaternion rotate = Donya::Quaternion::Make( right, radian );
//		orientation = rotate * orientation;
//	}
//
//	 Calculate front-vector. thereby I can decide camera-position.
//	Donya::Vector3  front = orientation.RotateVector( Donya::Vector3::Front() );
//	if ( ZeroEqual( front.LengthSq() ) ) { return; }
//	 else
//
//	front *= radius;
//	pos = focus + ( -front );
//}
//
//void Camera::Pan()
//{
//	if ( mouse.prev == mouse.current ) { return; }
//	 else
//
//	 If you want move to right, the camera(myself) must move to left.
//	Donya::Vector2 old{ mouse.prev };
//	Donya::Vector2 now( mouse.current );
//	old.x *= -1;
//	now.x *= -1;
//
//	Donya::Vector3 from	= ToWorldPos( old );
//	Donya::Vector3 to	= ToWorldPos( now );
//
//	Donya::Vector3 moveVec = to - from;
//
//	pos		+= moveVec;
//	focus	+= moveVec;
//}
//
//void Camera::CalcDistToVirtualScreen()
//{
//	 see http://marupeke296.com/ALG_No7_MoveCameraWithCursor.html
//
//	float FOV = scopeAngle;
//	FOV *= 0.5f;
//
//	virtualDistance = Common::ScreenHeightF() / ( 2 * tanf( FOV ) );
//}
//
//Donya::Vector3 Camera::ToWorldPos( const Donya::Vector2 &screenPos )
//{
//	 see http://marupeke296.com/ALG_No7_MoveCameraWithCursor.html
//
//	Donya::Vector3 worldPos{};
//	{
//		Donya::Vector3 virtualPos{ screenPos.x, screenPos.y, virtualDistance };
//		
//		Multiply( &virtualPos, orientation.RequireRotationMatrix() );
//
//		worldPos = virtualPos;
//	}
//
//	float rayLength{}; // The "a" of reference site.
//	{
//		Donya::Vector3 anyPoint  = Donya::Vector3::Zero();
//		Donya::Vector3 virNormal = pos - focus;
//		float dotSample = Donya::Vector3::Dot( { anyPoint - pos }, virNormal );
//		float dotTarget = Donya::Vector3::Dot( { worldPos - pos }, virNormal );
//
//		rayLength = dotSample / dotTarget;
//	}
//
//	Donya::Vector3 onPlanePos = pos + ( Donya::Vector3{ worldPos - pos } * rayLength );
//	return onPlanePos;
//}
//
//#if USE_IMGUI
//
//void Camera::ShowImGuiNode()
//{
//	if ( ImGui::TreeNode( u8"カメラ" ) )
//	{
//		if ( ImGui::TreeNode( u8"操作方法" ) )
//		{
//			std::string key		{ u8"\"R\" キー : 位置と注視点をリセット. " };
//			std::string zoom	{ u8"マウスホイール : ズームイン・アウト." };
//			std::string rotate	{ u8"左クリック(＋移動) : 注視点を軸に回転移動." };
//			std::string pan		{ u8"ホイール押し込み（＋移動） or\n右クリック(＋移動) : 位置と注視点を移動." };
//
//			auto ShowString = []( const std::string &str )
//			{
//				ImGui::Text( str.c_str() );
//			};
//
//			ShowString( key		);
//			ShowString( zoom	);
//			ShowString( rotate	);
//			ShowString( pan		);
//
//			ImGui::TreePop();
//		}
//
//		if ( ImGui::TreeNode( "Parameter" ) )
//		{
//			std::string vec3Info{ "[X:%5.3f][Y:%5.3f][Z:%5.3f]" };
//			std::string vec4Info{ "[X:%5.3f][Y:%5.3f][Z:%5.3f][W:%5.3f]" };
//			auto ShowVec3 = [&vec3Info]( std::string name, const Donya::Vector3 &param )
//			{
//				ImGui::Text( ( name + vec3Info ).c_str(), param.x, param.y, param.z );
//			};
//			auto ShowVec4 = [&vec4Info]( std::string name, const Donya::Vector4 &param )
//			{
//				ImGui::Text( ( name + vec4Info ).c_str(), param.x, param.y, param.z, param.w );
//			};
//
//			Donya::Vector3 up		= orientation.RotateVector( Donya::Vector3::Up()	);
//			Donya::Vector3 right	= orientation.RotateVector( Donya::Vector3::Right()	);
//			Donya::Vector3 front	= orientation.RotateVector( Donya::Vector3::Front()	);
//
//			ShowVec3( "Pos",	pos		);
//			ShowVec3( "Focus",	focus	);
//			ShowVec3( "Up",		up		);
//			ShowVec3( "Right",	right	);
//			ShowVec3( "Front",	front	);
//			
//			Donya::Vector4 vec4Posture
//			{
//				orientation.x,
//				orientation.y,
//				orientation.z,
//				orientation.w
//			};
//			ShowVec4( "Posture", vec4Posture );
//			ImGui::Text( "Norm[%5.3f]", orientation.Length() );
//
//			ImGui::TreePop();
//		}
//
//		ImGui::TreePop();
//	}
//}
//
//#endif // USE_IMGUI
