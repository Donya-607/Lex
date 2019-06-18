#ifndef INCLUDED_CAMERA_H_
#define INCLUDED_CAMERA_H_

#include <memory>

namespace DirectX
{
	struct XMFLOAT4X4;
	struct XMFLOAT3;
	struct XMMATRIX;
}

namespace Donya
{

	class Camera
	{
	private:
		struct Impl;
		std::unique_ptr<Camera::Impl> pImpl;
	public:
		Camera();
		virtual ~Camera();
		Camera( const Camera & )				= delete;
		Camera( Camera && )						= delete;
		Camera & operator = ( const Camera & )	= delete;
		Camera & operator = ( Camera && )		= delete;
	public:
		void Update();
	public:
		const DirectX::XMFLOAT4X4 &AssignOrthographicProjection( float width, float height, float zNear = 0.1f, float zFar = 100.0f );
		const DirectX::XMFLOAT4X4 &AssignPerspectiveProjection( float FOV, float aspect, float zNear = 0.1f, float zFar = 100.0f );
	public:
		const DirectX::XMFLOAT4X4 &GetProjection() const;
		const DirectX::XMFLOAT3   &GetPosition()   const;
		const DirectX::XMMATRIX   CalcViewMatrix() const;
	};

}

#endif //INCLUDED_CAMERA_H_
