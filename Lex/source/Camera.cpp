#include "Camera.h"

#include <DirectXMath.h>
#include <Windows.h>

#include "Vector.h"

using namespace DirectX;

namespace Donya
{
	struct Camera::Impl
	{
		float		theta;	// radian
		Vector3		pos;
		Vector3		focus;
		XMFLOAT4X4	projection;
	public:
		Impl() : theta( 0 ), pos( 0, 0, -10.0f ), focus( 0, 0, 0 ), projection() {}
	public:
		const DirectX::XMMATRIX CalcViewMatrix() const
		{
			XMVECTOR vecEye		= XMVectorSet( pos.x, pos.y, pos.z, 1.0f );
			XMVECTOR vecFocus	= XMVectorSet( focus.x, focus.y, focus.z, 1.0f );
			XMVECTOR vecUp		= XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );

			return DirectX::XMMatrixLookAtLH( vecEye, vecFocus, vecUp );
		}
	};

	Camera::Camera() : pImpl( std::make_unique<Camera::Impl>() ) {}
	Camera::~Camera() = default;

	void Camera::Update()
	{
		constexpr float SPEED = 0.01f;
		XMFLOAT3 velocity{ 0, 0, 0 };

		if( GetAsyncKeyState( 'I' ) < 0 ) { velocity.y += SPEED; }
		if( GetAsyncKeyState( 'K' ) < 0 ) { velocity.y -= SPEED; }
		if( GetAsyncKeyState( 'J' ) < 0 ) { velocity.x -= SPEED; }
		if( GetAsyncKeyState( 'L' ) < 0 ) { velocity.x += SPEED; }

		pImpl->pos   += velocity;
		pImpl->focus += velocity;
	}

	const DirectX::XMFLOAT4X4 &Camera::AssignOrthographicProjection( float width, float height, float zNear, float zFar )
	{
		DirectX::XMMATRIX matProj = DirectX::XMMatrixOrthographicLH( width, height, zNear, zFar );
		XMStoreFloat4x4( &pImpl->projection, matProj );
		return pImpl->projection;
	}
	const DirectX::XMFLOAT4X4 &Camera::AssignPerspectiveProjection( float FOV, float aspect, float zNear, float zFar )
	{
		DirectX::XMMATRIX matProj = DirectX::XMMatrixPerspectiveFovLH( FOV, aspect, zNear, zFar );
		XMStoreFloat4x4( &pImpl->projection, matProj );
		return pImpl->projection;
	}

	const DirectX::XMFLOAT4X4 &Camera::GetProjection() const
	{
		return pImpl->projection;
	}
	const DirectX::XMFLOAT3 &Camera::GetPosition() const
	{
		return pImpl->pos;
	}
	const DirectX::XMMATRIX Camera::CalcViewMatrix() const
	{
		return pImpl->CalcViewMatrix();
	}

}