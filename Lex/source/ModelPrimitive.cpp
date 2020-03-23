#include "ModelPrimitive.h"

#include <array>

#include "Donya/Donya.h"	// Use GetDevice(), GetImmdiateContext().

namespace
{
	template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	void SetNullIndexBuffer( ID3D11DeviceContext *pImmediateContext )
	{
		pImmediateContext->IASetIndexBuffer( nullptr, DXGI_FORMAT_R32_UINT, NULL );
	}
}

namespace Donya
{
	namespace Model
	{

		constexpr D3D11_USAGE	BUFFER_USAGE = D3D11_USAGE_IMMUTABLE;
		constexpr UINT			CPU_ACCESS_FLAG = NULL;
		template<typename Struct>
		HRESULT	MakeBuffer( const std::vector<Struct> &source, ComPtr<ID3D11Buffer> *pComBuffer, ID3D11Device *pDevice, D3D11_USAGE bufferUsage = D3D11_USAGE_IMMUTABLE, const UINT CPUAccessFlag = NULL )
		{
			return Donya::CreateVertexBuffer<Struct>
			(
				pDevice, source,
				bufferUsage, CPUAccessFlag,
				pComBuffer->GetAddressOf()
			);
		}

		namespace Impl
		{
			HRESULT PrimitiveModel::CreateBufferPos( const std::vector<Vertex::Pos> &source )
			{
				return MakeBuffer<Vertex::Pos>( source, &pBufferPos, Donya::GetDevice() );
			}

			void PrimitiveModel::SetVertexBuffers() const
			{
				constexpr UINT stride = sizeof( Vertex::Pos );
				constexpr UINT offset = 0;

				ID3D11DeviceContext *pImmediateContext = Donya::GetImmediateContext();
				pImmediateContext->IASetVertexBuffers
				(
					0U, 1U,
					pBufferPos.GetAddressOf(),
					&stride,
					&offset
				);
			}
			void PrimitiveModel::SetIndexBuffer() const
			{
				SetNullIndexBuffer( Donya::GetImmediateContext() );
			}
		}

	#pragma region Cube

