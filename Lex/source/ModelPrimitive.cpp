#include "ModelPrimitive.h"

#include <array>

#include "Donya/Constant.h"	// Use scast macro.
#include "Donya/Donya.h"	// Use GetDevice(), GetImmdiateContext().
#include "Donya/Useful.h"	// Use OutputDebugStr(), MultiToWide().
#include "Donya/RenderingStates.h"

namespace
{
	template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	void SetNullIndexBuffer( ID3D11DeviceContext *pImmediateContext )
	{
		pImmediateContext->IASetIndexBuffer( nullptr, DXGI_FORMAT_R32_UINT, NULL );
	}

	void AssertBaseCreation( const std::string &contentName, const std::string &identityName )
	{
		const std::string errMsg =
			"Failed : The creation of primitive's " + contentName +
			", that is : " + identityName + ".";
		Donya::OutputDebugStr( errMsg.c_str() );
		_ASSERT_EXPR( 0, Donya::MultiToWide( errMsg ).c_str() );
	}
}

namespace Donya
{
	namespace Model
	{
		namespace ShaderSource
		{
			static constexpr const char *EntryPointVS	= "VSMain";
			static constexpr const char *EntryPointPS	= "PSMain";

			static constexpr const char *BasicCode()
			{
				return
				"struct VS_IN\n"
				"{\n"
				"	float4		pos			: POSITION;\n"
				"	float4		normal		: NORMAL;\n"
				"};\n"
				"struct VS_OUT\n"
				"{\n"
				"	float4		svPos		: SV_POSITION;\n"
				"	float4		normal		: NORMAL;\n"
				"};\n"

				"cbuffer Constant : register( b0 )\n"
				"{\n"
				"	row_major\n"
				"	float4x4	cbWorld;\n"
				"	row_major\n"
				"	float4x4	cbViewProj;\n"
				"	float4		cbDrawColor;\n"
				"	float3		cbLightDirection;\n"
				"	float		cbLightBias;\n"
				"};\n"

				"VS_OUT VSMain( VS_IN vin )\n"
				"{\n"
				"	vin.pos.w		= 1.0f;\n"
				"	vin.normal.w	= 0.0f;\n"

				"	float4x4 WVP	= mul( cbWorld, cbViewProj );\n"

				"	VS_OUT vout		= ( VS_OUT )( 0 );\n"
				"	vout.svPos		= mul( vin.pos, WVP );\n"
				"	vout.normal		= normalize( mul( vin.normal, cbWorld ) );\n"
				"	return vout;\n"
				"}\n"

				"float Lambert( float3 nwsNormal, float3 nwsToLightVec )\n"
				"{\n"
				"	return max( 0.0f, dot( nwsNormal, nwsToLightVec ) );\n"
				"}\n"
				"float4 PSMain( VS_OUT pin ) : SV_TARGET\n"
				"{\n"
				"			pin.normal		= normalize( pin.normal );\n"
			
				"	float3	nLightVec		= normalize( -cbLightDirection );	// Vector from position.\n"
				"	float	diffuse			= Lambert( pin.normal.rgb, nLightVec );\n"
				"	float	Kd				= ( 1.0f - cbLightBias ) + ( diffuse * cbLightBias );\n"

				"	return	float4( cbDrawColor.rgb * Kd, cbDrawColor.a );\n"
				"}\n"
				;
			}
			static constexpr const char *BasicNameVS	= "Donya::DefaultPrimitiveVS";
			static constexpr const char *BasicNamePS	= "Donya::DefaultPrimitivePS";

			static constexpr RegisterDesc BasicPrimitiveSetting()
			{
				return RegisterDesc::Make( 0U, true, true );
			}
		}

		namespace Impl
		{
			HRESULT PrimitiveModel::CreateBufferPos( const std::vector<Vertex::Pos> &source )
			{
				return Donya::CreateVertexBuffer<Vertex::Pos>
				(
					Donya::GetDevice(), source,
					D3D11_USAGE_IMMUTABLE, NULL,
					pBufferPos.GetAddressOf()
				);
			}

			void PrimitiveModel::SetVertexBuffers()	const
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
			void PrimitiveModel::SetIndexBuffer()	const
			{
				SetNullIndexBuffer( Donya::GetImmediateContext() );
			}
		}

