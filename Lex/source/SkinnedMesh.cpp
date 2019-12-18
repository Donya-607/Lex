#include "SkinnedMesh.h"

#include <algorithm>

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
	bool SkinnedMesh::Create( const Loader &loader, SkinnedMesh *pOutput, ID3D11Device *pDevice )
	{
		if ( !pOutput ) { return false; }
		// else

		const std::vector<Loader::Mesh> *pLoadedMeshes = loader.GetMeshes();
		const size_t loadedMeshCount = pLoadedMeshes->size();

		std::vector<std::vector<size_t>> argIndices{};
		std::vector<std::vector<Vertex>> argVertices{};
		
		auto AssignBoneInfluences = []( Donya::SkinnedMesh::Vertex *pVertex, const Donya::Loader::BoneInfluencesPerControlPoint &influences )
		{
			/*
			0,	Clear the current bone indices and weights.
			1,	Store all influences data to temporary storage.
			2,	Sort with weight the storage by descending order.
			3,	Assign the higher data of storage to vertex as many as MAX_BONE_INFLUENCES(bone array size).
			4,	Assign the remaining storage data to highest weight bone.
			*/

			const size_t influenceCount = influences.cluster.size();

			// No.0
			pVertex->boneIndices.fill( 0 );
			pVertex->boneWeights.fill( 0 );
			pVertex->boneWeights[0] = 1.0f;

			// No.1
			std::vector<Donya::Loader::BoneInfluence> storage{ influenceCount };
			for ( size_t i = 0; i < influenceCount; ++i )
			{
				storage[i].index  = influences.cluster[i].index;
				storage[i].weight = influences.cluster[i].weight;
			}

			// No.2
			auto Compare = []( const Donya::Loader::BoneInfluence &lhs, const Donya::Loader::BoneInfluence &rhs )
			{
				return ( lhs.weight < rhs.weight ) ? true : false;
			};
			std::sort( storage.begin(), storage.end(), Compare );

			// No.3
			size_t  loopIndex = 0;
			for ( ; loopIndex < MAX_BONE_INFLUENCES; ++loopIndex )
			{
				if ( influenceCount <= loopIndex ) { continue; }
				// else

				pVertex->boneIndices[loopIndex] = storage[loopIndex].index;
				pVertex->boneWeights[loopIndex] = storage[loopIndex].weight;
			}

			// No.4
			size_t highestBoneIndex{};
			{
				float highestWeight = 0.0f;
				for ( size_t i = 0; i < MAX_BONE_INFLUENCES; ++i )
				{
					float selectWeight = pVertex->boneWeights[i];
					if ( highestWeight < selectWeight )
					{
						highestBoneIndex	= i;
						highestWeight		= selectWeight;
					}
				}
			}
			for ( ; loopIndex < influenceCount; ++loopIndex )
			{
				pVertex->boneWeights[highestBoneIndex] += storage[loopIndex].weight;
			}
		};
		auto AssignVertices = [&AssignBoneInfluences]( std::vector<Vertex> *pVertices, const Donya::Loader::Mesh &loadedMesh )
		{
			const std::vector<Donya::Vector3> &normals   = loadedMesh.normals;
			const std::vector<Donya::Vector3> &positions = loadedMesh.positions;
			const std::vector<Donya::Vector2> &texCoords = loadedMesh.texCoords;
			const std::vector<Loader::BoneInfluencesPerControlPoint> &boneInfluences = loadedMesh.influences;

			const size_t positionCount	= positions.size();
			const size_t normalCount	= normals.size();
			const size_t texCoordCount	= texCoords.size();

			const size_t vertexCount	= positionCount;
			pVertices->resize( vertexCount );
			for ( size_t i = 0; i < vertexCount; ++i )
			{
				auto &vertex = ( *pVertices )[i];

				vertex.pos		= positions[i];

				vertex.normal	= ( i < normalCount )
				? ( positions[i] - Donya::Vector3::Zero() ).Normalized()	// Assign approximate normal.
				: normals[i];

				vertex.texCoord	= ( i < texCoordCount )
				? texCoords[i]
				: Donya::Vector2::Zero();

				AssignBoneInfluences( &vertex, boneInfluences[i] );
			}
		};

		std::vector<SkinnedMesh::Mesh> meshes{};
		meshes.resize( loadedMeshCount );
		for ( size_t i = 0; i < loadedMeshCount; ++i )
		{
			auto &loadedMesh = ( *pLoadedMeshes )[i];

			meshes[i].meshNo				= loadedMesh.meshNo;
			meshes[i].coordinateConversion	= loadedMesh.coordinateConversion;
			meshes[i].globalTransform		= loadedMesh.globalTransform;

			std::vector<Vertex> vertices{};
			AssignVertices( &vertices, loadedMesh );

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

				const std::string fileDirectory = loader.GetFileDirectory();

				auto FetchMaterialContain = [&fileDirectory]( SkinnedMesh::Material *meshMtl, const Loader::Material &loadedMtl )
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

		bool   createResult = pOutput->Init( argIndices, argVertices, meshes, pDevice );
		return createResult;
	}

	SkinnedMesh::SkinnedMesh() :
		wasCreated( false ),
		meshes(),
		cbPerMesh(), cbPerSubset(),
		iRasterizerStateWire(), iRasterizerStateSurface(), iDepthStencilState()
	{}
	SkinnedMesh::~SkinnedMesh()
	{
		meshes.clear();
		meshes.shrink_to_fit();
	}

	bool SkinnedMesh::Init( const std::vector<std::vector<size_t>> &allIndices, const std::vector<std::vector<Vertex>> &allVertices, const std::vector<Mesh> &loadedMeshes, ID3D11Device *pDevice )
	{
		if ( wasCreated      ) { return false; }
		if ( !meshes.empty() ) { return false; }
		// else

		HRESULT hr = S_OK;

		// Use default device.
		if ( !pDevice )
		{
			pDevice = Donya::GetDevice();
		}

		meshes = loadedMeshes;
		const size_t meshCount = meshes.size();

		// Create VertexBuffers
		for ( size_t i = 0; i < meshCount; ++i )
		{
			hr = CreateVertexBuffer<SkinnedMesh::Vertex>
			(
				pDevice, allVertices[i],
				D3D11_USAGE_IMMUTABLE, 0,
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
			bool succeeded = true;

			succeeded = cbPerMesh.Create( pDevice );
			if ( !succeeded )
			{
				_ASSERT_EXPR( 0, L"Failed : Create Constant-Buffer" );
				return false;
			}
			// else

			succeeded = cbPerSubset.Create( pDevice );
			if ( !succeeded )
			{
				_ASSERT_EXPR( 0, L"Failed : Create Constant-Buffer" );
				return false;
			}
			// else
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
		// Create DepthStencilState
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

	void SkinnedMesh::Render( const Donya::MotionChunk &motionPerMesh, const Donya::Animator &currentAnimation, const Donya::Vector4x4 &worldViewProjection, const Donya::Vector4x4 &world, const CBSetOption &cbopPerMesh, const CBSetOption &cbopPerSubset, unsigned int psSetSamplerSlot, unsigned int psSetDiffuseMapSlot, bool isEnableFill ) const
	{
		if ( !wasCreated )
		{
			_ASSERT_EXPR( 0, L"Error : The mesh was not created !" );
			return;
		}
		if ( meshes.empty() ) { return; }
		// else

		// If you use coordinate-conversion change GUI.
		/*
		// Apply coordinateConversion, TODO : Should refactoring this.
		{
			// Initialize to convert rhs to lhs.
			static Donya::Vector4x4 coordConversion
			{
				-1.0f, 0.0f, 0.0f, 0.0f,
				 0.0f, 1.0f, 0.0f, 0.0f,
				 0.0f, 0.0f, 1.0f, 0.0f,
				 0.0f, 0.0f, 0.0f, 1.0f,
			};

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
		*/

		ID3D11DeviceContext *pImmediateContext = Donya::GetImmediateContext();

		Microsoft::WRL::ComPtr<ID3D11RasterizerState>	prevRasterizerState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>		prevSamplerState;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	prevDepthStencilState;
		{
			pImmediateContext->RSGetState( prevRasterizerState.ReleaseAndGetAddressOf() );
			pImmediateContext->PSGetSamplers( psSetSamplerSlot, 1, prevSamplerState.ReleaseAndGetAddressOf() );
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

			pImmediateContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
			
			ID3D11RasterizerState	*ppRasterizerState
									= ( isEnableFill )
									? iRasterizerStateSurface.Get()
									: iRasterizerStateWire.Get();
			pImmediateContext->RSSetState( ppRasterizerState );

			pImmediateContext->OMSetDepthStencilState( iDepthStencilState.Get(), 0xFFFFFFFF );
		}

		for ( auto &mesh : meshes )
		{
			// Update Constant Buffer
			{
				cbPerMesh.data.worldViewProjection		= ( ( mesh.globalTransform * mesh.coordinateConversion ) * worldViewProjection ).XMFloat();
				cbPerMesh.data.world					= ( ( mesh.globalTransform * mesh.coordinateConversion ) * world ).XMFloat();

				const Donya::Motion		useMotion		= motionPerMesh.FetchMotion( scast<unsigned int>( mesh.meshNo ) );
				const Donya::Skeletal	currentPosture	= currentAnimation.FetchCurrentPose( useMotion );
				auto TransformBones = []( std::array<DirectX::XMFLOAT4X4, MAX_BONE_COUNT> *pBoneTransform, const Donya::Skeletal &pose )
				{
					const size_t poseBoneCount = pose.skeletal.size();
					for ( size_t i = 0; i < MAX_BONE_COUNT/* pBoneTransform->size() */; ++i )
					{
						( *pBoneTransform )[i] = ( poseBoneCount <= i )
						? Donya::Vector4x4::Identity().XMFloat()
						: pose.skeletal[i].transform.XMFloat();
					}
				};
				TransformBones( &cbPerMesh.data.boneTransforms, currentPosture );

				cbPerMesh.Activate( cbopPerMesh.setSlot, cbopPerMesh.setVS, cbopPerMesh.setPS, pImmediateContext );
			}
			
			UINT stride = sizeof( Vertex );
			UINT offset = 0;
			pImmediateContext->IASetVertexBuffers( 0, 1, mesh.iVertexBuffer.GetAddressOf(), &stride, &offset );
			pImmediateContext->IASetIndexBuffer( mesh.iIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );

			for ( auto &subset : mesh.subsets )
			{
				// Update Material-Constant Buffer
				{
					cbPerSubset.data.ambient	= subset.ambient.color.XMFloat();
					cbPerSubset.data.bump		= subset.bump.color.XMFloat();
					cbPerSubset.data.diffuse	= subset.diffuse.color.XMFloat();
					cbPerSubset.data.emissive	= subset.emissive.color.XMFloat();
					cbPerSubset.data.specular	= subset.specular.color.XMFloat();

					cbPerSubset.Activate( cbopPerSubset.setSlot, cbopPerSubset.setVS, cbopPerSubset.setPS, pImmediateContext );
				}

				pImmediateContext->PSSetSamplers( psSetSamplerSlot, 1, subset.diffuse.iSampler.GetAddressOf() );

				for ( auto &texture : subset.diffuse.textures )
				{
					pImmediateContext->PSSetShaderResources( psSetDiffuseMapSlot, 1, texture.iSRV.GetAddressOf() );
					
					pImmediateContext->DrawIndexed( subset.indexCount, subset.indexStart, 0 );
				}
			}
		}

		// PostProcessing
		{
			pImmediateContext->RSSetState( prevRasterizerState.Get() );

			ID3D11ShaderResourceView *pNullSRV = nullptr;
			pImmediateContext->PSSetShaderResources( psSetDiffuseMapSlot, 1, &pNullSRV );
			pImmediateContext->PSSetSamplers( psSetSamplerSlot, 1, prevSamplerState.GetAddressOf() );

			pImmediateContext->OMSetDepthStencilState( prevDepthStencilState.Get(), 1 );

			cbPerMesh.Deactivate( pImmediateContext );
			cbPerSubset.Deactivate( pImmediateContext );
		}
	}

}