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
			std::array<Vertex::Pos, 4U * 6U> vertices;	// "4 * 6" represents six squares.
			std::array<size_t, 3U * 2U * 6U> indices;	// "3 * 2 * 6" represents the six groups of the square that constructed by two triangles.
		public:
			constexpr CubeConfigration() : vertices(), indices() {}
		};
		constexpr CubeConfigration CreateCube()
		{
			CubeConfigration cube{};

			std::array<Vertex::Pos, 4U * 6U> vertices{};
			std::array<size_t, 3U * 2U * 6U> indices{};

			/*
			Abbreviations:
			VTX	Vertex
			IDX	Index
			L	Left
			R	Right
			T	Top
			B	Bottom
			N	Near
			F	Far
			*/

			constexpr float  WHOLE_CUBE_SIZE = 1.0f;		// Unit size.
			constexpr float  DIST = WHOLE_CUBE_SIZE / 2.0f;	// The distance from center(0.0f).
			constexpr size_t STRIDE_VTX = 4U;				// Because the square is made up of 4 vertices.
			constexpr size_t STRIDE_IDX = 2U * 3U;			// Because the square is made up of 2 triangles, and that triangle is made up of 3 vertices.
			constexpr std::array<size_t, STRIDE_IDX> CW_INDICES
			{
				0, 1, 2,	// LT->RT->LB(->LT)
				1, 3, 2		// RT->RB->LB(->RT)
			};
			constexpr std::array<size_t, STRIDE_IDX> CCW_INDICES
			{
				0, 2, 1,	// LT->LB->RT(->LT)
				1, 2, 3		// RT->LB->RB(->RT)
			};

			constexpr size_t TOP_VTX_IDX = STRIDE_VTX * 0U;
			constexpr size_t TOP_IDX_IDX = STRIDE_IDX * 0U;
			constexpr Donya::Vector3 TOP_NORMAL{ 0.0f, +1.0f, 0.0f };
			// Top
			{
				cube.vertices[TOP_VTX_IDX + 0U].position = Donya::Vector3{ -DIST, +DIST, +DIST }; // LTF
				cube.vertices[TOP_VTX_IDX + 1U].position = Donya::Vector3{ +DIST, +DIST, +DIST }; // RTF
				cube.vertices[TOP_VTX_IDX + 2U].position = Donya::Vector3{ -DIST, +DIST, -DIST }; // LTN
				cube.vertices[TOP_VTX_IDX + 3U].position = Donya::Vector3{ +DIST, +DIST, -DIST }; // RTN
				cube.vertices[TOP_VTX_IDX + 0U].normal = TOP_NORMAL;
				cube.vertices[TOP_VTX_IDX + 1U].normal = TOP_NORMAL;
				cube.vertices[TOP_VTX_IDX + 2U].normal = TOP_NORMAL;
				cube.vertices[TOP_VTX_IDX + 3U].normal = TOP_NORMAL;
				
				cube.indices[TOP_IDX_IDX + 0U] = CW_INDICES[0U] + TOP_VTX_IDX;
				cube.indices[TOP_IDX_IDX + 1U] = CW_INDICES[1U] + TOP_VTX_IDX;
				cube.indices[TOP_IDX_IDX + 2U] = CW_INDICES[2U] + TOP_VTX_IDX;

				cube.indices[TOP_IDX_IDX + 3U] = CW_INDICES[3U] + TOP_VTX_IDX;
				cube.indices[TOP_IDX_IDX + 4U] = CW_INDICES[4U] + TOP_VTX_IDX;
				cube.indices[TOP_IDX_IDX + 5U] = CW_INDICES[5U] + TOP_VTX_IDX;
			}
			constexpr size_t BOTTOM_VTX_IDX = STRIDE_VTX * 1U;
			constexpr size_t BOTTOM_IDX_IDX = STRIDE_IDX * 1U;
			constexpr Donya::Vector3 BOTTOM_NORMAL{ 0.0f, -1.0f, 0.0f };
			// Bottom
			{
				cube.vertices[BOTTOM_VTX_IDX + 0U].position = Donya::Vector3{ -DIST, -DIST, +DIST }; // LBF
				cube.vertices[BOTTOM_VTX_IDX + 1U].position = Donya::Vector3{ +DIST, -DIST, +DIST }; // RBF
				cube.vertices[BOTTOM_VTX_IDX + 2U].position = Donya::Vector3{ -DIST, -DIST, -DIST }; // LBN
				cube.vertices[BOTTOM_VTX_IDX + 3U].position = Donya::Vector3{ +DIST, -DIST, -DIST }; // RBN
				cube.vertices[BOTTOM_VTX_IDX + 0U].normal = BOTTOM_NORMAL;
				cube.vertices[BOTTOM_VTX_IDX + 1U].normal = BOTTOM_NORMAL;
				cube.vertices[BOTTOM_VTX_IDX + 2U].normal = BOTTOM_NORMAL;
				cube.vertices[BOTTOM_VTX_IDX + 3U].normal = BOTTOM_NORMAL;

				cube.indices[BOTTOM_IDX_IDX + 0U] = CCW_INDICES[0U] + BOTTOM_VTX_IDX;
				cube.indices[BOTTOM_IDX_IDX + 1U] = CCW_INDICES[1U] + BOTTOM_VTX_IDX;
				cube.indices[BOTTOM_IDX_IDX + 2U] = CCW_INDICES[2U] + BOTTOM_VTX_IDX;

				cube.indices[BOTTOM_IDX_IDX + 3U] = CCW_INDICES[3U] + BOTTOM_VTX_IDX;
				cube.indices[BOTTOM_IDX_IDX + 4U] = CCW_INDICES[4U] + BOTTOM_VTX_IDX;
				cube.indices[BOTTOM_IDX_IDX + 5U] = CCW_INDICES[5U] + BOTTOM_VTX_IDX;
			}
			constexpr size_t RIGHT_VTX_IDX = STRIDE_VTX * 2U;
			constexpr size_t RIGHT_IDX_IDX = STRIDE_IDX * 2U;
			constexpr Donya::Vector3 RIGHT_NORMAL{ +1.0f, 0.0f, 0.0f };
			// Right
			{
				cube.vertices[RIGHT_VTX_IDX + 0U].position = Donya::Vector3{ +DIST, +DIST, -DIST }; // RTN
				cube.vertices[RIGHT_VTX_IDX + 1U].position = Donya::Vector3{ +DIST, +DIST, +DIST }; // RTF
				cube.vertices[RIGHT_VTX_IDX + 2U].position = Donya::Vector3{ +DIST, -DIST, -DIST }; // RBN
				cube.vertices[RIGHT_VTX_IDX + 3U].position = Donya::Vector3{ +DIST, -DIST, +DIST }; // RBF
				cube.vertices[RIGHT_VTX_IDX + 0U].normal = RIGHT_NORMAL;
				cube.vertices[RIGHT_VTX_IDX + 1U].normal = RIGHT_NORMAL;
				cube.vertices[RIGHT_VTX_IDX + 2U].normal = RIGHT_NORMAL;
				cube.vertices[RIGHT_VTX_IDX + 3U].normal = RIGHT_NORMAL;

				cube.indices[RIGHT_IDX_IDX + 0U] = CW_INDICES[0U] + RIGHT_VTX_IDX;
				cube.indices[RIGHT_IDX_IDX + 1U] = CW_INDICES[1U] + RIGHT_VTX_IDX;
				cube.indices[RIGHT_IDX_IDX + 2U] = CW_INDICES[2U] + RIGHT_VTX_IDX;

				cube.indices[RIGHT_IDX_IDX + 3U] = CW_INDICES[3U] + RIGHT_VTX_IDX;
				cube.indices[RIGHT_IDX_IDX + 4U] = CW_INDICES[4U] + RIGHT_VTX_IDX;
				cube.indices[RIGHT_IDX_IDX + 5U] = CW_INDICES[5U] + RIGHT_VTX_IDX;
			}
			constexpr size_t LEFT_VTX_IDX = STRIDE_VTX * 3U;
			constexpr size_t LEFT_IDX_IDX = STRIDE_IDX * 3U;
			constexpr Donya::Vector3 LEFT_NORMAL{ -1.0f, 0.0f, 0.0f };
			// Left
			{
				cube.vertices[LEFT_VTX_IDX + 0U].position = Donya::Vector3{ -DIST, +DIST, -DIST }; // LTN
				cube.vertices[LEFT_VTX_IDX + 1U].position = Donya::Vector3{ -DIST, +DIST, +DIST }; // LTF
				cube.vertices[LEFT_VTX_IDX + 2U].position = Donya::Vector3{ -DIST, -DIST, -DIST }; // LBN
				cube.vertices[LEFT_VTX_IDX + 3U].position = Donya::Vector3{ -DIST, -DIST, +DIST }; // LBF
				cube.vertices[LEFT_VTX_IDX + 0U].normal = LEFT_NORMAL;
				cube.vertices[LEFT_VTX_IDX + 1U].normal = LEFT_NORMAL;
				cube.vertices[LEFT_VTX_IDX + 2U].normal = LEFT_NORMAL;
				cube.vertices[LEFT_VTX_IDX + 3U].normal = LEFT_NORMAL;

				cube.indices[LEFT_IDX_IDX + 0U] = CCW_INDICES[0U] + LEFT_VTX_IDX;
				cube.indices[LEFT_IDX_IDX + 1U] = CCW_INDICES[1U] + LEFT_VTX_IDX;
				cube.indices[LEFT_IDX_IDX + 2U] = CCW_INDICES[2U] + LEFT_VTX_IDX;

				cube.indices[LEFT_IDX_IDX + 3U] = CCW_INDICES[3U] + LEFT_VTX_IDX;
				cube.indices[LEFT_IDX_IDX + 4U] = CCW_INDICES[4U] + LEFT_VTX_IDX;
				cube.indices[LEFT_IDX_IDX + 5U] = CCW_INDICES[5U] + LEFT_VTX_IDX;
			}
			constexpr size_t FAR_VTX_IDX = STRIDE_VTX * 4U;
			constexpr size_t FAR_IDX_IDX = STRIDE_IDX * 4U;
			constexpr Donya::Vector3 FAR_NORMAL{ 0.0f, 0.0f, +1.0f };
			// Far
			{
				cube.vertices[FAR_VTX_IDX + 0U].position = Donya::Vector3{ +DIST, -DIST, +DIST }; // RBF
				cube.vertices[FAR_VTX_IDX + 1U].position = Donya::Vector3{ +DIST, +DIST, +DIST }; // RTF
				cube.vertices[FAR_VTX_IDX + 2U].position = Donya::Vector3{ -DIST, -DIST, +DIST }; // LBF
				cube.vertices[FAR_VTX_IDX + 3U].position = Donya::Vector3{ -DIST, +DIST, +DIST }; // LTF
				cube.vertices[FAR_VTX_IDX + 0U].normal = FAR_NORMAL;
				cube.vertices[FAR_VTX_IDX + 1U].normal = FAR_NORMAL;
				cube.vertices[FAR_VTX_IDX + 2U].normal = FAR_NORMAL;
				cube.vertices[FAR_VTX_IDX + 3U].normal = FAR_NORMAL;

				cube.indices[FAR_IDX_IDX + 0U] = CW_INDICES[0U] + FAR_VTX_IDX;
				cube.indices[FAR_IDX_IDX + 1U] = CW_INDICES[1U] + FAR_VTX_IDX;
				cube.indices[FAR_IDX_IDX + 2U] = CW_INDICES[2U] + FAR_VTX_IDX;

				cube.indices[FAR_IDX_IDX + 3U] = CW_INDICES[3U] + FAR_VTX_IDX;
				cube.indices[FAR_IDX_IDX + 4U] = CW_INDICES[4U] + FAR_VTX_IDX;
				cube.indices[FAR_IDX_IDX + 5U] = CW_INDICES[5U] + FAR_VTX_IDX;
			}
			constexpr size_t NEAR_VTX_IDX = STRIDE_VTX * 5U;
			constexpr size_t NEAR_IDX_IDX = STRIDE_IDX * 5U;
			constexpr Donya::Vector3 NEAR_NORMAL{ 0.0f, 0.0f, -1.0f };
			// Near
			{
				cube.vertices[NEAR_VTX_IDX + 0U].position = Donya::Vector3{ +DIST, -DIST, -DIST }; // RBN
				cube.vertices[NEAR_VTX_IDX + 1U].position = Donya::Vector3{ +DIST, +DIST, -DIST }; // RTN
				cube.vertices[NEAR_VTX_IDX + 2U].position = Donya::Vector3{ -DIST, -DIST, -DIST }; // LBN
				cube.vertices[NEAR_VTX_IDX + 3U].position = Donya::Vector3{ -DIST, +DIST, -DIST }; // LTN
				cube.vertices[NEAR_VTX_IDX + 0U].normal = NEAR_NORMAL;
				cube.vertices[NEAR_VTX_IDX + 1U].normal = NEAR_NORMAL;
				cube.vertices[NEAR_VTX_IDX + 2U].normal = NEAR_NORMAL;
				cube.vertices[NEAR_VTX_IDX + 3U].normal = NEAR_NORMAL;
				
				cube.indices[NEAR_IDX_IDX + 0U] = CCW_INDICES[0U] + NEAR_VTX_IDX;
				cube.indices[NEAR_IDX_IDX + 1U] = CCW_INDICES[1U] + NEAR_VTX_IDX;
				cube.indices[NEAR_IDX_IDX + 2U] = CCW_INDICES[2U] + NEAR_VTX_IDX;

				cube.indices[NEAR_IDX_IDX + 3U] = CCW_INDICES[3U] + NEAR_VTX_IDX;
				cube.indices[NEAR_IDX_IDX + 4U] = CCW_INDICES[4U] + NEAR_VTX_IDX;
				cube.indices[NEAR_IDX_IDX + 5U] = CCW_INDICES[5U] + NEAR_VTX_IDX;
			}

			return cube;
		}


		bool Cube::Create()
		{

		}

	// region Cube
	#pragma endregion
	}
}