		struct CubeConfigration
		{
			std::array<Vertex::Pos, 4 * 6> vertices;	// "4 * 6" represents six squares.
			std::array<size_t, 3 * 2 * 6>  indices;		// "3 * 2 * 6" represents the six groups of the square that constructed by two triangles.
		public:
			constexpr CubeConfigration() : vertices(), indices() {}
		};
		constexpr CubeConfigration CreateCube()
		{
			CubeConfigration cube{};

			size_t vIndex = 0;
			size_t iIndex = 0;
			// Top
			{
				constexpr Donya::Vector3 N{ 0.0f, 1.0f, 0.0f };
				cube.vertices[vIndex + 0].position = Donya::Vector3{ -0.5f, +0.5f, +0.5f }; // LTF
				cube.vertices[vIndex + 1].position = Donya::Vector3{ +0.5f, +0.5f, +0.5f }; // RTF
				cube.vertices[vIndex + 2].position = Donya::Vector3{ -0.5f, +0.5f, -0.5f }; // LTB
				cube.vertices[vIndex + 3].position = Donya::Vector3{ +0.5f, +0.5f, -0.5f }; // RTB
				cube.vertices[vIndex + 0].normal = N;
				cube.vertices[vIndex + 1].normal = N;
				cube.vertices[vIndex + 2].normal = N;
				cube.vertices[vIndex + 3].normal = N;

				cube.indices[iIndex + 0] = vIndex + 0;
				cube.indices[iIndex + 1] = vIndex + 1;
				cube.indices[iIndex + 2] = vIndex + 2;

				cube.indices[iIndex + 3] = vIndex + 1;
				cube.indices[iIndex + 4] = vIndex + 3;
				cube.indices[iIndex + 5] = vIndex + 2;
			}
			vIndex += 4;
			iIndex += 6;
			// Bottom
			{
				constexpr Donya::Vector3 N{ 0.0f, -1.0f, 0.0f };
				cube.vertices[vIndex + 0].position = Donya::Vector3{ -0.5f, -0.5f, +0.5f }; // LBF
				cube.vertices[vIndex + 1].position = Donya::Vector3{ +0.5f, -0.5f, +0.5f }; // RBF
				cube.vertices[vIndex + 2].position = Donya::Vector3{ -0.5f, -0.5f, -0.5f }; // LBB
				cube.vertices[vIndex + 3].position = Donya::Vector3{ +0.5f, -0.5f, -0.5f }; // RBB
				cube.vertices[vIndex + 0].normal = N;
				cube.vertices[vIndex + 1].normal = N;
				cube.vertices[vIndex + 2].normal = N;
				cube.vertices[vIndex + 3].normal = N;

				cube.indices[iIndex + 0] = vIndex + 0;
				cube.indices[iIndex + 1] = vIndex + 2;
				cube.indices[iIndex + 2] = vIndex + 1;

				cube.indices[iIndex + 3] = vIndex + 1;
				cube.indices[iIndex + 4] = vIndex + 2;
				cube.indices[iIndex + 5] = vIndex + 3;
			}
			vIndex += 4;
			iIndex += 6;
			// Right
			{
				constexpr Donya::Vector3 N{ 1.0f, 0.0f, 0.0f };
				cube.vertices[vIndex + 0].position = Donya::Vector3{ +0.5f, +0.5f, -0.5f }; // RTB
				cube.vertices[vIndex + 1].position = Donya::Vector3{ +0.5f, +0.5f, +0.5f }; // RTF
				cube.vertices[vIndex + 2].position = Donya::Vector3{ +0.5f, -0.5f, -0.5f }; // RBB
				cube.vertices[vIndex + 3].position = Donya::Vector3{ +0.5f, -0.5f, +0.5f }; // RBF
				cube.vertices[vIndex + 0].normal = N;
				cube.vertices[vIndex + 1].normal = N;
				cube.vertices[vIndex + 2].normal = N;
				cube.vertices[vIndex + 3].normal = N;

				cube.indices[iIndex + 0] = vIndex + 0;
				cube.indices[iIndex + 1] = vIndex + 1;
				cube.indices[iIndex + 2] = vIndex + 2;

				cube.indices[iIndex + 3] = vIndex + 1;
				cube.indices[iIndex + 4] = vIndex + 3;
				cube.indices[iIndex + 5] = vIndex + 2;
			}
			vIndex += 4;
			iIndex += 6;
			// Left
			{
				constexpr Donya::Vector3 N{ -1.0f, 0.0f, 0.0f };
				cube.vertices[vIndex + 0].position = Donya::Vector3{ -0.5f, +0.5f, -0.5f }; // LTB
				cube.vertices[vIndex + 1].position = Donya::Vector3{ -0.5f, +0.5f, +0.5f }; // LTF
				cube.vertices[vIndex + 2].position = Donya::Vector3{ -0.5f, -0.5f, -0.5f }; // LBB
				cube.vertices[vIndex + 3].position = Donya::Vector3{ -0.5f, -0.5f, +0.5f }; // LBF
				cube.vertices[vIndex + 0].normal = N;
				cube.vertices[vIndex + 1].normal = N;
				cube.vertices[vIndex + 2].normal = N;
				cube.vertices[vIndex + 3].normal = N;

				cube.indices[iIndex + 0] = vIndex + 0;
				cube.indices[iIndex + 1] = vIndex + 2;
				cube.indices[iIndex + 2] = vIndex + 1;

				cube.indices[iIndex + 3] = vIndex + 1;
				cube.indices[iIndex + 4] = vIndex + 2;
				cube.indices[iIndex + 5] = vIndex + 3;
			}
			vIndex += 4;
			iIndex += 6;
			// Near
			{
				constexpr Donya::Vector3 N{ 0.0f, 0.0f, 1.0f };
				cube.vertices[vIndex + 0].position = Donya::Vector3{ +0.5f, -0.5f, +0.5f }; // RBF
				cube.vertices[vIndex + 1].position = Donya::Vector3{ +0.5f, +0.5f, +0.5f }; // RTF
				cube.vertices[vIndex + 2].position = Donya::Vector3{ -0.5f, -0.5f, +0.5f }; // LBF
				cube.vertices[vIndex + 3].position = Donya::Vector3{ -0.5f, +0.5f, +0.5f }; // LTF
				cube.vertices[vIndex + 0].normal = N;
				cube.vertices[vIndex + 1].normal = N;
				cube.vertices[vIndex + 2].normal = N;
				cube.vertices[vIndex + 3].normal = N;

				cube.indices[iIndex + 0] = vIndex + 0;
				cube.indices[iIndex + 1] = vIndex + 1;
				cube.indices[iIndex + 2] = vIndex + 2;

				cube.indices[iIndex + 3] = vIndex + 1;
				cube.indices[iIndex + 4] = vIndex + 3;
				cube.indices[iIndex + 5] = vIndex + 2;
			}
			vIndex += 4;
			iIndex += 6;
			// Far
			{
				constexpr Donya::Vector3 N{ 0.0f, 0.0f, -1.0f };
				cube.vertices[vIndex + 0].position = Donya::Vector3{ +0.5f, -0.5f, -0.5f }; // RBB
				cube.vertices[vIndex + 1].position = Donya::Vector3{ +0.5f, +0.5f, -0.5f }; // RTB
				cube.vertices[vIndex + 2].position = Donya::Vector3{ -0.5f, -0.5f, -0.5f }; // LBB
				cube.vertices[vIndex + 3].position = Donya::Vector3{ -0.5f, +0.5f, -0.5f }; // LTB
				cube.vertices[vIndex + 0].normal = N;
				cube.vertices[vIndex + 1].normal = N;
				cube.vertices[vIndex + 2].normal = N;
				cube.vertices[vIndex + 3].normal = N;

				cube.indices[iIndex + 0] = vIndex + 0;
				cube.indices[iIndex + 1] = vIndex + 2;
				cube.indices[iIndex + 2] = vIndex + 1;

				cube.indices[iIndex + 3] = vIndex + 1;
				cube.indices[iIndex + 4] = vIndex + 2;
				cube.indices[iIndex + 5] = vIndex + 3;
			}
			vIndex += 4;
			iIndex += 6;
		}


		bool Cube::Create()
		{

		}

	// region Cube
	#pragma endregion
	}
}
