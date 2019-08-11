#pragma once

#include "Common.h"	// Use DEBUG_MODE macro.

#include "Quaternion.h"
#include "Vector.h"

class Camera
{
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
	Donya::Quaternion	posture;
	DirectX::XMFLOAT4X4	projection;
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
	DirectX::XMMATRIX SetOrthographicProjectionMatrix( float width, float height, float mostNear, float mostFar );
	/// <summary>
	/// ScopeAngle, Near, Far are used to default.
	/// </summary>
	DirectX::XMMATRIX SetPerspectiveProjectionMatrix( float aspectRatio );
	DirectX::XMMATRIX SetPerspectiveProjectionMatrix( float scopeAngle, float aspectRatio, float mostNear, float mostFar );
	DirectX::XMMATRIX GetViewMatrix() const;
	DirectX::XMMATRIX GetProjectionMatrix() const;
	Donya::Vector3 GetPos() const { return pos; }
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

#if DEBUG_MODE

	void ShowPaametersToImGui();

#endif // DEBUG_MODE
};
