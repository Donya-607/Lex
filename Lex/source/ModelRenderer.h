#pragma once

#include <array>			// Use std::array for bone-transforms.
#include <d3d11.h>
#include <memory>

#include "Donya/CBuffer.h"
#include "Donya/Shader.h"
#include "Donya/Vector.h"

#include "ModelAnimator.h"
#include "ModelMaker.h"		// Use for ModelUsage, and specify to friend to making function.
#include "ModelMotion.h"

namespace Donya
{
	namespace Model
	{
		template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

		namespace Strategy
		{
			namespace CBStructPerMesh = Constants::PerMesh;

			/// <summary>
			/// Use for toggle a constant-buffer by the specification of vertex type.<para></para>
			/// But this must update also a member of constants because the actual constant-buffer type is known only this.<para></para>
			/// So this uses also constant-buffer.
			/// </summary>
			class IConstantsPerMesh
			{
			public:
				virtual bool CreateBuffer( ID3D11Device *pDevice ) = 0;
			public:
				virtual void Update( const CBStructPerMesh::Common &source ) = 0;
				virtual void Update( const CBStructPerMesh::Common &sourceCommon, const CBStructPerMesh::Bone &sourceBone ) {}
			public:
				virtual void Activate( unsigned int setSlot, bool setVS, bool setPS, ID3D11DeviceContext *pImmediateContext ) const = 0;
				virtual void Deactivate( ID3D11DeviceContext *pImmediateContext ) const = 0;
			};

			class SkinningConstantsPerMesh : public IConstantsPerMesh
			{
			public:
				static constexpr unsigned int MAX_BONE_COUNT = 64U;
			public:
				struct Constants
				{
					CBStructPerMesh::Common common;
					CBStructPerMesh::Bone   bone;
				};
			private:
				Donya::CBuffer<Constants> cbuffer;
			public:
				bool CreateBuffer( ID3D11Device *pDevice ) override;
			public:
				void Update( const CBStructPerMesh::Common &source ) override;
				void Update( const CBStructPerMesh::Common &sourceCommon, const CBStructPerMesh::Bone &sourceBone ) override;
			public:
				void Activate( unsigned int setSlot, bool setVS, bool setPS, ID3D11DeviceContext *pImmediateContext ) const override;
				void Deactivate( ID3D11DeviceContext *pImmediateContext ) const override;
			};
			class StaticConstantsPerMesh : public IConstantsPerMesh
			{
			public:
				struct Constants
				{
					CBStructPerMesh::Common common;
				};
			private:
				Donya::CBuffer<Constants> cbuffer;
			public:
				bool CreateBuffer( ID3D11Device *pDevice ) override;
			public:
				void Update( const CBStructPerMesh::Common &source ) override;
			public:
				void Activate( unsigned int setSlot, bool setVS, bool setPS, ID3D11DeviceContext *pImmediateContext ) const override;
				void Deactivate( ID3D11DeviceContext *pImmediateContext ) const override;
			};
		}

		class Model; // Use for a reference at Render() method.
	
		/// <summary>
		/// Renderer of the Model class.<para></para>
		/// If you do not want using a custom shader, you can use a default shading by internal class.
		/// </summary>
		class ModelRenderer
		{
		public:
			/// <summary>
			/// Provides a default shading and settings.<para></para>
			/// You can use:<para></para>
			/// Depth Stencil state,<para></para>
			/// Rasterizer state,<para></para>
			/// Sampler state,<para></para>
			/// (The Blend state is there at Blend.h)<para></para>
			/// Constant Buffer of Donya::Model::Constants::PerScene,<para></para>
			/// Shaders for skinning,<para></para>
			/// Shaders for static.<para></para>
			/// Use with ActivateXXX / DeactivateXXX.
			/// </summary>
			class Default
			{
			private:
				struct Member
				{
					static constexpr int DEFAULT_ID = 0;
					int idDSState	= DEFAULT_ID;
					int idRSState	= DEFAULT_ID;
					int idPSSampler	= DEFAULT_ID;
					Donya::CBuffer<Constants::PerScene::Common> CBPerScene;