		namespace
		{
			using FindFunction = std::function<bool( int )>;
			int FindAvailableID( const FindFunction &IsAlreadyExists, int beginIndex = 0 )
			{
				for ( int i = beginIndex; i < INT_MAX; ++i )
				{
					if ( IsAlreadyExists( i ) ) { continue; }
					// else

					return i;
				}
				// else

				_ASSERT_EXPR( 0, L"Error : An available identifier was not found!" );
				return beginIndex - 1;
			}

			// These creation functions returns the identifier or NULL(if failed the creation).

			constexpr int DEFAULT_STATE_ID = -1;

			int CreateDS( const D3D11_DEPTH_STENCIL_DESC &desc )
			{
				int  id =  FindAvailableID( Donya::DepthStencil::IsAlreadyExists, DEFAULT_STATE_ID + 1 );
				if ( id == DEFAULT_STATE_ID ) { return DEFAULT_STATE_ID; } // The ID was not founded.
				// else

				bool result = Donya::DepthStencil::CreateState( id, desc );
				return ( result ) ? id : DEFAULT_STATE_ID;
			}
			int CreateRS( const D3D11_RASTERIZER_DESC &desc )
			{
				int  id = FindAvailableID( Donya::Rasterizer::IsAlreadyExists, DEFAULT_STATE_ID + 1 );
				if ( id == DEFAULT_STATE_ID ) { return DEFAULT_STATE_ID; } // The ID was not founded.
				// else

				bool result = Donya::Rasterizer::CreateState( id, desc );
				return ( result ) ? id : DEFAULT_STATE_ID;
			}
			int CreatePS( const D3D11_SAMPLER_DESC &desc )
			{
				int  id = FindAvailableID( Donya::Sampler::IsAlreadyExists, DEFAULT_STATE_ID + 1 );
				if ( id == DEFAULT_STATE_ID ) { return DEFAULT_STATE_ID; } // The ID was not founded.
				// else

				bool result = Donya::Sampler::CreateState( id, desc );
				return ( result ) ? id : DEFAULT_STATE_ID;
			}

			static constexpr D3D11_DEPTH_STENCIL_DESC	BasicDepthStencilDesc()
			{
				D3D11_DEPTH_STENCIL_DESC standard{};
				standard.DepthEnable		= TRUE;
				standard.DepthWriteMask		= D3D11_DEPTH_WRITE_MASK_ALL;
				standard.DepthFunc			= D3D11_COMPARISON_LESS;
				standard.StencilEnable		= FALSE;
				return standard;
			}
			static constexpr D3D11_RASTERIZER_DESC		BasicRasterizerDesc()
			{
				D3D11_RASTERIZER_DESC standard{};
				standard.FillMode				= D3D11_FILL_SOLID;
				standard.CullMode				= D3D11_CULL_BACK;
				standard.FrontCounterClockwise	= FALSE;
				standard.DepthBias				= 0;
				standard.DepthBiasClamp			= 0;
				standard.SlopeScaledDepthBias	= 0;
				standard.DepthClipEnable		= TRUE;
				standard.ScissorEnable			= FALSE;
				standard.MultisampleEnable		= FALSE;
				standard.AntialiasedLineEnable	= TRUE;
				return standard;
			}
		}

	#pragma region Cube

