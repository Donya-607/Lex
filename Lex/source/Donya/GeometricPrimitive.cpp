#include "GeometricPrimitive.h"

#include <array>

#include "Color.h"
#include "Constant.h"
#include "Direct3DUtil.h"
#include "Donya.h"
#include "Resource.h"
#include "Useful.h"

using namespace Donya;
using namespace DirectX;

namespace Donya
{
	namespace Geometric
	{
		Base::Base() :
			iVertexBuffer(), iIndexBuffer(), iConstantBuffer(),
			iInputLayout(), iVertexShader(), iPixelShader(),
			iRasterizerStateWire(), iRasterizerStateSurface(), iDepthStencilState(),
			indicesCount( NULL ), wasCreated( false )
		{}

		constexpr const char		*GeometryShaderSourceCode()
		{
			return
			"struct VS_IN\n"
			"{\n"
			"	float4 pos		: POSITION;\n"
			"	float4 normal	: NORMAL;\n"
			"};\n"
			"struct VS_OUT\n"
			"{\n"
			"	float4 pos		: SV_POSITION;\n"
			"	float4 color	: COLOR;\n"
			"};\n"
			"cbuffer CONSTANT_BUFFER : register( b0 )\n"
			"{\n"
			"	row_major\n"
			"	float4x4	worldViewProjection;\n"
			"	row_major\n"
			"	float4x4	world;\n"
			"	float4		lightDirection;\n"
			"	float4		lightColor;\n"
			"	float4		materialColor;\n"
			"};\n"
			"VS_OUT VSMain( VS_IN vin )\n"
			"{\n"
			"	vin.normal.w	= 0;\n"
			"	float4 norm		= normalize( mul( vin.normal, world ) );\n"
			"	float4 light	= normalize( -lightDirection );\n"
			"	float  NL		= saturate( dot( light, norm ) );\n"
			"	NL				= NL * 0.5f + 0.5f;\n"
			"	VS_OUT vout;\n"
			"	vout.pos		= mul( vin.pos, worldViewProjection );\n"
			"	vout.color		= materialColor * NL;\n"
			"	vout.color.a	= materialColor.a;\n"
			"	return vout;\n"
			"}\n"
			"float4 PSMain( VS_OUT pin ) : SV_TARGET\n"
			"{\n"
			"	if ( pin.color.a <= 0 ) { discard; }\n"
			"	return pin.color * float4( lightColor.rgb * lightColor.w, 1.0f );\n"
			"}\n"
			;
		}
		constexpr const char		*GeometryShaderNameVS()
		{
			return "GeometryVS";
		}
		constexpr const char		*GeometryShaderNamePS()
		{
			return "GeometryPS";
		}
		constexpr const char		*GeometryShaderEntryPointVS()
		{
			return "VSMain";
		}
		constexpr const char		*GeometryShaderEntryPointPS()
		{
			return "PSMain";
		}