					struct Shader
					{
						Donya::VertexShader VS;
						Donya::PixelShader  PS;
					};
					Shader shaderSkinning;
					Shader shaderStatic;
				};
			private:
				static std::unique_ptr<Member> pMember;
			public:
				/// <summary>
				/// Initialize all default settings.<para></para>
				/// Please call this before the initialization of application(before a game-loop).<para></para>
				/// Returns true if all initialization was succeeded, or already initialized.
				/// </summary>
				static bool Initialize( ID3D11Device *pDevice );
			private:
				static bool AssignStatusIdentifiers( ID3D11Device *pDevice );
				static bool CreateRenderingStates  ( ID3D11Device *pDevice );
				static bool CreateDefaultShaders   ( ID3D11Device *pDevice );
			public:
				/// <summary>
				/// If set nullptr to "pImmediateContext", use default device.
				/// </summary>
				static bool ActivateDepthStencil( ID3D11DeviceContext *pImmediateContext = nullptr );
				/// <summary>
				/// If set nullptr to "pImmediateContext", use default device.
				/// </summary>
				static bool ActivateRasterizer( ID3D11DeviceContext *pImmediateContext = nullptr );
				/// <summary>
				/// If set nullptr to "pImmediateContext", use default device.
				/// </summary>
				static bool ActivateSampler( const RegisterDesc &setting, ID3D11DeviceContext *pImmediateContext = nullptr );
				/// <summary>
				/// If set nullptr to "pImmediateContext", use default device.
				/// </summary>
				static void DeactivateDepthStencil( ID3D11DeviceContext *pImmediateContext = nullptr );
				/// <summary>
				/// If set nullptr to "pImmediateContext", use default device.
				/// </summary>
				static void DeactivateRasterizer( ID3D11DeviceContext *pImmediateContext = nullptr );
				/// <summary>
				/// If set nullptr to "pImmediateContext", use default device.
				/// </summary>
				static void DeactivateSampler( ID3D11DeviceContext *pImmediateContext = nullptr );
			public:
				/// <summary>
				/// If set nullptr to "pImmediateContext", use default device.
				/// </summary>
				static void ActivateVertexShaderSkinning( ID3D11DeviceContext *pImmediateContext = nullptr );
				/// <summary>
				/// If set nullptr to "pImmediateContext", use default device.
				/// </summary>
				static void ActivatePixelShaderSkinning( ID3D11DeviceContext *pImmediateContext = nullptr );
				/// <summary>
				/// If set nullptr to "pImmediateContext", use default device.
				/// </summary>
				static void ActivateVertexShaderStatic( ID3D11DeviceContext *pImmediateContext = nullptr );
				/// <summary>
				/// If set nullptr to "pImmediateContext", use default device.
				/// </summary>
				static void ActivatePixelShaderStatic( ID3D11DeviceContext *pImmediateContext = nullptr );
				/// <summary>
				/// If set nullptr to "pImmediateContext", use default device.
				/// </summary>
				static void DeactivateVertexShaderSkinning( ID3D11DeviceContext *pImmediateContext = nullptr );
				/// <summary>
				/// If set nullptr to "pImmediateContext", use default device.
				/// </summary>
				static void DeactivatePixelShaderSkinning( ID3D11DeviceContext *pImmediateContext = nullptr );
				/// <summary>
				/// If set nullptr to "pImmediateContext", use default device.
				/// </summary>
				static void DeactivateVertexShaderStatic( ID3D11DeviceContext *pImmediateContext = nullptr );
				/// <summary>
				/// If set nullptr to "pImmediateContext", use default device.
				/// </summary>
				static void DeactivatePixelShaderStatic( ID3D11DeviceContext *pImmediateContext = nullptr );
			public:
				static void UpdateSceneConstants( const Constants::PerScene::Common &assignParameter );
				/// <summary>
				/// If set nullptr to "pImmediateContext", use default device.
				/// </summary>
				static void ActivateSceneConstants( const RegisterDesc &setting, ID3D11DeviceContext *pImmediateContext = nullptr );
				/// <summary>
				/// If set nullptr to "pImmediateContext", use default device.
				/// </summary>
				static void DeactivateSceneConstants( ID3D11DeviceContext *pImmediateContext = nullptr );
			};
		public:
			/// <summary>
			/// Returns input-element-descs that used for create a default vertex-shader.<para></para>
			/// That is the synthesis of return value of the static method of some structure's that belongs to Donya::Vertex.
			/// </summary>
			static std::vector<D3D11_INPUT_ELEMENT_DESC> GetInputElementDescs( ModelUsage usage );
		private:
			Donya::CBuffer<Constants::PerSubset::Common> CBPerSubset;
		private:
			// This making function declared at "ModelMaker.h".
			friend size_t MakeRenderer( ModelUsage usage, ID3D11Device *pDevice );
		protected:
			/// <summary>
			/// If set nullptr to "pDevice", use default device.
			/// </summary>
			ModelRenderer( ID3D11Device *pDevice = nullptr );
			virtual ~ModelRenderer() = default;
		protected:
			void SetVertexBuffers( const Donya::Model::Model &model, size_t meshIndex, ID3D11DeviceContext *pImmediateContext );
			void SetIndexBuffer( const Donya::Model::Model &model, size_t meshIndex, ID3D11DeviceContext *pImmediateContext );

