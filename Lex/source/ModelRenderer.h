#pragma once

#include <array>			// Use std::array for bone-transforms.
#include <d3d11.h>
#include <memory>

#include "Donya/CBuffer.h"
#include "Donya/Shader.h"
#include "Donya/Vector.h"

#include "ModelAnimator.h"
#include "ModelMaker.h"		// Use for ModelUsage, and specify to friend to making function.

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

			class SkinnedConstantsPerMesh : public IConstantsPerMesh
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
		/// Render a 3D-model.
		/// </summary>
		class ModelRenderer
		{
		private:
			// Static members. Use for default shading.
		private:
			struct DefaultStatus
			{
				static constexpr int DEFAULT_ID = 0;
				int idDSState	= DEFAULT_ID;
				int idRSState	= DEFAULT_ID;
				int idPSSampler	= DEFAULT_ID;
				Donya::CBuffer<Constants::PerNeed::Common> CBPerScene;

				struct Shader
				{
					Donya::VertexShader VS;
					Donya::PixelShader  PS;
				};
				Shader shaderSkinned;
				Shader shaderStatic;
			};
		private:
			static std::unique_ptr<DefaultStatus> pDefaultStatus;
		public:
			/// <summary>
			/// Returns input-element-descs that used for create a default vertex-shader.<para></para>
			/// That is the synthesis of return value of the static method of some structure's that belongs to Donya::Vertex.
			/// </summary>
			static std::vector<D3D11_INPUT_ELEMENT_DESC> GetInputElementDescs( ModelUsage usage );
			/// <summary>
			/// Initialization of default shading statuses.<para></para>
			/// Please call only once a time when the initialize of application(before a game-loop).<para></para>
			/// If set nullptr to "pDevice", use default device.
			/// </summary>
			static bool InitDefaultStatus( ID3D11Device *pDevice = nullptr );			
		private:
			static bool AssignStatusIdentifiers( DefaultStatus *pStatus );
			static bool CreateRenderingStates  ( DefaultStatus *pStatus );
			static bool CreateDefaultShaders   ( DefaultStatus *pStatus );
		public:
			/// <summary>
			/// If set nullptr to "pImmediateContext", use default device.
			/// </summary>
			static bool ActivateDefaultStateDepthStencil( ID3D11DeviceContext *pImmediateContext = nullptr );
			/// <summary>
			/// If set nullptr to "pImmediateContext", use default device.
			/// </summary>
			static void DeactivateDefaultStateDepthStencil( ID3D11DeviceContext *pImmediateContext = nullptr );
			/// <summary>
			/// If set nullptr to "pImmediateContext", use default device.
			/// </summary>
			static bool ActivateDefaultStateRasterizer( ID3D11DeviceContext *pImmediateContext = nullptr );
			/// <summary>
			/// If set nullptr to "pImmediateContext", use default device.
			/// </summary>
			static void DeactivateDefaultStateRasterizer( ID3D11DeviceContext *pImmediateContext = nullptr );
			/// <summary>
			/// If set nullptr to "pImmediateContext", use default device.
			/// </summary>
			static bool ActivateDefaultStateSampler( const TextureDesc &setting, ID3D11DeviceContext *pImmediateContext = nullptr );
			/// <summary>
			/// If set nullptr to "pImmediateContext", use default device.
			/// </summary>
			static void DeactivateDefaultStateSampler( ID3D11DeviceContext *pImmediateContext = nullptr );
		public:
			/// <summary>
			/// If set nullptr to "pImmediateContext", use default device.
			/// </summary>
			static void ActivateDefaultVertexShaderSkinned( ID3D11DeviceContext *pImmediateContext = nullptr );
			/// <summary>
			/// If set nullptr to "pImmediateContext", use default device.
			/// </summary>
			static void DeactivateDefaultVertexShaderSkinned( ID3D11DeviceContext *pImmediateContext = nullptr );
			/// <summary>
			/// If set nullptr to "pImmediateContext", use default device.
			/// </summary>
			static void ActivateDefaultPixelShaderSkinned( ID3D11DeviceContext *pImmediateContext = nullptr );
			/// <summary>
			/// If set nullptr to "pImmediateContext", use default device.
			/// </summary>
			static void DeactivateDefaultPixelShaderSkinned( ID3D11DeviceContext *pImmediateContext = nullptr );
		public:
			static void UpdateDefaultConstants( const Constants::PerNeed::Common &assignParameter );
			/// <summary>
			/// If set nullptr to "pImmediateContext", use default device.
			/// </summary>
			static void ActivateDefaultConstants( const ConstantDesc &settings, ID3D11DeviceContext *pImmediateContext = nullptr );
			/// <summary>
			/// If set nullptr to "pImmediateContext", use default device.
			/// </summary>
			static void DeactivateDefaultConstants( ID3D11DeviceContext *pImmediateContext = nullptr );
		public:
		// Instance members.
		private:
			const ModelUsage inputUsage;
			Donya::CBuffer<Constants::PerModel::Common>  CBPerModel;
			std::shared_ptr<Strategy::IConstantsPerMesh> pCBPerMesh;
			Donya::CBuffer<Constants::PerSubset::Common> CBPerSubset;
		private:
			// This making function declared at "ModelMaker.h".
			friend size_t MakeRenderer( ModelUsage usage, ID3D11Device *pDevice );
		private:
			/// <summary>
			/// If set nullptr to "pDevice", use default device.
			/// </summary>
			ModelRenderer( ModelUsage usage, ID3D11Device *pDevice = nullptr );
			bool AssignSpecifiedCBuffer( ModelUsage usage, ID3D11Device *pDevice );
		public:
			/// <summary>
			/// Enable a configuration of drawing some model.<para></para>
			/// If set nullptr to "pImmediateContext", use default device.
			/// </summary>
			void ActivateModelConstants( const Constants::PerModel::Common &enableData, const ConstantDesc &settingDesc, ID3D11DeviceContext *pImmediateContext = nullptr );
			/// <summary>
			/// Disable a configuration of drawing some model.<para></para>
			/// If set nullptr to "pImmediateContext", use default device.
			/// </summary>
			void DeactivateModelConstants( ID3D11DeviceContext *pImmediateContext = nullptr ) const;
			/// <summary>
			/// Call a draw method by each subset of each mesh of the model.<para></para>
			/// This method is "Skinned" version. If the usage that specified when creation is different, returns false and don't render.<para></para>
			/// If set nullptr to "pImmediateContext", use default device.
			/// </summary>
			bool RenderSkinned( const Donya::Model::Model &model,/* const Animation::Motion &motion,*/ const Animator &animator, const ConstantDesc &meshSettings, const ConstantDesc &subsetSettings, const TextureDesc &diffuseSettings, ID3D11DeviceContext *pImmediateContext = nullptr );
			/// <summary>
			/// Call a draw method by each subset of each mesh of the model.<para></para>
			/// This method is "Static" version. If the usage that specified when creation is different, returns false and don't render.<para></para>
			/// If set nullptr to "pImmediateContext", use default device.
			/// </summary>
			bool RenderStatic( const Donya::Model::Model &model, const ConstantDesc &meshSettings, const ConstantDesc &subsetSettings, const TextureDesc &diffuseSettings, ID3D11DeviceContext *pImmediateContext = nullptr );
		private:
			bool EnableSkinned();

			Constants::PerMesh::Common MakeConstantsCommon( const Donya::Model::Model &model, size_t meshIndex ) const;
			Constants::PerMesh::Bone   MakeConstantsBone  ( const Donya::Model::Model &model, size_t meshIndex, const Animation::KeyFrame &currentPose ) const;
			void UpdateConstantsPerMeshSkinned( const Donya::Model::Model &model, size_t meshIndex, const Animation::KeyFrame &currentPose, const ConstantDesc &meshSettings, ID3D11DeviceContext *pImmediateContext );
			void UpdateConstantsPerMeshStatic ( const Donya::Model::Model &model, size_t meshIndex, const ConstantDesc &meshSettings, ID3D11DeviceContext *pImmediateContext );
			void ActivateCBPerMesh( const ConstantDesc &meshSettings, ID3D11DeviceContext *pImmediateContext );
			void DeactivateCBPerMesh( ID3D11DeviceContext *pImmediateContext );

			void UpdateConstantsPerSubset( const Donya::Model::Model &model, size_t meshIndex, size_t subsetIndex, const ConstantDesc &subsetSettings, ID3D11DeviceContext *pImmediateContext );
			void ActivateCBPerSubset( const ConstantDesc &subsetSettings, ID3D11DeviceContext *pImmediateContext );
			void DeactivateCBPerSubset( ID3D11DeviceContext *pImmediateContext );

			using SRVType = ID3D11ShaderResourceView * const *;
			void SetTexture( const TextureDesc &diffuseSettings, SRVType diffuseSRV, ID3D11DeviceContext *pImmediateContext ) const;
			void ResetTexture( const TextureDesc &diffuseSettings, ID3D11DeviceContext *pImmediateContext ) const;
		};
	}
}
