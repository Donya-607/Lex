#include "SkinnedMesh.h"

#include "Donya/Direct3DUtil.h"
#include "Donya/Donya.h"
#include "Donya/Resource.h"
#include "Donya/Useful.h"

#include "Common.h"
#include "Loader.h"

#if DEBUG_MODE

#include "Keyboard.h"

#endif // DEBUG_MODE

using namespace DirectX;

namespace Donya
{
	bool SkinnedMesh::Create( const Loader *loader, SkinnedMesh *pOutput )
	{
		if ( !loader || !pOutput ) { return false; }
		// else

		const std::vector<Loader::Mesh> *pLoadedMeshes = loader->GetMeshes();
		const size_t loadedMeshCount = pLoadedMeshes->size();

		std::vector<std::vector<size_t>> argIndices{};
		std::vector<std::vector<Vertex>> argVertices{};

		std::vector<SkinnedMesh::Mesh> meshes{};
		meshes.resize( loadedMeshCount );
		for ( size_t i = 0; i < loadedMeshCount; ++i )
		{
			auto &loadedMesh = ( *pLoadedMeshes )[i];

			meshes[i].coordinateConversion	= loadedMesh.coordinateConversion;
			meshes[i].globalTransform		= loadedMesh.globalTransform;

			std::vector<Vertex> vertices{};
			{
				const std::vector<Donya::Vector3> &normals   = loadedMesh.normals;
				const std::vector<Donya::Vector3> &positions = loadedMesh.positions;
				const std::vector<Donya::Vector2> &texCoords = loadedMesh.texCoords;
				const std::vector<Loader::BoneInfluencesPerControlPoint> &boneInfluences = loadedMesh.influences;

				vertices.resize( std::max( normals.size(), positions.size() ) );
				size_t end = vertices.size();
				for ( size_t j = 0; j < end; ++j )
				{
					vertices[j].normal		= normals[j];
					vertices[j].pos			= positions[j];
					
					vertices[j].texCoord	= ( j < texCoords.size() )
											? texCoords[j]
											: Donya::Vector2{};

					size_t influenceCount = boneInfluences[j].cluster.size();
					for ( size_t k = 0; k < influenceCount; ++k )
					{
						vertices[j].boneIndices[k] = boneInfluences[j].cluster[k].index;
						vertices[j].boneWeights[k] = boneInfluences[j].cluster[k].weight;
					}
				}
			}
			argVertices.emplace_back( vertices );
			argIndices.emplace_back( loadedMesh.indices );
			
			size_t subsetCount = loadedMesh.subsets.size();
			meshes[i].subsets.resize( subsetCount );
			for ( size_t j = 0; j < subsetCount; ++j )
			{
				auto &loadedSubset		= loadedMesh.subsets[j];
				auto &mySubset			= meshes[i].subsets[j];

				mySubset.indexStart		= loadedSubset.indexStart;
				mySubset.indexCount		= loadedSubset.indexCount;
				mySubset.transparency	= loadedSubset.transparency;

				const std::string fileDirectory = loader->GetFileDirectory();

				auto FetchMaterialContain =
				[&fileDirectory]( SkinnedMesh::Material *meshMtl, const Loader::Material &loadedMtl )
				{
					meshMtl->color.x = loadedMtl.color.x;
					meshMtl->color.y = loadedMtl.color.y;
					meshMtl->color.z = loadedMtl.color.z;
					meshMtl->color.w = 1.0f;

					size_t texCount = loadedMtl.relativeTexturePaths.size();
					meshMtl->textures.resize( texCount );
					for ( size_t i = 0; i < texCount; ++i )
					{
						meshMtl->textures[i].fileName = fileDirectory + loadedMtl.relativeTexturePaths[i];
					}
				};

				FetchMaterialContain( &mySubset.ambient,	loadedSubset.ambient	);
				FetchMaterialContain( &mySubset.bump,		loadedSubset.bump		);
				FetchMaterialContain( &mySubset.diffuse,	loadedSubset.diffuse	);
				FetchMaterialContain( &mySubset.emissive,	loadedSubset.emissive	);
				FetchMaterialContain( &mySubset.specular,	loadedSubset.specular	);
				mySubset.specular.color.w = loadedSubset.specular.color.w;
			}
		}

		bool   createResult = pOutput->Init( argIndices, argVertices, meshes );
		return createResult;
	}