		struct CubeConfigration
		{
			std::array<Vertex::Pos, 4U * 6U> vertices;	// "4 * 6" represents six squares.
			std::array<size_t, 3U * 2U * 6U> indices;	// "3 * 2 * 6" represents the six groups of the square that constructed by two triangles.
		};
		CubeConfigration CreateCube()
		{
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
			constexpr size_t STRIDE_IDX = 3U * 2U;			// Because the square is made up of 2 triangles, and that triangle is made up of 3 vertices.
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

		#if 1 // Use for loop version

			constexpr Donya::Vector3 TOP_NORMAL		{ 0.0f, +1.0f, 0.0f };
			constexpr Donya::Vector3 BOTTOM_NORMAL	{ 0.0f, -1.0f, 0.0f };
			constexpr Donya::Vector3 RIGHT_NORMAL	{ +1.0f, 0.0f, 0.0f };
			constexpr Donya::Vector3 LEFT_NORMAL	{ -1.0f, 0.0f, 0.0f };
			constexpr Donya::Vector3 FAR_NORMAL		{ 0.0f, 0.0f, +1.0f };
			constexpr Donya::Vector3 NEAR_NORMAL	{ 0.0f, 0.0f, -1.0f };

			constexpr std::array<Donya::Vector3, STRIDE_VTX> TOP_VERTICES
			{
				Donya::Vector3{ -DIST, +DIST, +DIST }, // LTF
				Donya::Vector3{ +DIST, +DIST, +DIST }, // RTF
				Donya::Vector3{ -DIST, +DIST, -DIST }, // LTN
				Donya::Vector3{ +DIST, +DIST, -DIST }, // RTN
			};
			constexpr std::array<Donya::Vector3, STRIDE_VTX> BOTTOM_VERTICES
			{
				Donya::Vector3{ -DIST, -DIST, +DIST }, // LBF
				Donya::Vector3{ +DIST, -DIST, +DIST }, // RBF
				Donya::Vector3{ -DIST, -DIST, -DIST }, // LBN
				Donya::Vector3{ +DIST, -DIST, -DIST }, // RBN
			};
			constexpr std::array<Donya::Vector3, STRIDE_VTX> RIGHT_VERTICES
			{
				Donya::Vector3{ +DIST, +DIST, -DIST }, // RTN
				Donya::Vector3{ +DIST, +DIST, +DIST }, // RTF
				Donya::Vector3{ +DIST, -DIST, -DIST }, // RBN
				Donya::Vector3{ +DIST, -DIST, +DIST }, // RBF
			};
			constexpr std::array<Donya::Vector3, STRIDE_VTX> LEFT_VERTICES
			{
				Donya::Vector3{ -DIST, +DIST, -DIST }, // LTN
				Donya::Vector3{ -DIST, +DIST, +DIST }, // LTF
				Donya::Vector3{ -DIST, -DIST, -DIST }, // LBN
				Donya::Vector3{ -DIST, -DIST, +DIST }, // LBF
			};
			constexpr std::array<Donya::Vector3, STRIDE_VTX> FAR_VERTICES
			{
				Donya::Vector3{ +DIST, -DIST, +DIST }, // RBF
				Donya::Vector3{ +DIST, +DIST, +DIST }, // RTF
				Donya::Vector3{ -DIST, -DIST, +DIST }, // LBF
				Donya::Vector3{ -DIST, +DIST, +DIST }, // LTF
			};
			constexpr std::array<Donya::Vector3, STRIDE_VTX> NEAR_VERTICES
			{
				Donya::Vector3{ +DIST, -DIST, -DIST }, // RBN
				Donya::Vector3{ +DIST, +DIST, -DIST }, // RTN
				Donya::Vector3{ -DIST, -DIST, -DIST }, // LBN
				Donya::Vector3{ -DIST, +DIST, -DIST }, // LTN
			};
		
			struct Step // These pointers are used for reference only.
			{
				const size_t stride = 0;
				const Donya::Vector3							*pNormal	= nullptr;
				const std::array<Donya::Vector3, STRIDE_VTX>	*pVertices	= nullptr;
				const std::array<size_t, STRIDE_IDX>			*pIndices	= nullptr;
			public:
				constexpr Step
				(
					size_t stride,
					const Donya::Vector3							&normal,
					const std::array<Donya::Vector3, STRIDE_VTX>	&vertices,
					const std::array<size_t, STRIDE_IDX>			&indices
				) : stride( stride ), pNormal( &normal ), pVertices( &vertices ), pIndices( &indices )
				{}
				constexpr Step( const Step &source )
					: stride( source.stride ), pNormal( source.pNormal ), pVertices( source.pVertices ), pIndices( source.pIndices )
				{}
			};
			constexpr std::array<Step, 6U> SQUARES
			{
				Step{ 0U, TOP_NORMAL,		TOP_VERTICES,		CW_INDICES	},
				Step{ 1U, BOTTOM_NORMAL,	BOTTOM_VERTICES,	CCW_INDICES	},
				Step{ 2U, RIGHT_NORMAL,		RIGHT_VERTICES,		CW_INDICES	},
				Step{ 3U, LEFT_NORMAL,		LEFT_VERTICES,		CCW_INDICES	},
				Step{ 4U, FAR_NORMAL,		FAR_VERTICES,		CW_INDICES	},
				Step{ 5U, NEAR_NORMAL,		NEAR_VERTICES,		CCW_INDICES	}
			};

			CubeConfigration cube{};

			size_t vStride = 0U;
			size_t iStride = 0U;
			for ( const auto &it : SQUARES )
			{
				vStride = STRIDE_VTX * it.stride;
				iStride = STRIDE_IDX * it.stride;
				for ( size_t i = 0; i < STRIDE_VTX; ++i )
				{
					cube.vertices[vStride + i].position = it.pVertices->at( i );
					cube.vertices[vStride + i].normal   = *it.pNormal;
				}
				for ( size_t i = 0; i < STRIDE_IDX; ++i )
				{
					cube.indices[iStride + i] = it.pIndices->at( i ) + vStride;
				}
			}

		#else
		
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

		#endif // Use for loop version

			return cube;
		}

