#pragma once

#include "Vector.h"

class Camera
{
private:
	float scopeAngle; // 0-based, Radian
	Donya::Vector3 pos;
	Donya::Vector3 focus;
	DirectX::XMFLOAT4X4 projection;
public:
	Camera();
	Camera( float scopeAngle );
	~Camera() {}
public:
	/// <summary>
	/// The position are setting to { 0.0f, 0.0f, 0.0f }.<para></para>
	/// The focus are setting to { 0.0f, 0.0f, 1.0f }.
	/// </summary>
	void SetHomePosition( Donya::Vector3 homePosition = { 0.0f, 0.0f, 0.0f }, Donya::Vector3 homeFocus = { 0.0f, 0.0f, 1.0f } );
	/// <summary>
	/// "scopeAngle" is 0-based, radian.
	/// </summary>
	void SetScopeAngle( float scopeAngle );

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
	void Move( const Donya::Vector3 &targetPos );
	void ResetOrthographicProjection();
	void ResetPerspectiveProjection();
};
