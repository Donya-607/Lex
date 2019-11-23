#ifndef INCLUDED_LEX_CAMERA_H_
#define INCLUDED_LEX_CAMERA_H_

#include <memory>

#include "Donya/Constant.h"	// Use DEBUG_MODE macro.
#include "Donya/Quaternion.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

/// <summary>
/// The interface of camera.
/// </summary>
class ICamera
{
public:
	/// <summary>
	/// Specify the camera's movement.
	/// </summary>
	enum class Mode
	{
		Free,		// The rotate origin is myself. The focus point will be invalid.
		Look,		// Keep looking the focus point. The rotation is invalid.
		Satellite,	// The movement is like satellite. Keep looking the focus point. The rotation is invalid.
	};
	/// <summary>
	/// Store information of drive the camera.
	/// </summary>
	struct Controller
	{
		Donya::Vector3		moveVelocity{};				// Set move vector(contain speed).
		Donya::Quaternion	rotation{};					// Set the rotation. This parameter is enable only when the camera mode is Free.
		float				slerpPercent{ 1.0f };		// Set percentage of interpolation(0.0f ~ 1.0f). This affects the movement and the rotation.
		bool				moveInLocalSpace{ true };	// Specify the space of movement(world-space or camera-space). If the Satellite mode, moving space is fixed to camera-space.
	public:
		// This condition is same as default constructed condition.
		void SetNoOperation()
		{
			moveVelocity	= Donya::Vector3::Zero();
			rotation		= Donya::Quaternion::Identity();
			slerpPercent	= 0.0f;
		}
	};
// private:
public: // I want to hide the "Configuration" struct, but if hide it, a derived from "BaseCamera" class can not access. :(
	/// <summary>
	/// Use when change the mode. Store a user specified parameter, then change the mode and set the parameter.
	/// </summary>
	struct Configuration
	{
		float				FOV{};
		float				zNear{};
		float				zFar{};
		Donya::Vector2		screenSize{};
		Donya::Vector3		pos{};
		Donya::Vector3		focus{};
		Donya::Quaternion	orientation{};
	};
public:
	class BaseCamera;
private:
	std::unique_ptr<BaseCamera> pCamera;
	Mode currentMode;
public:
	ICamera();
	~ICamera();
public:
	void Init( Mode initialMode );
	void Uninit();

	void Update( Controller controller );
public:
	void ChangeMode( Mode nextMode );

	/// <summary>
	/// This set only z-range(near, far), so you should call SetProjectionXXX() after this.
	/// </summary>
	void SetZRange					( float zNear, float zFar );
	/// <summary>
	/// This set only Field-Of-View, so you should call SetProjectionXXX() after this.
	/// </summary>
	void SetFOV						( float FOV );
	/// <summary>
	/// Please don't set zero to the height of screenSize. Because will be divided the width by height.<para></para>
	/// This set only screen size, so you should call SetProjectionXXX() after this.
	/// </summary>
	void SetScreenSize				( const Donya::Vector2 &screenSize );
	void SetPosition				( const Donya::Vector3 &point );
	void SetFocusPoint				( const Donya::Vector3 &point );
	/// <summary>
	/// This method is valid when the camera's orientation is valid.
	/// </summary>
	void SetFocusToFront			( float distance );
	void SetOrientation				( const Donya::Quaternion &orientation );
	/// <summary>
	/// If set { 0, 0 } to the "viewSize", use registered screen size.
	/// </summary>
	void SetProjectionOrthographic	( const Donya::Vector2 &viewSize = { 0.0f, 0.0f } );
	/// <summary>
	/// If set 0.0f to the "aspectRatio", calculate by registered screen size.
	/// </summary>
	void SetProjectionPerspective	( float aspectRatio = 0.0f );

	Donya::Vector3		GetPosition()			const;
	Donya::Vector3		GetFocusPoint()			const;
	Donya::Quaternion	GetOrientation()		const;
	Donya::Vector4x4	CalcViewMatrix()		const;
	Donya::Vector4x4	GetProjectionMatrix()	const;

#if USE_IMGUI

	void ShowImGuiNode();

#endif // USE_IMGUI
private:
	void AssertIfNullptr() const;

	Configuration BuildCurrentConfiguration() const;
};