		bool Cube::Create()
		{
			if ( wasCreated ) { return true; }
			// else

			const auto cubeSource = CreateCube();

			// The creation method requires std::vector.

			const std::vector<Vertex::Pos>	vectorVertices{ cubeSource.vertices.begin(), cubeSource.vertices.end() };
			const std::vector<size_t>		vectorIndices { cubeSource.indices.begin(),  cubeSource.indices.end()  };

			ID3D11Device	*pDevice	= Donya::GetDevice();
			HRESULT			hr			= S_OK;

			hr = CreateBufferPos  ( vectorVertices );
			if ( FAILED( hr ) ) { AssertBaseCreation( "buffer", "Vertex::Pos" ); return false; }
			// else

			hr = CreateBufferIndex( vectorIndices  );
			if ( FAILED( hr ) ) { AssertBaseCreation( "buffer", "Index" ); return false; }
			// else

			wasCreated = true;
			return true;
		}
		HRESULT Cube::CreateBufferIndex( const std::vector<size_t> &source )
		{
			return Donya::CreateIndexBuffer
			(
				Donya::GetDevice(), source, pIndexBuffer.GetAddressOf()
			);
		}

		void Cube::SetIndexBuffer() const
		{
			ID3D11DeviceContext *pImmediateContext = Donya::GetImmediateContext();
			pImmediateContext->IASetIndexBuffer( pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );
		}
		void Cube::SetPrimitiveTopology() const
		{
			ID3D11DeviceContext *pImmediateContext = Donya::GetImmediateContext();
			pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		}

		void Cube::CallDraw() const
		{
			constexpr UINT INDEX_COUNT = 3U * 2U * 6U;
			ID3D11DeviceContext *pImmediateContext = Donya::GetImmediateContext();
			pImmediateContext->DrawIndexed( INDEX_COUNT, 0U, 0 );
		}


		bool CubeRenderer::Create()
		{
			ID3D11Device *pDevice = Donya::GetDevice();
			bool result		= true;
			bool succeeded	= true;

			if ( !cbuffer.Create() )
			{
				AssertBaseCreation( "cbuffer", "Primitive" );
				succeeded = false;
			}

			// States.
			{
				idDS = CreateDS( BasicDepthStencilDesc()	);
				idRS = CreateRS( BasicRasterizerDesc()		);
				if ( idDS == DEFAULT_STATE_ID )
				{
					AssertBaseCreation( "state", "DepthStencil" );
					succeeded = false;
				}
				if ( idRS == DEFAULT_STATE_ID )
				{
					AssertBaseCreation( "state", "Rasterizer" );
					succeeded = false;
				}
			}

			// Shaders.
			{
				const auto arrayIEDesc = Vertex::Pos::GenerateInputElements( 0 );

				result = VS.CreateByEmbededSourceCode
				(
					ShaderSource::BasicNameVS, ShaderSource::BasicCode(), ShaderSource::EntryPointVS,
					{ arrayIEDesc.begin(), arrayIEDesc.end() },
					pDevice
				);
				if ( !result )
				{
					AssertBaseCreation( "shader", "Cube::VS" );
					succeeded = false;
				}

				result = PS.CreateByEmbededSourceCode
				(
					ShaderSource::BasicNamePS, ShaderSource::BasicCode(), ShaderSource::EntryPointPS,
					pDevice
				);
				if ( !result )
				{
					AssertBaseCreation( "shader", "Cube::PS" );
					succeeded = false;
				}
			}

			return succeeded;
		}
		void CubeRenderer::ActivateConstant()
		{
			const auto desc = ShaderSource::BasicPrimitiveSetting();
			PrimitiveRenderer::ActivateConstant( desc.setSlot, desc.setVS, desc.setPS );
		}
		void CubeRenderer::Draw( const Cube &cube )
		{
			cube.SetVertexBuffers();
			cube.SetIndexBuffer();
			cube.SetPrimitiveTopology();
			cube.CallDraw();
		}