	SkinnedMesh::SkinnedMesh() :
		wasCreated( false ),
		meshes(),
		iConstantBuffer(), iMaterialCBuffer(),
		iInputLayout(), iVertexShader(), iPixelShader(),
		iRasterizerStateWire(), iRasterizerStateSurface(), iDepthStencilState()
	{

	}
	SkinnedMesh::~SkinnedMesh()
	{
		meshes.clear();
		meshes.shrink_to_fit();
	}

	bool SkinnedMesh::Init( const std::vector<std::vector<size_t>> &allIndices, const std::vector<std::vector<Vertex>> &allVertices, const std::vector<Mesh> &loadedMeshes )
	{
		if ( wasCreated      ) { return false; }
		if ( !meshes.empty() ) { return false; }
		// else

		HRESULT hr = S_OK;
		ID3D11Device *pDevice = Donya::GetDevice();

		meshes = loadedMeshes;
		const size_t meshCount = meshes.size();

		// Create VertexBuffers
		for ( size_t i = 0; i < meshCount; ++i )
		{
			hr = CreateVertexBuffer<Vertex>
			(
				pDevice,
				allVertices[i],
				meshes[i].iVertexBuffer.GetAddressOf()
			);
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create Vertex-Buffer" );
				return false;
			}
		}
		// Create IndexBuffers
		for ( size_t i = 0; i < meshCount; ++i )
		{
			hr = CreateIndexBuffer
			(
				pDevice,
				allIndices[i],
				meshes[i].iIndexBuffer.GetAddressOf()
			);
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create Vertex-Buffer" );
				return false;
			}
		}
		// Create ConstantBuffers
		{
			hr = CreateConstantBuffer
			(
				pDevice,
				sizeof( ConstantBuffer ),
				iConstantBuffer.GetAddressOf()
			);
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create Constant-Buffer" );
				return false;
			}
			// else

			hr = CreateConstantBuffer
			(
				pDevice,
				sizeof( MaterialConstantBuffer ),
				iMaterialCBuffer.GetAddressOf()
			);
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : Create Constant-Buffer" );
				return false;
			}
			// else
		}
		// Create VertexShader and InputLayout
		{
			D3D11_INPUT_ELEMENT_DESC d3d11InputElementsDesc[] =
			{
				{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL"		, 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD"	, 0, DXGI_FORMAT_R32G32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "BONES"		, 0, DXGI_FORMAT_R32G32B32A32_UINT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "WEIGHTS"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			bool succeeded = Resource::CreateVertexShaderFromCso
			(
				pDevice,
				"./Data/Shader/SkinnedMeshVS.cso", "rb",
				iVertexShader.GetAddressOf(),
				iInputLayout.GetAddressOf(),
				d3d11InputElementsDesc,
				_countof( d3d11InputElementsDesc ),
				/* enableCache = */ true
			);
			if ( !succeeded )
			{
				_ASSERT_EXPR( 0, L"Failed : Create vertex-shader from cso file." );
				return false;
			}
		}
		// Create PixelShader
		{
			bool succeeded = Resource::CreatePixelShaderFromCso
			(
				pDevice,
				"./Data/Shader/SkinnedMeshPS.cso", "rb",
				iPixelShader.GetAddressOf(),
				/* enableCache = */ false
			);
			if ( !succeeded )
			{
				_ASSERT_EXPR( 0, L"Failed : Create pixel-shader from cso file." );
				return false;
			}
		}
		// Create Rasterizer States
		{
			D3D11_RASTERIZER_DESC d3d11ResterizerDescBase{};
			d3d11ResterizerDescBase.CullMode					= D3D11_CULL_FRONT;
			d3d11ResterizerDescBase.FrontCounterClockwise		= FALSE;
			d3d11ResterizerDescBase.DepthBias					= 0;
			d3d11ResterizerDescBase.DepthBiasClamp				= 0;
			d3d11ResterizerDescBase.SlopeScaledDepthBias		= 0;
			d3d11ResterizerDescBase.DepthClipEnable				= TRUE;
			d3d11ResterizerDescBase.ScissorEnable				= FALSE;
			d3d11ResterizerDescBase.MultisampleEnable			= FALSE;
			d3d11ResterizerDescBase.AntialiasedLineEnable		= TRUE;

			D3D11_RASTERIZER_DESC d3d11ResterizerWireDesc		= d3d11ResterizerDescBase;
			D3D11_RASTERIZER_DESC d3d11ResterizerSurfaceDesc	= d3d11ResterizerDescBase;
			d3d11ResterizerWireDesc.FillMode					= D3D11_FILL_WIREFRAME;
			d3d11ResterizerSurfaceDesc.FillMode					= D3D11_FILL_SOLID;
			d3d11ResterizerSurfaceDesc.AntialiasedLineEnable	= FALSE;

			hr = pDevice->CreateRasterizerState( &d3d11ResterizerWireDesc, iRasterizerStateWire.GetAddressOf() );
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : CreateRasterizerState()" );
				return false;
			}
			// else

			hr = pDevice->CreateRasterizerState( &d3d11ResterizerSurfaceDesc, iRasterizerStateSurface.GetAddressOf() );
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : CreateRasterizerState()" );
				return false;
			}
			// else
		}
		// Create DepthsStencilState
		{
			D3D11_DEPTH_STENCIL_DESC d3dDepthStencilDesc{};
			d3dDepthStencilDesc.DepthEnable		= TRUE;
			d3dDepthStencilDesc.DepthWriteMask	= D3D11_DEPTH_WRITE_MASK_ALL;
			d3dDepthStencilDesc.DepthFunc		= D3D11_COMPARISON_LESS;
			d3dDepthStencilDesc.StencilEnable	= false;

			hr = pDevice->CreateDepthStencilState
			(
				&d3dDepthStencilDesc,
				iDepthStencilState.GetAddressOf()
			);
			if ( FAILED( hr ) )
			{
				_ASSERT_EXPR( 0, L"Failed : CreateDepthStencilState()" );
				return false;
			}
		}
		// Create Texture
		{
			D3D11_SAMPLER_DESC samplerDesc{};
			samplerDesc.Filter			= D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			samplerDesc.AddressU		= D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.AddressV		= D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.AddressW		= D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.ComparisonFunc	= D3D11_COMPARISON_ALWAYS;
			samplerDesc.MinLOD			= 0;
			samplerDesc.MaxLOD			= D3D11_FLOAT32_MAX;

			auto CreateSamplerAndTextures = [&]( SkinnedMesh::Material *pMtl )
			{
				size_t textureCount = pMtl->textures.size();
				if ( !textureCount )
				{
					pMtl->iSampler = Resource::RequireInvalidSamplerStateComPtr();

					SkinnedMesh::Material::Texture dummy{};
					Resource::CreateUnicolorTexture
					(
						pDevice,
						dummy.iSRV.GetAddressOf(),
						&dummy.texture2DDesc
					);

					pMtl->textures.push_back( dummy );

					return;	// Escape from lambda-expression.
				}
				// else

				Resource::CreateSamplerState
				(
					pDevice,
					&pMtl->iSampler,
					samplerDesc,
					/* enableCache = */ false
				);
				for ( size_t i = 0; i < textureCount; ++i )
				{
					auto &tex = pMtl->textures[i];
					bool succeeded = Resource::CreateTexture2DFromFile
					(
						pDevice,
						Donya::MultiToWide( tex.fileName ),
						tex.iSRV.GetAddressOf(),
						&tex.texture2DDesc,
						/* enableCache = */ false
					);
					if ( !succeeded )
					{
						_ASSERT_EXPR( 0, L"Failed : Create texture from file." );
					}
				}
			};

			size_t meshCount = meshes.size();
			for ( size_t i = 0; i < meshCount; ++i )
			{
				size_t subsetCount = meshes[i].subsets.size();
				for ( size_t j = 0; j < subsetCount; ++j )
				{
					CreateSamplerAndTextures( &meshes[i].subsets[j].ambient );
					CreateSamplerAndTextures( &meshes[i].subsets[j].bump );
					CreateSamplerAndTextures( &meshes[i].subsets[j].diffuse );
					CreateSamplerAndTextures( &meshes[i].subsets[j].emissive );
					CreateSamplerAndTextures( &meshes[i].subsets[j].specular );
				}
			}
		}

		wasCreated = true;
		return true;
	}

	void SkinnedMesh::Render( const DirectX::XMFLOAT4X4 &worldViewProjection, const DirectX::XMFLOAT4X4 &world, const DirectX::XMFLOAT4 &eyePosition, const DirectX::XMFLOAT4 &materialColor, const DirectX::XMFLOAT4 &lightColor, const DirectX::XMFLOAT4 &lightDirection, bool isEnableFill )
	{
		if ( !wasCreated )
		{
			_ASSERT_EXPR( 0, L"Error : The mesh was not created !" );
			return;
		}
		if ( meshes.empty() ) { return; }
		// else

		{
			DirectX::XMFLOAT4X4 identity{}; DirectX::XMStoreFloat4x4( &identity, DirectX::XMMatrixIdentity() );
			identity._11 = -1.0f;
			static DirectX::XMFLOAT4X4 coordConversion = identity;	// I'm not want to initialize to identity every frame.

		#if USE_IMGUI
			if ( ImGui::BeginIfAllowed( "SkinnedMesh" ) )
			// if ( ImGui::BeginIfAllowed() )
			{
				if ( ImGui::TreeNode( "CoordinateConversion" ) )
				{
					ImGui::SliderFloat4( "11, 12, 13, 14", &coordConversion._11, -1.0f, 1.0f );
					ImGui::SliderFloat4( "21, 22, 23, 24", &coordConversion._21, -1.0f, 1.0f );
					ImGui::SliderFloat4( "31, 32, 33, 34", &coordConversion._31, -1.0f, 1.0f );
					ImGui::SliderFloat4( "41, 42, 43, 44", &coordConversion._41, -1.0f, 1.0f );

					ImGui::TreePop();
				}

				ImGui::End();
			}
		#endif // USE_IMGUI

			for ( auto &it : meshes )
			{
				it.coordinateConversion = coordConversion;
			}
		}

		ID3D11DeviceContext *pImmediateContext = Donya::GetImmediateContext();

		Microsoft::WRL::ComPtr<ID3D11RasterizerState>	prevRasterizerState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>		prevSamplerState;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	prevDepthStencilState;
		{
			pImmediateContext->RSGetState( prevRasterizerState.ReleaseAndGetAddressOf() );
			pImmediateContext->PSGetSamplers( 0, 1, prevSamplerState.ReleaseAndGetAddressOf() );
			pImmediateContext->OMGetDepthStencilState( prevDepthStencilState.ReleaseAndGetAddressOf(), 0 );
		}

		// Commmon Settings
		{
			/*
			IA...InputAssembler
			VS...VertexShader
			HS...HullShader
			DS...DomainShader
			GS...GeometryShader
			SO...StreamOutput
			RS...RasterizerState
			PS...PixelShader
			OM...OutputMerger

			CS...ComputeShader
			*/

			pImmediateContext->IASetInputLayout( iInputLayout.Get() );
			pImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
			
			pImmediateContext->VSSetShader( iVertexShader.Get(), nullptr, 0 );

			ID3D11RasterizerState	*ppRasterizerState
									= ( isEnableFill )
									? iRasterizerStateSurface.Get()
									: iRasterizerStateWire.Get();
			pImmediateContext->RSSetState( ppRasterizerState );

			pImmediateContext->PSSetShader( iPixelShader.Get(), nullptr, 0 );

			pImmediateContext->OMSetDepthStencilState( iDepthStencilState.Get(), 0xffffffff );
		}

		for ( auto &mesh : meshes )
		{
			// Update Constant Buffer
			{
				auto Mul4x4 = []( const DirectX::XMFLOAT4X4 &lhs, const DirectX::XMFLOAT4X4 &rhs )
				->DirectX::XMFLOAT4X4
				{
					DirectX::XMFLOAT4X4 rv{};
					DirectX::XMStoreFloat4x4
					(
						&rv,
						DirectX::XMLoadFloat4x4( &lhs ) * DirectX::XMLoadFloat4x4( &rhs )
					);
					return rv;
				};

				ConstantBuffer cb{};
				cb.worldViewProjection	= Mul4x4( Mul4x4( mesh.globalTransform, mesh.coordinateConversion ), worldViewProjection );
				cb.world				= Mul4x4( Mul4x4( mesh.globalTransform, mesh.coordinateConversion ), world );
				// TODO:Attach a bone matrix here.
				for ( auto &it : cb.boneTransforms )
				{
					it = Donya::Vector4x4::Identity().XMFloat();
				}
				cb.lightColor			= lightColor;
				cb.lightDir				= lightDirection;
				cb.materialColor		= materialColor;
				// cb.eyePosition			= eyePosition;
				pImmediateContext->UpdateSubresource( iConstantBuffer.Get(), 0, nullptr, &cb, 0, 0 );
			}
			pImmediateContext->VSSetConstantBuffers( 0, 1, iConstantBuffer.GetAddressOf() );
			pImmediateContext->PSSetConstantBuffers( 0, 1, iConstantBuffer.GetAddressOf() );

			UINT stride = sizeof( Vertex );
			UINT offset = 0;
			pImmediateContext->IASetVertexBuffers( 0, 1, mesh.iVertexBuffer.GetAddressOf(), &stride, &offset );
			pImmediateContext->IASetIndexBuffer( mesh.iIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );

			for ( auto &subset : mesh.subsets )
			{
				// Update Material-Constant Buffer
				{
					MaterialConstantBuffer mtlCB{};
					mtlCB.ambient	= subset.ambient.color;
					mtlCB.bump		= subset.bump.color;
					mtlCB.diffuse	= subset.diffuse.color;
					mtlCB.emissive	= subset.emissive.color;
					mtlCB.specular	= subset.specular.color;

					pImmediateContext->UpdateSubresource( iMaterialCBuffer.Get(), 0, nullptr, &mtlCB, 0, 0 );
				}
				pImmediateContext->VSSetConstantBuffers( 1, 1, iMaterialCBuffer.GetAddressOf() );
				pImmediateContext->PSSetConstantBuffers( 1, 1, iMaterialCBuffer.GetAddressOf() );

				// TODO:diffuseˆÈŠO‚Ì‚à‚Ì‚à“K—p‚·‚é

				pImmediateContext->PSSetSamplers( 0, 1, subset.diffuse.iSampler.GetAddressOf() );

				for ( auto &texture : subset.diffuse.textures )
				{
					pImmediateContext->PSSetShaderResources( 0, 1, texture.iSRV.GetAddressOf() );
					
					pImmediateContext->DrawIndexed( subset.indexCount, subset.indexStart, 0 );
				}
			}
		}

		// PostProcessing
		{
			ID3D11ShaderResourceView *pNullSRV = nullptr;

			pImmediateContext->IASetInputLayout( 0 );

			pImmediateContext->VSSetShader( 0, 0, 0 );

			pImmediateContext->RSSetState( prevRasterizerState.Get() );

			pImmediateContext->PSSetShader( 0, 0, 0 );
			pImmediateContext->PSSetShaderResources( 0, 1, &pNullSRV );
			pImmediateContext->PSSetSamplers( 0, 1, prevSamplerState.GetAddressOf() );

			pImmediateContext->OMSetDepthStencilState( prevDepthStencilState.Get(), 1 );
		}
	}

}