// Donya's version.
/*
/// <summary>
/// 
/// </summary>
class Camera
{
	static constexpr unsigned int PROGRAM_VERSION = 0;
public:
	/// <summary>
	/// Store information of drive the camera.
	/// </summary>
	struct Controller
	{
		// Donya::Vector3 rotateOrigin;

		Donya::Vector3	moveVelocity{};						// Set move direction(contain speed).
		Donya::Vector3	rotation{};							// Set rotate angles(radian), each angles are used to direction of rotate(e.g. rotation.x is used to yaw-axis rotate).
		float			slerpPercent{ 1.0f };				// Set percentage of interpolation(0.0f ~ 1.0f, will be clamped). use to rotate direction to "lookAt" if that is not zero.
		bool			moveAtLocalSpace{ true };			// Specify the space of movement. world-space or local-space(with current orientation).
	public:
		// This condition is same as default constructed condition.
		void SetNoOperation()
		{
			moveVelocity	= Donya::Vector3::Zero();
			rotation		= Donya::Vector3::Zero();
			slerpPercent	= 0.0f;
		}
	};
private:
	float				focusDistance;		// This enable when distance != zero, the focus is there front of camera.
	float				scopeAngle;			// Radian
	Donya::Vector2		screenSize;
	Donya::Vector2		halfScreenSize;
	Donya::Vector3		pos;
	Donya::Vector3		focus;
	Donya::Vector3		velocity;
	Donya::Quaternion	orientation;
	Donya::Vector4x4	projection;
public:
	Camera();
	~Camera();
public:
	void Init( float screenWidth, float screenHeight, float scopeAngle );
	/// <summary>
	/// Set Whole-size.
	/// </summary>
	void SetScreenSize( float newScreenWidth, float newScreenHeight );
	/// <summary>
	/// Set Whole-size.
	/// </summary>
	void SetScreenSize( Donya::Vector2 newScreenSize );
	/// <summary>
	/// Can not Set to focus-position.
	/// </summary>
	void SetPosition( Donya::Vector3 newPosition );
	/// <summary>
	/// "scopeAngle" is 0-based, radian.
	/// </summary>
	void SetScopeAngle( float scopeAngle );

	/// <summary>
	/// Set distance of focus from camera position.<para></para>
	/// ! If the camera rotate after called this, the focus will be move also. !
	/// </summary>
	void SetFocusDistance( float distance );
	/// <summary>
	/// Set the focus position in world-space.<para></para>
	/// If that focus position same as camera position, that focus position will be pushed to front.<para></para>
	/// ! The focus position is fixed, that is not move if the camera rotate after call this.
	/// </summary>
	void SetFocusCoordinate( const Donya::Vector3 &coordinate );

	void ResetOrthographicProjection();
	/// <summary>
	/// Requirement : the camera must be already initialized.
	/// </summary>
	void ResetPerspectiveProjection();
	Donya::Vector4x4	SetOrthographicProjectionMatrix( float width, float height, float mostNear, float mostFar );
	/// <summary>
	/// ScopeAngle, Near, Far are used to default.
	/// </summary>
	Donya::Vector4x4	SetPerspectiveProjectionMatrix( float aspectRatio );
	Donya::Vector4x4	SetPerspectiveProjectionMatrix( float scopeAngle, float aspectRatio, float mostNear, float mostFar );
	Donya::Vector4x4	CalcViewMatrix()		const;
	Donya::Vector4x4	GetProjectionMatrix()	const { return projection; }
	Donya::Vector3		GetPos()				const { return pos; }
	Donya::Quaternion	GetPosture()			const { return orientation; }
public:
	void Update( Controller controller );
private:
	bool IsFocusFixed() const;

	void ResetOrientation();
	void NormalizePostureIfNeeded();

	void SetFocusToFront();
	void LookAtFocus();

	void Move( Controller controller );

	void Zoom( Controller controller );

	void Rotate( Controller controller );
	void OrbitAround( Controller controller );

	void Pan( Controller controller );
	void CalcDistToVirtualScreen();
	Donya::Vector3 ToWorldPos( const Donya::Vector2 &screenPos );

#if DEBUG_MODE

public:
	void ShowParametersToImGui();

#endif // DEBUG_MODE
};

*/

// Old Lex version.
/*
#include "Donya/Quaternion.h"
#include "Donya/UseImGui.h"	// Use USE_IMGUI macro.
#include "Donya/Vector.h"

class Camera
{
	static constexpr unsigned int PROGRAM_VERSION = 0;
private:
	enum class Mode
	{
		None,			// when the left-button and wheel-button is not pressed.
		OrbitAround,	// when left-clicking.
		Pan				// when wheel-pressing, or right-clicking.
	};
	struct MouseCoord
	{
		Donya::Vector2 prev{};
		Donya::Vector2 current{};
	};
private:
	Mode				moveMode;
	float				radius;				// same as length of vector(pos-focus).
	float				scopeAngle;			// 0-based, Radian
	float				virtualDistance;	// The distance of from my-position to virtual-screen, use for Pan() move.
	Donya::Vector3		pos;
	Donya::Vector3		focus;
	Donya::Vector3		velocity;
	MouseCoord			mouse;
	Donya::Quaternion	orientation;
	Donya::Vector4x4	projection;
public:
	Camera();
	Camera( float scopeAngle );
	~Camera() {}
public:
	/// <summary>
	/// The position are setting to { 0.0f, 0.0f, 0.0f }.<para></para>
	/// The focus are setting to { 0.0f, 0.0f, 1.0f }.
	/// </summary>
	void SetToHomePosition( Donya::Vector3 homePosition = { 0.0f, 0.0f, 0.0f }, Donya::Vector3 homeFocus = { 0.0f, 0.0f, 1.0f } );
	/// <summary>
	/// "scopeAngle" is 0-based, radian.
	/// </summary>
	void SetScopeAngle( float scopeAngle );

	void ResetOrthographicProjection();
	void ResetPerspectiveProjection();
	Donya::Vector4x4 SetOrthographicProjectionMatrix( float width, float height, float mostNear, float mostFar );
	/// <summary>
	/// ScopeAngle, Near, Far are used to default.
	/// </summary>
	Donya::Vector4x4 SetPerspectiveProjectionMatrix( float aspectRatio );
	Donya::Vector4x4 SetPerspectiveProjectionMatrix( float scopeAngle, float aspectRatio, float mostNear, float mostFar );
	Donya::Vector4x4 CalcViewMatrix() const;

	Donya::Vector4x4 GetProjectionMatrix()	const { return projection; }
	Donya::Vector3   GetPos()				const { return pos; }
public:
	void Update( const Donya::Vector3 &targetPos );	// You can set nullptr.
private:
	void MouseUpdate();

	void ChangeMode();

	void Move( const Donya::Vector3 &targetPos );

	void Zoom();

	void OrbitAround();

	void Pan();
	void CalcDistToVirtualScreen();
	Donya::Vector3 ToWorldPos( const Donya::Vector2 &screenPos );

#if USE_IMGUI

public:
	void ShowImGuiNode();

#endif // USE_IMGUI
};
*/

#endif // INCLUDED_LEX_CAMERA_H_