	// region Cube
	#pragma endregion

	#pragma region Sphere

		struct SphereConfigration
		{
			std::vector<Vertex::Pos>	vertices;
			std::vector<size_t>			indices;
		};
		SphereConfigration CreateSphere( size_t sliceCountH, size_t sliceCountV )
		{
			// see http://rudora7.blog81.fc2.com/blog-entry-388.html

			constexpr float RADIUS = 1.0f / 2.0f;
			constexpr Donya::Vector3 CENTER{ 0.0f, 0.0f, 0.0f };

			SphereConfigration sphere{};

			// Make Vertices
			{
				auto MakeVertex = [&CENTER]( const Donya::Vector3 &pos )
				{
					Vertex::Pos v{};
					v.position	= pos;
					v.normal	= Donya::Vector3{ pos - CENTER }.Unit();
					return v;
				};
				auto PushVertex = [&sphere]( const Vertex::Pos &vertex )->void
				{
					sphere.vertices.emplace_back( vertex );
				};
			
				const Vertex::Pos TOP_VERTEX = MakeVertex( CENTER + Donya::Vector3{ 0.0f, RADIUS, 0.0f } );
				PushVertex( TOP_VERTEX );

				const float xyPlaneStep = ToRadian( 180.0f ) / scast<float>( sliceCountV );		// Line-up to vertically.
				const float xzPlaneStep = ToRadian( 360.0f ) / scast<float>( sliceCountH );		// Line-up to horizontally.

				constexpr float BASE_THETA = ToRadian( 90.0f ); // Use cosf(), sinf() with start from top(90-degrees).

				float nowRadius{};
				Donya::Vector3 pos{};
				for ( size_t vertical = 1; vertical < sliceCountV; ++vertical )
				{
					nowRadius	=  cosf( BASE_THETA + ( xyPlaneStep * vertical ) ) * RADIUS;
					pos.y		=  sinf( BASE_THETA + ( xyPlaneStep * vertical ) ) * RADIUS;
					pos.y		+= CENTER.y;

					for ( size_t horizontal = 0; horizontal <= sliceCountH; ++horizontal )
					{
						pos.x = CENTER.x + cosf( xzPlaneStep * horizontal ) * nowRadius;
						pos.z = CENTER.z + sinf( xzPlaneStep * horizontal ) * nowRadius;

						PushVertex( MakeVertex( pos ) );
					}
				}

				const Vertex::Pos BOTTOM_VERTEX = MakeVertex( CENTER - Donya::Vector3{ 0.0f, RADIUS, 0.0f } );
				PushVertex( BOTTOM_VERTEX );
			}

			// Make Triangle Indices
			{
				auto PushIndex = [&sphere]( size_t index )->void
				{
					sphere.indices.emplace_back( index );
				};

				// Make triangles with top.
				{
					const size_t TOP_INDEX = 0;

					for ( size_t i = 1; i <= sliceCountH; ++i )
					{
						PushIndex( TOP_INDEX );
						PushIndex( i + 1 );
						PushIndex( i );
					}
				}

				const size_t vertexCountPerRing = sliceCountH + 1;

				// Make triangles of inner.
				{
					const size_t BASE_INDEX = 1; // Start next of top vertex.

					size_t step{};		// The index of current ring.
					size_t nextStep{};	// The index of next ring.
					for ( size_t ring = 0; ring < sliceCountV - 2/* It's OK to "-1" also */; ++ring )
					{
						step		= ( ring ) * vertexCountPerRing;
						nextStep	= ( ring + 1 ) * vertexCountPerRing;

						for ( size_t i = 0; i < sliceCountH; ++i )
						{
							PushIndex( BASE_INDEX + step		+ i		);
							PushIndex( BASE_INDEX + step		+ i + 1	);
							PushIndex( BASE_INDEX + nextStep	+ i		);
							
							PushIndex( BASE_INDEX + nextStep	+ i		);
							PushIndex( BASE_INDEX + step		+ i + 1	);
							PushIndex( BASE_INDEX + nextStep	+ i + 1	);
						}
					}
				}

				// Make triangles with bottom.
				{
					const size_t BOTTOM_INDEX = sphere.vertices.size() - 1;
					const size_t BASE_INDEX   = BOTTOM_INDEX - vertexCountPerRing;

					for ( size_t i = 0; i < sliceCountH; ++i )
					{
						PushIndex( BOTTOM_INDEX );
						PushIndex( BASE_INDEX + i );
						PushIndex( BASE_INDEX + i + 1 );
					}
				}
			}

			return sphere;
		}

