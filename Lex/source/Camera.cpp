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
		LookAtDestFocus();
	}
	virtual void SetFocusPoint				( const Donya::Vector3 &point )
	{
		dest.focus = point;
		LookAtDestFocus();
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
	virtual Donya::Vector3		GetPosition()			const { return m.pos; }
	virtual Donya::Vector3		GetFocusPoint()			const { return m.focus; }
	virtual Donya::Quaternion	GetOrientation()		const { return m.orientation; }
	virtual Donya::Vector4x4	CalcViewMatrix()		const
	{
		Donya::Quaternion invRotation = m.orientation.Conjugate(); // Conjugate() represent inverse rotation only when using to rotation.
		Donya::Vector4x4  I_R = invRotation.RequireRotationMatrix();
		Donya::Vector4x4  I_T = Donya::Vector4x4::MakeTranslation( -m.pos );

		return ( I_T * I_R );
	}
	virtual Donya::Vector4x4	GetProjectionMatrix()	const { return projection; }
protected:
	/// <summary>
	/// The dest.orientation will look to dest.focus by dest.pos.
	/// </summary>
	virtual void LookAtDestFocus()
	{
		const Donya::Vector3 nLookDir = ( dest.focus - dest.pos ).Unit();
		dest.orientation = Donya::Quaternion::LookAt( dest.orientation, nLookDir, Donya::Quaternion::Freeze::Front );
	}
	virtual void AssignMemberToDestination()
	{
		dest.pos			= m.pos;
		dest.focus			= m.focus;
		dest.orientation	= m.orientation;
	}
	virtual void InterpolateMember( float lerpFactor )
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
	virtual void RotateByRollPitchYaw( ICamera::Controller controller )
	{
		// HACK : The rotation axes should fetch from rotated orientation ? or it is OK to use prepared axes ?

		auto RotateIfNotZero = [&]( const Donya::Vector3 &axis, float angle )
		{
			if ( ZeroEqual( angle ) ) { return; }
			// else

			const Donya::Quaternion rotation = Donya::Quaternion::Make( axis, angle );
			dest.orientation.RotateBy( rotation );
			dest.orientation.Normalize();
		};

		RotateIfNotZero( dest.orientation.LocalFront(),	controller.roll  );
		RotateIfNotZero( dest.orientation.LocalRight(),	controller.pitch );
		RotateIfNotZero( Donya::Vector3::Up(),			controller.yaw   );
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
			if ( ImGui::Button( u8"初期化" ) )
			{
				dest.pos			= Donya::Vector3{ 0.0f, 0.0f,-1.0f };
				dest.focus			= Donya::Vector3{ 0.0f, 0.0f, 0.0f };
				dest.orientation	= Donya::Quaternion::Identity();
			}

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

		AssignMemberToDestination();

		focusDistance = ( m.focus - m.pos ).Length();
		UpdateFocusPoint();
	}
	void Uninit() override
	{
		// No op.
	}

	void Update( ICamera::Controller controller ) override
	{
		Move( controller );
		RotateByRollPitchYaw( controller );

		InterpolateMember( controller.slerpPercent );
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

		AssignMemberToDestination();
	}
	void Uninit() override
	{
		// No op.
	}

	void Update( ICamera::Controller controller ) override
	{
		Move( controller );

		InterpolateMember( controller.slerpPercent );
	}
private:
	void Move( ICamera::Controller controller )
	{
		if ( controller.moveInLocalSpace )
		{
			controller.moveVelocity = dest.orientation.RotateVector( controller.moveVelocity );
		}

		dest.pos += controller.moveVelocity;
	}
};

// LookCamera
#pragma endregion

#pragma region SatelliteCamera

class SatelliteCamera : public ICamera::BaseCamera
{
private:
	float focusDistance;
public:
	SatelliteCamera() : BaseCamera(), focusDistance( 1.0f )
	{}
	~SatelliteCamera() = default;
public:
	void Init( ICamera::Configuration member ) override
	{
		m = member;
		SetProjectionPerspective();

		AssignMemberToDestination();

		CalcFocusDistance();
	}
	void Uninit() override
	{
		// No op.
	}

	void Update( ICamera::Controller controller ) override
	{
		Move( controller );
		RotateByRollPitchYaw( controller );

		InterpolateMember( controller.slerpPercent );

		PutPositionOnCircumference();
	}
private:
	void CalcFocusDistance()
	{
		focusDistance = ( m.focus - m.pos ).Length();
	}
	void CalcDestFocusDistance()
	{
		focusDistance = ( dest.focus - dest.pos ).Length();
	}

	void PutPositionOnCircumference()
	{
		const Donya::Vector3 nFront = m.orientation.LocalFront();
		m.pos = m.focus + ( -nFront * focusDistance );
	}

	void Move ( ICamera::Controller controller )
	{
		Dolly( controller.moveVelocity.z );
		Pan  ( controller.moveVelocity.x );
		Tilt ( controller.moveVelocity.y );
	}
	void Dolly( float moveAmount )
	{
		// The distance is increase if a moveAmount taking get far(i.e. negative). So addition by inverse.
		focusDistance += -moveAmount;
	}
	void Pan  ( float moveAmount )
	{
		const Donya::Vector3 movement = dest.orientation.LocalRight() * moveAmount;
		dest.pos   += movement;
		dest.focus += movement;
	}
	void Tilt ( float moveAmount )
	{
		const Donya::Vector3 movement = dest.orientation.LocalUp() * moveAmount;
		dest.pos   += movement;
		dest.focus += movement;
	}
public:
	void SetPosition( const Donya::Vector3 &point ) override
	{
		BaseCamera::SetPosition( point );
		CalcDestFocusDistance();
	}
	void SetFocusPoint( const Donya::Vector3 &point ) override
	{
		BaseCamera::SetFocusPoint( point );
		CalcDestFocusDistance();
	}
	void SetFocusToFront( float distance ) override
	{
		focusDistance = distance;
		dest.focus = dest.orientation.LocalFront() * distance;

		PutPositionOnCircumference();
	}
};

// SatelliteCamera
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
		{ 0.0f, 0.0f, -1.0f },			// pos
		{ 0.0f, 0.0f, 0.0f },			// focus
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
	case ICamera::Mode::Satellite:
		pCamera = std::make_unique<SatelliteCamera>();
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

float				ICamera::GetFOV()				const
{
	AssertIfNullptr();
	return pCamera->GetFOV();
}
float				ICamera::GetZNear()				const
{
	AssertIfNullptr();
	return pCamera->GetZNear();
}
float				ICamera::GetZFar()				const
{
	AssertIfNullptr();
	return pCamera->GetZFar();
}
Donya::Vector2		ICamera::GetScreenSize()		const
{
	AssertIfNullptr();
	return pCamera->GetScreenSize();
}
Donya::Vector3		ICamera::GetPosition()			const
{
	AssertIfNullptr();
	return pCamera->GetPosition();
}
Donya::Vector3		ICamera::GetFocusPoint()		const
{
	AssertIfNullptr();
	return pCamera->GetFocusPoint();
}
Donya::Quaternion	ICamera::GetOrientation()		const
{
	AssertIfNullptr();
	return pCamera->GetOrientation();
}
Donya::Vector4x4	ICamera::CalcViewMatrix()		const
{
	AssertIfNullptr();
	return pCamera->CalcViewMatrix();
}
Donya::Vector4x4	ICamera::GetProjectionMatrix()	const
{
	AssertIfNullptr();
	return pCamera->GetProjectionMatrix();
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