		/// <summary>
		/// Contain : [0:POSITION], [1:NORMAL].
		/// </summary>
		constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2>	GeometryInputElementDescs()
		{
			return std::array<D3D11_INPUT_ELEMENT_DESC, 2>
			{
				D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
				D3D11_INPUT_ELEMENT_DESC{ "NORMAL"	, 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
		}
		constexpr D3D11_RASTERIZER_DESC						GeometryRasterizerDesc( D3D11_FILL_MODE fillMode )
		{
			D3D11_RASTERIZER_DESC standard{};
			standard.FillMode				= fillMode;
			standard.CullMode				= D3D11_CULL_BACK;
			standard.FrontCounterClockwise	= FALSE;
			standard.DepthBias				= 0;
			standard.DepthBiasClamp			= 0;
			standard.SlopeScaledDepthBias	= 0;
			standard.DepthClipEnable		= TRUE;
			standard.ScissorEnable			= FALSE;
			standard.MultisampleEnable		= FALSE;
			standard.AntialiasedLineEnable	= ( fillMode == D3D11_FILL_WIREFRAME )
											? TRUE
											: FALSE;

			return standard;
		}
		constexpr D3D11_DEPTH_STENCIL_DESC					GeometryDepthStencilDesc()
		{
			D3D11_DEPTH_STENCIL_DESC standard{};
			standard.DepthEnable			= TRUE;							// default : TRUE ( Z-Test:ON )
			standard.DepthWriteMask			= D3D11_DEPTH_WRITE_MASK_ALL;	// default : D3D11_DEPTH_WRITE_ALL ( Z-Write:ON, OFF is D3D11_DEPTH_WRITE_ZERO, does not means write zero! )
			standard.DepthFunc				= D3D11_COMPARISON_LESS;		// default : D3D11_COMPARISION_LESS ( ALWAYS:always pass )
			standard.StencilEnable			= FALSE;

			return standard;
		}

	#pragma region Cube

		Cube::Cube() : Base()
		{}
		Cube::~Cube() = default;

		/// <summary>
		/// The "Vertex" must has "pos" and "normal" at member! doing by duck-typing.<para></para>
		/// The vertices is place at [-0.5f ~ +0.5f], the center is 0.0f.<para></para>
		/// Returns pair is:<para></para>
		/// First : Vertices array, size is 4 * 6(Rect vertices(4) to cubic(6)).<para></para>
		/// Second : Indices array, size is 3 * 2 * 6(Triangle(3) to rect(2) to cubic(6)).
		/// </summary>
		template<class Vertex>
		std::pair<std::array<Vertex, 4 * 6>, std::array<size_t, 3 * 2 * 6>> MakeCube()
		{
			std::array<Vertex, 4 * 6>		vertices{};
			std::array<size_t, 3 * 2 * 6>	indices{};

			// HACK:I should be refactoring this makeing cube method, doing hard-coding :(
			{
				auto MakeVertex = []( XMFLOAT3 pos, XMFLOAT3 normal )->Vertex
				{
					Vertex v{};
					v.pos = pos;
					v.normal = normal;
					return v;
				};
			
				size_t vIndex = 0;
				size_t iIndex = 0;
				// Top
				{
					XMFLOAT3 norm{ 0.0f, 1.0f, 0.0f };
					vertices[vIndex + 0] = MakeVertex( { -0.5f, +0.5f, +0.5f }, norm ); // LTF
					vertices[vIndex + 1] = MakeVertex( { +0.5f, +0.5f, +0.5f }, norm ); // RTF
					vertices[vIndex + 2] = MakeVertex( { -0.5f, +0.5f, -0.5f }, norm ); // LTB
					vertices[vIndex + 3] = MakeVertex( { +0.5f, +0.5f, -0.5f }, norm ); // RTB

					indices[iIndex + 0] = vIndex + 0;
					indices[iIndex + 1] = vIndex + 1;
					indices[iIndex + 2] = vIndex + 2;

					indices[iIndex + 3] = vIndex + 1;
					indices[iIndex + 4] = vIndex + 3;
					indices[iIndex + 5] = vIndex + 2;
				}
				vIndex += 4;
				iIndex += 6;
				// Bottom
				{
					XMFLOAT3 norm{ 0.0f, -1.0f, 0.0f };
					vertices[vIndex + 0] = MakeVertex( { -0.5f, -0.5f, +0.5f }, norm ); // LBF
					vertices[vIndex + 1] = MakeVertex( { +0.5f, -0.5f, +0.5f }, norm ); // RBF
					vertices[vIndex + 2] = MakeVertex( { -0.5f, -0.5f, -0.5f }, norm ); // LBB
					vertices[vIndex + 3] = MakeVertex( { +0.5f, -0.5f, -0.5f }, norm ); // RBB

					indices[iIndex + 0] = vIndex + 0;
					indices[iIndex + 1] = vIndex + 2;
					indices[iIndex + 2] = vIndex + 1;

					indices[iIndex + 3] = vIndex + 1;
					indices[iIndex + 4] = vIndex + 2;
					indices[iIndex + 5] = vIndex + 3;
				}
				vIndex += 4;
				iIndex += 6;
				// Right
				{
					XMFLOAT3 norm{ 1.0f, 0.0f, 0.0f };
					vertices[vIndex + 0] = MakeVertex( { +0.5f, +0.5f, -0.5f }, norm ); // RTB
					vertices[vIndex + 1] = MakeVertex( { +0.5f, +0.5f, +0.5f }, norm ); // RTF
					vertices[vIndex + 2] = MakeVertex( { +0.5f, -0.5f, -0.5f }, norm ); // RBB
					vertices[vIndex + 3] = MakeVertex( { +0.5f, -0.5f, +0.5f }, norm ); // RBF

					indices[iIndex + 0] = vIndex + 0;
					indices[iIndex + 1] = vIndex + 1;
					indices[iIndex + 2] = vIndex + 2;

					indices[iIndex + 3] = vIndex + 1;
					indices[iIndex + 4] = vIndex + 3;
					indices[iIndex + 5] = vIndex + 2;
				}
				vIndex += 4;
				iIndex += 6;
				// Left
				{
					XMFLOAT3 norm{ -1.0f, 0.0f, 0.0f };
					vertices[vIndex + 0] = MakeVertex( { -0.5f, +0.5f, -0.5f }, norm ); // LTB
					vertices[vIndex + 1] = MakeVertex( { -0.5f, +0.5f, +0.5f }, norm ); // LTF
					vertices[vIndex + 2] = MakeVertex( { -0.5f, -0.5f, -0.5f }, norm ); // LBB
					vertices[vIndex + 3] = MakeVertex( { -0.5f, -0.5f, +0.5f }, norm ); // LBF

					indices[iIndex + 0] = vIndex + 0;
					indices[iIndex + 1] = vIndex + 2;
					indices[iIndex + 2] = vIndex + 1;

					indices[iIndex + 3] = vIndex + 1;
					indices[iIndex + 4] = vIndex + 2;
					indices[iIndex + 5] = vIndex + 3;
				}
				vIndex += 4;
				iIndex += 6;
				// Back
				{
					XMFLOAT3 norm{ 0.0f, 0.0f, 1.0f };
					vertices[vIndex + 0] = MakeVertex( { +0.5f, -0.5f, +0.5f }, norm ); // RBF
					vertices[vIndex + 1] = MakeVertex( { +0.5f, +0.5f, +0.5f }, norm ); // RTF
					vertices[vIndex + 2] = MakeVertex( { -0.5f, -0.5f, +0.5f }, norm ); // LBF
					vertices[vIndex + 3] = MakeVertex( { -0.5f, +0.5f, +0.5f }, norm ); // LTF

					indices[iIndex + 0] = vIndex + 0;
					indices[iIndex + 1] = vIndex + 1;
					indices[iIndex + 2] = vIndex + 2;

					indices[iIndex + 3] = vIndex + 1;
					indices[iIndex + 4] = vIndex + 3;
					indices[iIndex + 5] = vIndex + 2;
				}
				vIndex += 4;
				iIndex += 6;
				// Front
				{
					XMFLOAT3 norm{ 0.0f, 0.0f, -1.0f };
					vertices[vIndex + 0] = MakeVertex( { +0.5f, -0.5f, -0.5f }, norm ); // RBB
					vertices[vIndex + 1] = MakeVertex( { +0.5f, +0.5f, -0.5f }, norm ); // RTB
					vertices[vIndex + 2] = MakeVertex( { -0.5f, -0.5f, -0.5f }, norm ); // LBB
					vertices[vIndex + 3] = MakeVertex( { -0.5f, +0.5f, -0.5f }, norm ); // LTB

					indices[iIndex + 0] = vIndex + 0;
					indices[iIndex + 1] = vIndex + 2;
					indices[iIndex + 2] = vIndex + 1;

					indices[iIndex + 3] = vIndex + 1;
					indices[iIndex + 4] = vIndex + 2;
					indices[iIndex + 5] = vIndex + 3;
				}
				vIndex += 4;
				iIndex += 6;
			}

			return std::make_pair( vertices, indices );
		}

		void Cube::Init()
		{
			if ( wasCreated ) { return; }
			// else

			HRESULT hr = S_OK;
			ID3D11Device *pDevice = Donya::GetDevice();

			auto arrayPair = MakeCube<Cube::Vertex>();
			indicesCount = arrayPair.second.size();

			// Create VertexBuffer
			{
				hr = CreateVertexBuffer<Cube::Vertex>
				(
					pDevice,
					// CreateVertexBuffer want std::vector.
					std::vector<Vertex>( arrayPair.first.begin(), arrayPair.first.end() ),
					iVertexBuffer.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Vertex-Buffer." );
			}
			// Create IndexBuffer
			{
				hr = CreateIndexBuffer
				(
					pDevice,
					// CreateIndexBuffer want std::vector.
					std::vector<size_t>( arrayPair.second.begin(), arrayPair.second.end() ),
					iIndexBuffer.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Index-Buffer." );
			}
			// Create ConstantBuffer
			{
				hr = CreateConstantBuffer
				(
					pDevice,
					sizeof( Cube::ConstantBuffer ),
					iConstantBuffer.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Constant-Buffer." );
			}
			// Create VertexShader and InputLayout
			{
				constexpr auto INPUT_ELEMENT_DESCS = GeometryInputElementDescs();

				Resource::CreateVertexShaderFromSource
				(
					pDevice,
					GeometryShaderNameVS(),
					GeometryShaderSourceCode(),
					GeometryShaderEntryPointVS(),
					iVertexShader.ReleaseAndGetAddressOf(),
					iInputLayout.ReleaseAndGetAddressOf(),
					INPUT_ELEMENT_DESCS.data(),
					INPUT_ELEMENT_DESCS.size()
				);
			}
			// Create PixelShader
			{
				Resource::CreatePixelShaderFromSource
				(
					pDevice,
					GeometryShaderNamePS(),
					GeometryShaderSourceCode(),
					GeometryShaderEntryPointPS(),
					iPixelShader.ReleaseAndGetAddressOf()
				);
			}
			// Create Rasterizer States
			{
				D3D11_RASTERIZER_DESC rdWireFrame	= GeometryRasterizerDesc( D3D11_FILL_WIREFRAME	);
				D3D11_RASTERIZER_DESC rdSurface		= GeometryRasterizerDesc( D3D11_FILL_SOLID		);
				
				hr = pDevice->CreateRasterizerState
				(
					&rdWireFrame,
					iRasterizerStateWire.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Rasterizer-State of WireFrame." );

				hr = pDevice->CreateRasterizerState
				(
					&rdSurface,
					iRasterizerStateSurface.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Rasterizer-State of Surface." );
			}
			// Create DepthsStencilState
			{
				D3D11_DEPTH_STENCIL_DESC desc = GeometryDepthStencilDesc();

				hr = pDevice->CreateDepthStencilState
				(
					&desc,
					iDepthStencilState.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), "Failed : Create Depth-Stencil-State." );
			}

			wasCreated = true;
		}
		void Cube::Uninit()
		{
			// NOP.
		}

		void Cube::Render( ID3D11DeviceContext *pImmediateContext, bool useDefaultShading, bool isEnableFill, const XMFLOAT4X4 &defMatWVP, const XMFLOAT4X4 &defMatW, const XMFLOAT4 &defLightDir, const XMFLOAT4 &defMtlColor ) const
		{
			if ( !wasCreated )
			{
				_ASSERT_EXPR( 0, L"Error : The cube was not created!" );
				return;
			}
			// else

			HRESULT hr = S_OK;

			// Use default context.
			if ( !pImmediateContext )
			{
				pImmediateContext = Donya::GetImmediateContext();
			}

			if ( useDefaultShading )
			{
				ConstantBuffer cb;
				cb.worldViewProjection	= defMatWVP;
				cb.world				= defMatW;
				cb.lightDirection		= defLightDir;
				cb.lightColor			= { 1.0f, 1.0f, 1.0f, 1.0f };
				cb.materialColor		= defMtlColor;
				cb.materialColor.w		= Donya::Color::FilteringAlpha( cb.materialColor.w );
				
				pImmediateContext->UpdateSubresource( iConstantBuffer.Get(), 0, nullptr, &cb, 0, 0 );
				pImmediateContext->VSSetConstantBuffers( 0, 1, iConstantBuffer.GetAddressOf() );
				pImmediateContext->PSSetConstantBuffers( 0, 1, iConstantBuffer.GetAddressOf() );
			}

			// Settings
			{
				UINT stride = sizeof( Cube::Vertex );
				UINT offset = 0;
				pImmediateContext->IASetVertexBuffers( 0, 1, iVertexBuffer.GetAddressOf(), &stride, &offset );
				pImmediateContext->IASetIndexBuffer( iIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );
				pImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

				if ( useDefaultShading )
				{
					pImmediateContext->IASetInputLayout( iInputLayout.Get() );
					pImmediateContext->VSSetShader( iVertexShader.Get(), nullptr, 0 );
				}

				ID3D11RasterizerState	*ppRasterizerState =
										( isEnableFill )
										? iRasterizerStateSurface.Get()
										: iRasterizerStateWire.Get();
				pImmediateContext->RSSetState( ppRasterizerState );

				if ( useDefaultShading )
				{
					pImmediateContext->PSSetShader( iPixelShader.Get(), nullptr, 0 );
				}

				pImmediateContext->OMSetDepthStencilState( iDepthStencilState.Get(), 0xffffffff );
			}

			pImmediateContext->DrawIndexed( indicesCount, 0, 0 );
		}

	// region Cube
	#pragma endregion

	#pragma region Sphere

		Sphere::Sphere( size_t hSlice, size_t vSlice ) : Base(),
			HORIZONTAL_SLICE( hSlice ), VERTICAL_SLICE( vSlice )
		{}
		Sphere::~Sphere() = default;

		/// <summary>
		/// The "Vertex" must has "pos" and "normal" at member! doing by duck-typing.<para></para>
		/// The vertices is place at [-0.5f ~ +0.5f], the center is 0.0f.<para></para>
		/// Returns pair is:<para></para>
		/// First : Vertices array.<para></para>
		/// Second : Indices array.
		/// </summary>
		template<class Vertex>
		std::pair<std::vector<Vertex>, std::vector<size_t>> MakeSphere( size_t horizontalSliceCount, size_t verticalSliceCount )
		{
			// see http://rudora7.blog81.fc2.com/blog-entry-388.html

			constexpr float RADIUS = 0.5f;
			constexpr XMFLOAT3 CENTER{ 0.0f, 0.0f, 0.0f };

			std::vector<Vertex> vertices{};
			std::vector<size_t> indices{};

			// Make Vertices
			{
				auto MakeVertex = [&CENTER]( XMFLOAT3 pos )
				{
					Vertex v{};
					v.pos		= pos;
					v.normal	= pos - CENTER;
					return v;
				};
				auto PushVertex = [&vertices]( Vertex vertex )->void
				{
					vertices.emplace_back( vertex );
				};
			
				const Vertex TOP_VERTEX = MakeVertex( CENTER + XMFLOAT3{ 0.0f, RADIUS, 0.0f } );
				PushVertex( TOP_VERTEX );

				const float xyPlaneStep = ToRadian( 180.0f ) / verticalSliceCount;		// Line-up to vertically.
				const float xzPlaneStep = ToRadian( 360.0f ) / horizontalSliceCount;	// Line-up to horizontally.

				constexpr float BASE_THETA = ToRadian( 90.0f ); // Use cosf(), sinf() with start from top(90-degrees).

				float radius{}; XMFLOAT3 pos{};
				for ( size_t vertical = 1; vertical < verticalSliceCount; ++vertical )
				{
					radius =			cosf( BASE_THETA + xyPlaneStep * vertical ) * RADIUS;
					pos.y  = CENTER.y + sinf( BASE_THETA + xyPlaneStep * vertical ) * RADIUS;

					for ( size_t horizontal = 0; horizontal <= horizontalSliceCount; ++horizontal )
					{
						pos.x = CENTER.x + cosf( xzPlaneStep * horizontal ) * radius;
						pos.z = CENTER.z + sinf( xzPlaneStep * horizontal ) * radius;

						PushVertex( MakeVertex( pos ) );
					}
				}

				const Vertex BOTTOM_VERTEX = MakeVertex( CENTER - XMFLOAT3{ 0.0f, RADIUS, 0.0f } );
				PushVertex( BOTTOM_VERTEX );
			}

			// Make Triangle Indices
			{
				auto PushIndex = [&indices]( size_t index )->void
				{
					indices.emplace_back( index );
				};

				// Make triangles with top.
				{
					const size_t TOP_INDEX = 0;

					for ( size_t i = 1; i <= horizontalSliceCount; ++i )
					{
						PushIndex( TOP_INDEX );
						PushIndex( i + 1 );
						PushIndex( i );
					}
				}

				const size_t VERTEX_COUNT_PER_RING = horizontalSliceCount + 1;

				// Make triangles of inner.
				{
					const size_t BASE_INDEX = 1; // Start next of top vertex.

					size_t step{};		// The index of current ring.
					size_t nextStep{};	// The index of next ring.
					for ( size_t ring = 0; ring < verticalSliceCount - 2/*It's OK to "-1" also*/; ++ring )
					{
						step		= ( ring ) * VERTEX_COUNT_PER_RING;
						nextStep	= ( ring + 1 ) * VERTEX_COUNT_PER_RING;

						for ( size_t i = 0; i < horizontalSliceCount; ++i )
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
					const size_t BOTTOM_INDEX = vertices.size() - 1;
					const size_t BASE_INDEX   = BOTTOM_INDEX - VERTEX_COUNT_PER_RING;

					for ( size_t i = 0; i < horizontalSliceCount; ++i )
					{
						PushIndex( BOTTOM_INDEX );
						PushIndex( BASE_INDEX + i );
						PushIndex( BASE_INDEX + i + 1 );
					}
				}
			}

			return std::make_pair( vertices, indices );
		}

		void Sphere::Init()
		{
			if ( wasCreated ) { return; }
			// else

			HRESULT hr = S_OK;
			ID3D11Device *pDevice = Donya::GetDevice();

			auto arrayPair = MakeSphere<Sphere::Vertex>( HORIZONTAL_SLICE, VERTICAL_SLICE );
			indicesCount   = arrayPair.second.size();

			// Create VertexBuffer
			{
				hr = CreateVertexBuffer<Sphere::Vertex>
				(
					pDevice,
					arrayPair.first,
					iVertexBuffer.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Vertex-Buffer." );
			}
			// Create IndexBuffer
			{
				hr = CreateIndexBuffer
				(
					pDevice,
					arrayPair.second,
					iIndexBuffer.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Index-Buffer." );
			}
			// Create ConstantBuffer
			{
				hr = CreateConstantBuffer
				(
					pDevice,
					sizeof( Sphere::ConstantBuffer ),
					iConstantBuffer.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Constant-Buffer." );
			}
			// Create VertexShader and InputLayout
			{
				constexpr auto INPUT_ELEMENT_DESCS = GeometryInputElementDescs();

				Resource::CreateVertexShaderFromSource
				(
					pDevice,
					GeometryShaderNameVS(),
					GeometryShaderSourceCode(),
					GeometryShaderEntryPointVS(),
					iVertexShader.ReleaseAndGetAddressOf(),
					iInputLayout.ReleaseAndGetAddressOf(),
					INPUT_ELEMENT_DESCS.data(),
					INPUT_ELEMENT_DESCS.size()
				);
			}
			// Create PixelShader
			{
				Resource::CreatePixelShaderFromSource
				(
					pDevice,
					GeometryShaderNamePS(),
					GeometryShaderSourceCode(),
					GeometryShaderEntryPointPS(),
					iPixelShader.ReleaseAndGetAddressOf()
				);
			}
			// Create Rasterizer States
			{
				D3D11_RASTERIZER_DESC rdWireFrame	= GeometryRasterizerDesc( D3D11_FILL_WIREFRAME );
				D3D11_RASTERIZER_DESC rdSurface		= GeometryRasterizerDesc( D3D11_FILL_SOLID );

				hr = pDevice->CreateRasterizerState
				(
					&rdWireFrame,
					iRasterizerStateWire.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Rasterizer-State of WireFrame." );

				hr = pDevice->CreateRasterizerState
				(
					&rdSurface,
					iRasterizerStateSurface.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Rasterizer-State of Surface." );
			}
			// Create DepthsStencilState
			{
				D3D11_DEPTH_STENCIL_DESC desc = GeometryDepthStencilDesc();

				hr = pDevice->CreateDepthStencilState
				(
					&desc,
					iDepthStencilState.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), "Failed : Create Depth-Stencil-State." );
			}

			wasCreated = true;
		}
		void Sphere::Uninit()
		{
			// NOP.
		}

		void Sphere::Render( ID3D11DeviceContext *pImmediateContext, bool useDefaultShading, bool isEnableFill, const XMFLOAT4X4 &defMatWVP, const XMFLOAT4X4 &defMatW, const XMFLOAT4 &defLightDir, const XMFLOAT4 &defMtlColor ) const
		{
			if ( !wasCreated )
			{
				_ASSERT_EXPR( 0, L"Error : The sphere was not created!" );
				return;
			}
			// else

			HRESULT hr = S_OK;
			
			// Use default context.
			if ( !pImmediateContext )
			{
				pImmediateContext = Donya::GetImmediateContext();
			}

			if ( useDefaultShading )
			{
				ConstantBuffer cb;
				cb.worldViewProjection	= defMatWVP;
				cb.world				= defMatW;
				cb.lightDirection		= defLightDir;
				cb.lightColor			= { 1.0f, 1.0f, 1.0f, 1.0f };
				cb.materialColor		= defMtlColor;
				cb.materialColor.w		= Donya::Color::FilteringAlpha( cb.materialColor.w );
				
				pImmediateContext->UpdateSubresource( iConstantBuffer.Get(), 0, nullptr, &cb, 0, 0 );
				pImmediateContext->VSSetConstantBuffers( 0, 1, iConstantBuffer.GetAddressOf() );
			}

			// Settings
			{
				UINT stride = sizeof( Sphere::Vertex );
				UINT offset = 0;
				pImmediateContext->IASetVertexBuffers( 0, 1, iVertexBuffer.GetAddressOf(), &stride, &offset );
				pImmediateContext->IASetIndexBuffer( iIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );
				pImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

				if ( useDefaultShading )
				{
					pImmediateContext->IASetInputLayout( iInputLayout.Get() );
					pImmediateContext->VSSetShader( iVertexShader.Get(), nullptr, 0 );
				}

				ID3D11RasterizerState	*ppRasterizerState =
										( isEnableFill )
										? iRasterizerStateSurface.Get()
										: iRasterizerStateWire.Get();
				pImmediateContext->RSSetState( ppRasterizerState );

				if ( useDefaultShading )
				{
					pImmediateContext->PSSetShader( iPixelShader.Get(), nullptr, 0 );
				}

				pImmediateContext->OMSetDepthStencilState( iDepthStencilState.Get(), 0xffffffff );
			}

			pImmediateContext->DrawIndexed( indicesCount, 0, 0 );
		}

	// region Sphere
	#pragma endregion

	#pragma region TextureBoard

		constexpr const char	*TextureBoardShaderSourceCode()
		{
			return
			"Texture2D		diffuseMap			: register( t0 );\n"
			"SamplerState	diffuseMapSampler	: register( s0 );\n"
			"struct VS_IN\n"
			"{\n"
			"	float4 pos					: POSITION;\n"
			"	float4 normal				: NORMAL;\n"
			"	float2 texCoord				: TEXCOORD;\n"
			"	float4 texCoordTransform	: TEXCOORD_TRANSFORM;\n"
			"};\n"
			"struct VS_OUT\n"
			"{\n"
			"	float4 pos		: SV_POSITION;\n"
			"	float4 color	: COLOR;\n"
			"	float2 texCoord	: TEXCOORD;\n"
			"};\n"
			"cbuffer CONSTANT_BUFFER : register( b0 )\n"
			"{\n"
			"	row_major\n"
			"	float4x4	worldViewProjection;\n"
			"	row_major\n"
			"	float4x4	world;\n"
			"	float4		lightDirection;\n"
			"	float4		lightColor;\n"
			"	float4		materialColor;\n"
			"};\n"
			"VS_OUT VSMain( VS_IN vin )\n"
			"{\n"
			"	vin.normal.w	= 0;\n"
			"	float4 norm		= normalize( mul( vin.normal, world ) );\n"
			"	float4 light	= normalize( -lightDirection );\n"
			"	float  NL		= saturate( dot( light, norm ) );\n"
			"	NL				= NL * 0.5f + 0.5f;\n"
			"	VS_OUT vout;\n"
			"	vout.pos		= mul( vin.pos, worldViewProjection );\n"
			"	vout.color		= materialColor * NL;\n"
			"	vout.color.a	= materialColor.a;\n"
			"	vout.texCoord	= vin.texCoord * vin.texCoordTransform.zw + vin.texCoordTransform.xy;\n"
			"	return vout;\n"
			"}\n"
			"float4 PSMain( VS_OUT pin ) : SV_TARGET\n"
			"{\n"
			"	if ( pin.color.a <= 0 ) { discard; }\n"
			"	float4 sampleColor = diffuseMap.Sample( diffuseMapSampler, pin.texCoord );\n"
			"	return sampleColor * pin.color * float4( lightColor.rgb * lightColor.w, 1.0f );\n"
			"}\n"
			;
		}
		constexpr const char	*TextureBoardShaderNameVS()
		{
			return "TextureBoardVS";
		}
		constexpr const char	*TextureBoardShaderNamePS()
		{
			return "TextureBoardPS";
		}
		constexpr const char	*TextureBoardShaderEntryPointVS()
		{
			return "VSMain";
		}
		constexpr const char	*TextureBoardShaderEntryPointPS()
		{
			return "PSMain";
		}

		D3D11_SAMPLER_DESC		TextureBoardSamplerDesc()
		{
			D3D11_SAMPLER_DESC standard{};
			/*
			standard.MipLODBias		= 0;
			standard.MaxAnisotropy	= 16;
			*/
			standard.Filter			= D3D11_FILTER_MIN_MAG_MIP_POINT;
			standard.AddressU		= D3D11_TEXTURE_ADDRESS_BORDER;
			standard.AddressV		= D3D11_TEXTURE_ADDRESS_BORDER;
			standard.AddressW		= D3D11_TEXTURE_ADDRESS_BORDER;
			standard.ComparisonFunc	= D3D11_COMPARISON_NEVER;
			standard.MinLOD			= 0;
			standard.MaxLOD			= D3D11_FLOAT32_MAX;

			constexpr DirectX::XMFLOAT4 border{ 0.0f, 0.0f, 0.0f, 0.0f };
			standard.BorderColor[0] = border.x;
			standard.BorderColor[1] = border.y;
			standard.BorderColor[2] = border.z;
			standard.BorderColor[3] = border.w;

			return standard;
		}

		TextureBoard::TextureBoard( std::wstring filePath ) : Base(),
			FILE_PATH( filePath ),
			vertices(), textureDesc(), iSRV(), iSampler()
		{}
		TextureBoard::~TextureBoard() = default;

		/// <summary>
		/// The vertices is place at [-0.5f ~ +0.5f], the center is 0.0f.
		/// </summary>
		std::array<TextureBoard::Vertex, 8> MakeBoard()
		{
			constexpr unsigned int VERTEX_COUNT = 8;

			constexpr float OFFSET	= 0.5f;
			constexpr float DEPTH	= 0.0f;

			constexpr std::array<XMFLOAT3, VERTEX_COUNT> POSITIONS // Front:LT,RT,LB,RB, Back:LB,RB,LT,RT.
			{
				// Front
				XMFLOAT3{ -OFFSET, +OFFSET, DEPTH },
				XMFLOAT3{ +OFFSET, +OFFSET, DEPTH },
				XMFLOAT3{ -OFFSET, -OFFSET, DEPTH },
				XMFLOAT3{ +OFFSET, -OFFSET, DEPTH },
				// Back
				XMFLOAT3{ -OFFSET, -OFFSET, DEPTH },
				XMFLOAT3{ +OFFSET, -OFFSET, DEPTH },
				XMFLOAT3{ -OFFSET, +OFFSET, DEPTH },
				XMFLOAT3{ +OFFSET, +OFFSET, DEPTH }
			};
			constexpr std::array<XMFLOAT3, VERTEX_COUNT> NORMALS
			{
				// Front
				XMFLOAT3{ 0.0f, 0.0f, 1.0f },
				XMFLOAT3{ 0.0f, 0.0f, 1.0f },
				XMFLOAT3{ 0.0f, 0.0f, 1.0f },
				XMFLOAT3{ 0.0f, 0.0f, 1.0f },
				// Back
				XMFLOAT3{ 0.0f, 0.0f, -1.0f },
				XMFLOAT3{ 0.0f, 0.0f, -1.0f },
				XMFLOAT3{ 0.0f, 0.0f, -1.0f },
				XMFLOAT3{ 0.0f, 0.0f, -1.0f }
			};
			constexpr std::array<XMFLOAT2, VERTEX_COUNT> TEX_COORDS // Front:LT,RT,LB,RB, Back:LB,RB,LT,RT.
			{
				// Front
				XMFLOAT2{ 0.0f, 0.0f },
				XMFLOAT2{ 1.0f, 0.0f },
				XMFLOAT2{ 0.0f, 1.0f },
				XMFLOAT2{ 1.0f, 1.0f },
				// Back
				XMFLOAT2{ 0.0f, 1.0f },
				XMFLOAT2{ 1.0f, 1.0f },
				XMFLOAT2{ 0.0f, 0.0f },
				XMFLOAT2{ 1.0f, 0.0f }
			};

			std::array<TextureBoard::Vertex, VERTEX_COUNT> vertices{};
			for ( size_t i = 0; i < VERTEX_COUNT; ++i )
			{
				vertices[i].pos			= POSITIONS[i];
				vertices[i].normal		= NORMALS[i];
				vertices[i].texCoord	= TEX_COORDS[i];
			}

			return vertices;
		}

		void TextureBoard::Init()
		{
			if ( wasCreated ) { return; }
			// else

			HRESULT hr = S_OK;
			ID3D11Device *pDevice = Donya::GetDevice();

			vertices = MakeBoard();

			// Create VertexBuffer
			{
				D3D11_BUFFER_DESC bufferDesc{};
				bufferDesc.ByteWidth			= sizeof( TextureBoard::Vertex ) * vertices.size();
				bufferDesc.Usage				= D3D11_USAGE_DYNAMIC;
				bufferDesc.BindFlags			= D3D11_BIND_VERTEX_BUFFER;
				bufferDesc.CPUAccessFlags		= D3D11_CPU_ACCESS_WRITE;
				bufferDesc.MiscFlags			= 0;
				bufferDesc.StructureByteStride	= 0;

				D3D11_SUBRESOURCE_DATA subResource{};
				subResource.pSysMem				= vertices.data();
				subResource.SysMemPitch			= 0; // Not use for vertex buffers.
				subResource.SysMemSlicePitch	= 0; // Not use for vertex buffers.

				hr = pDevice->CreateBuffer
				(
					&bufferDesc,
					&subResource,
					iVertexBuffer.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Vertex-Buffer." );
			}

			// IndexBuffer is not use.

			// Create ConstantBuffer
			{
				hr = CreateConstantBuffer
				(
					pDevice,
					sizeof( TextureBoard::ConstantBuffer ),
					iConstantBuffer.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Constant-Buffer." );
			}
			// Create VertexShader and InputLayout
			{
				constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 4> INPUT_ELEMENT_DESCS
				{
					D3D11_INPUT_ELEMENT_DESC{ "POSITION",			0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
					D3D11_INPUT_ELEMENT_DESC{ "NORMAL",				0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
					D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD",			0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
					D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD_TRANSFORM",	0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
				};

				Resource::CreateVertexShaderFromSource
				(
					pDevice,
					TextureBoardShaderNameVS(),
					TextureBoardShaderSourceCode(),
					TextureBoardShaderEntryPointVS(),
					iVertexShader.ReleaseAndGetAddressOf(),
					iInputLayout.ReleaseAndGetAddressOf(),
					INPUT_ELEMENT_DESCS.data(),
					INPUT_ELEMENT_DESCS.size()
				);
			}
			// Create PixelShader
			{
				Resource::CreatePixelShaderFromSource
				(
					pDevice,
					TextureBoardShaderNamePS(),
					TextureBoardShaderSourceCode(),
					TextureBoardShaderEntryPointPS(),
					iPixelShader.ReleaseAndGetAddressOf()
				);
			}
			// Create Rasterizer States
			{
				D3D11_RASTERIZER_DESC rdWireFrame	= GeometryRasterizerDesc( D3D11_FILL_WIREFRAME );
				D3D11_RASTERIZER_DESC rdSurface		= GeometryRasterizerDesc( D3D11_FILL_SOLID );

				hr = pDevice->CreateRasterizerState
				(
					&rdWireFrame,
					iRasterizerStateWire.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Rasterizer-State of WireFrame." );

				hr = pDevice->CreateRasterizerState
				(
					&rdSurface,
					iRasterizerStateSurface.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : Create Rasterizer-State of Surface." );
			}
			// Create DepthsStencilState
			{
				D3D11_DEPTH_STENCIL_DESC desc = GeometryDepthStencilDesc();

				hr = pDevice->CreateDepthStencilState
				(
					&desc,
					iDepthStencilState.GetAddressOf()
				);
				_ASSERT_EXPR( SUCCEEDED( hr ), "Failed : Create Depth-Stencil-State." );
			}
			// Load Texture
			{
				D3D11_SAMPLER_DESC desc = TextureBoardSamplerDesc();

				Resource::CreateSamplerState
				(
					pDevice,
					&iSampler, // required pointer of ComPtr.
					desc
				);

				Resource::CreateTexture2DFromFile
				(
					pDevice,
					FILE_PATH,
					iSRV.GetAddressOf(),
					&textureDesc
				);
			}

			wasCreated = true;
		}
		void TextureBoard::Uninit()
		{
			// NOP.
		}

		Donya::Vector4x4 TextureBoard::CalcBillboardRotation( const Donya::Vector3 &lookDirection, float zRadian, Donya::Quaternion::Freeze freezeDirection ) const
		{
			constexpr	Donya::Quaternion FRONT			= Donya::Quaternion::Identity(); // I regard the initial pose is front(Z plus).
			const		Donya::Quaternion ROLL			= Donya::Quaternion::Make( Donya::Vector3::Front(), zRadian );

			Donya::Quaternion lookRotation = FRONT.LookAt( lookDirection, freezeDirection, /* returnsRotatedQuaternion = */ false );
			lookRotation.RotateBy( ROLL );

			return lookRotation.RequireRotationMatrix();
		}

		Donya::Vector2 GetTextureSize( const D3D11_TEXTURE2D_DESC &textureDesc )
		{
			return Donya::Vector2
			{ 
				scast<float>( textureDesc.Width  ),
				scast<float>( textureDesc.Height )
			};
		}
		void TextureBoard::Render( ID3D11DeviceContext *pImmediateContext, bool useDefaultShading, bool isEnableFill, const XMFLOAT4X4 &defMatWVP, const XMFLOAT4X4 &defMatW, const XMFLOAT4 &defLightDir, const XMFLOAT4 &defMtlColor ) const
		{
			const Donya::Vector2	ltCoord{ 0.0f, 0.0f };
			RenderPart( ltCoord, GetTextureSize( textureDesc ), pImmediateContext, useDefaultShading, isEnableFill, defMatWVP, defMatW, defLightDir, defMtlColor );
		}
		void TextureBoard::RenderPart( const Donya::Vector2 &texPartPosLT, const Donya::Vector2 &texPartWholeSize, ID3D11DeviceContext *pImmediateContext, bool useDefaultShading, bool isEnableFill, const XMFLOAT4X4 &defMatWVP, const XMFLOAT4X4 &defMatW, const XMFLOAT4 &defLightDir, const XMFLOAT4 &defMtlColor ) const
		{
			if ( !wasCreated )
			{
				_ASSERT_EXPR( 0, L"Error : The texture-board was not created!" );
				return;
			}
			// else

			constexpr unsigned int VERTEX_COUNT = 4 * 2; // Front-face and back-face.

			HRESULT hr = S_OK;
			
			// Use default context.
			if ( !pImmediateContext )
			{
				pImmediateContext = Donya::GetImmediateContext();
			}

			// Mapping
			{
				const Donya::Vector2 wholeSize = GetTextureSize( textureDesc );
				auto CalcUpdatedVertex = [&wholeSize, &texPartPosLT, &texPartWholeSize]( TextureBoard::Vertex *pVertex )
				{
					pVertex->texCoordTransform.x = texPartPosLT.x		/ wholeSize.x;
					pVertex->texCoordTransform.y = texPartPosLT.y		/ wholeSize.y;

					pVertex->texCoordTransform.z = texPartWholeSize.x	/ wholeSize.x;
					pVertex->texCoordTransform.w = texPartWholeSize.y	/ wholeSize.y;
				};
				for ( auto &it : vertices )
				{
					CalcUpdatedVertex( &it );
				}
				
				D3D11_MAPPED_SUBRESOURCE mappedSubresource{};

				hr = pImmediateContext->Map( iVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource );
				if ( FAILED( hr ) )
				{
					_ASSERT_EXPR( 0, L"Failed : Map at TextureBoard." );
					return;
				}
				// else

				memcpy_s( mappedSubresource.pData, sizeof( TextureBoard::Vertex ) * VERTEX_COUNT, vertices.data(), mappedSubresource.RowPitch );

				pImmediateContext->Unmap( iVertexBuffer.Get(), 0 );
			}

			if ( useDefaultShading )
			{
				ConstantBuffer cb{};
				cb.worldViewProjection	= defMatWVP;
				cb.world				= defMatW;
				cb.lightDirection		= defLightDir;
				cb.lightColor			= { 1.0f, 1.0f, 1.0f, 1.0f };
				cb.materialColor		= defMtlColor;
				cb.materialColor.w		= Donya::Color::FilteringAlpha( cb.materialColor.w );
				
				pImmediateContext->UpdateSubresource( iConstantBuffer.Get(), 0, nullptr, &cb, 0, 0 );
				pImmediateContext->VSSetConstantBuffers( 0, 1, iConstantBuffer.GetAddressOf() );
				pImmediateContext->PSSetConstantBuffers( 0, 1, iConstantBuffer.GetAddressOf() );
			}

			// Settings
			{
				UINT stride = sizeof( TextureBoard::Vertex );
				UINT offset = 0;
				pImmediateContext->IASetVertexBuffers( 0, 1, iVertexBuffer.GetAddressOf(), &stride, &offset );
				pImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

				// Reset
				pImmediateContext->IASetIndexBuffer( nullptr, DXGI_FORMAT_R32_UINT, NULL );

				if ( useDefaultShading )
				{
					pImmediateContext->IASetInputLayout( iInputLayout.Get() );
					pImmediateContext->VSSetShader( iVertexShader.Get(), nullptr, 0 );
				}

				ID3D11RasterizerState *ppRasterizerState =
				( isEnableFill )
				? iRasterizerStateSurface.Get()
				: iRasterizerStateWire.Get();
				pImmediateContext->RSSetState( ppRasterizerState );

				if ( useDefaultShading )
				{
					pImmediateContext->PSSetShader( iPixelShader.Get(), nullptr, 0 );
				}
				pImmediateContext->PSSetShaderResources( 0, 1, iSRV.GetAddressOf() );
				pImmediateContext->PSSetSamplers( 0, 1, iSampler.GetAddressOf() );

				pImmediateContext->OMSetDepthStencilState( iDepthStencilState.Get(), 0xffffffff );
			}

			pImmediateContext->Draw( VERTEX_COUNT, 0 );
		}

	// region TextureBoard
	#pragma endregion

	#pragma region Creater

		Cube			CreateCube()
		{
			Cube instance{};
			instance.Init();
			return instance;
		}
		Sphere			CreateSphere( size_t hSlice, size_t vSlice )
		{
			Sphere instance{ hSlice, vSlice };
			instance.Init();
			return instance;
		}
		TextureBoard	CreateTextureBoard( std::wstring textureFilePath )
		{
			TextureBoard instance{ textureFilePath };
			instance.Init();
			return instance;
		}

	// region Creater
	#pragma endregion

	}
}