		Sphere::Sphere( size_t sliceH, size_t sliceV )
			: sliceCountH( sliceH ), sliceCountV( sliceV ), indexCount(), pIndexBuffer()
		{}
		bool Sphere::Create()
		{
			if ( wasCreated ) { return true; }
			// else

			const auto		sphereSource	= CreateSphere( sliceCountH, sliceCountV );
			ID3D11Device	*pDevice = Donya::GetDevice();
			HRESULT			hr = S_OK;

			hr = CreateBufferPos  ( sphereSource.vertices );
			if ( FAILED( hr ) ) { AssertBaseCreation( "buffer", "Vertex::Pos" ); return false; }
			// else

			hr = CreateBufferIndex( sphereSource.indices  );
			if ( FAILED( hr ) ) { AssertBaseCreation( "buffer", "Index" ); return false; }
			// else

			indexCount = sphereSource.indices.size();

			wasCreated = true;
			return true;
		}
		HRESULT Sphere::CreateBufferIndex( const std::vector<size_t> &source )
		{
			return Donya::CreateIndexBuffer
			(
				Donya::GetDevice(), source, pIndexBuffer.GetAddressOf()
			);
		}

		void Sphere::SetIndexBuffer() const
		{
			ID3D11DeviceContext *pImmediateContext = Donya::GetImmediateContext();
			pImmediateContext->IASetIndexBuffer( pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );
		}
		void Sphere::SetPrimitiveTopology() const
		{
			ID3D11DeviceContext *pImmediateContext = Donya::GetImmediateContext();
			pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		}

		void Sphere::CallDraw() const
		{
			ID3D11DeviceContext *pImmediateContext = Donya::GetImmediateContext();
			pImmediateContext->DrawIndexed( indexCount, 0U, 0 );
		}


		bool SphereRenderer::Create()
		{
			ID3D11Device *pDevice = Donya::GetDevice();
			bool result		= true;
			bool succeeded	= true;

			if ( !cbuffer.Create() )
			{
				AssertBaseCreation( "cbuffer", "Primitive" );
				succeeded = false;
			}

			// States.
			{
				idDS = CreateDS( BasicDepthStencilDesc()	);
				idRS = CreateRS( BasicRasterizerDesc()		);
				if ( idDS == DEFAULT_STATE_ID )
				{
					AssertBaseCreation( "state", "DepthStencil" );
					succeeded = false;
				}
				if ( idRS == DEFAULT_STATE_ID )
				{
					AssertBaseCreation( "state", "Rasterizer" );
					succeeded = false;
				}
			}

			// Shaders.
			{
				const auto arrayIEDesc = Vertex::Pos::GenerateInputElements( 0 );

				result = VS.CreateByEmbededSourceCode
				(
					ShaderSource::BasicNameVS, ShaderSource::BasicCode(), ShaderSource::EntryPointVS,
					{ arrayIEDesc.begin(), arrayIEDesc.end() },
					pDevice
				);
				if ( !result )
				{
					AssertBaseCreation( "shader", "Sphere::VS" );
					succeeded = false;
				}

				result = PS.CreateByEmbededSourceCode
				(
					ShaderSource::BasicNamePS, ShaderSource::BasicCode(), ShaderSource::EntryPointPS,
					pDevice
				);
				if ( !result )
				{
					AssertBaseCreation( "shader", "Sphere::PS" );
					succeeded = false;
				}
			}

			return succeeded;
		}
		void SphereRenderer::ActivateConstant()
		{
			const auto desc = ShaderSource::BasicPrimitiveSetting();
			PrimitiveRenderer::ActivateConstant( desc.setSlot, desc.setVS, desc.setPS );
		}
		void SphereRenderer::Draw( const Sphere &sphere )
		{
			sphere.SetVertexBuffers();
			sphere.SetIndexBuffer();
			sphere.SetPrimitiveTopology();
			sphere.CallDraw();
		}

	// region Sphere
	#pragma endregion
	}
}