			Constants::PerMesh::Common MakeCommonConstantsPerMesh( const Model &model, size_t meshIndex ) const;

			void DrawEachSubsets( const Donya::Model::Model &model, size_t meshIndex, const RegisterDesc &subsetSetting, const RegisterDesc &diffuseMapSetting, ID3D11DeviceContext *pImmediateContext );
		private:
			void UpdateCBPerSubset( const Donya::Model::Model &model, size_t meshIndex, size_t subsetIndex, const RegisterDesc &subsetSettings, ID3D11DeviceContext *pImmediateContext );
			void ActivateCBPerSubset( const RegisterDesc &subsetSettings, ID3D11DeviceContext *pImmediateContext );
			void DeactivateCBPerSubset( ID3D11DeviceContext *pImmediateContext );

			using SRVType = ID3D11ShaderResourceView * const *;
			void SetTexture( const RegisterDesc &diffuseSettings, SRVType diffuseSRV, ID3D11DeviceContext *pImmediateContext ) const;
			void UnsetTexture( const RegisterDesc &diffuseSettings, ID3D11DeviceContext *pImmediateContext ) const;

			void DrawIndexed( const Donya::Model::Model &model, size_t meshIndex, size_t subsetIndex, ID3D11DeviceContext *pImmediateContext ) const;
		};

		class SkinningRenderer : public ModelRenderer
		{
		private:
			Strategy::SkinningConstantsPerMesh CBPerMesh;
		private:
			/// <summary>
			/// If set nullptr to "pDevice", use default device.
			/// </summary>
			SkinningRenderer( ID3D11Device *pDevice = nullptr );
		public:
			void Render
			(
				const Model			&model,
				const FocusMotion	&activeMotion,
				const RegisterDesc	&meshSetting,
				const RegisterDesc	&subsetSetting,
				const RegisterDesc	&diffuseMapSetting,
				ID3D11DeviceContext	*pImmediateContext
			);
		private:
			Constants::PerMesh::Bone MakeBoneConstants( const Model &model, size_t meshIndex, const FocusMotion &activeMotion ) const;
			void UpdateCBPerMesh( const Model &model, size_t meshIndex, const FocusMotion &activeMotion, const RegisterDesc &meshSetting, ID3D11DeviceContext *pImmediateContext );
			void ActivateCBPerMesh( const RegisterDesc &meshSetting, ID3D11DeviceContext *pImmediateContext );
			void DeactivateCBPerMesh( ID3D11DeviceContext *pImmediateContext );
		};
	}
}
