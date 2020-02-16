#pragma once

#include <array>			// Use std::array for bone-transforms.
#include <d3d11.h>
#include <memory>

#include "Donya/CBuffer.h"
#include "Donya/Shader.h"
#include "Donya/Vector.h"

#include "ModelMaker.h"		// Use for ModelUsage, and specify to friend to making function.

namespace Donya
{
	template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	namespace Strategy
	{
		namespace CBStructPerMesh = Donya::Constants::PerMesh;

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
		static std::vector<D3D11_INPUT_ELEMENT_DESC> GetInputElementDescs( Donya::ModelUsage usage );
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
		ModelRenderer( Donya::ModelUsage usage, ID3D11Device *pDevice = nullptr );
		bool AssignSpecifiedCBuffer( Donya::ModelUsage usage, ID3D11Device *pDevice );
	public:
		/// <summary>
		/// Enable a configuration of drawing some model.<para></para>
		/// If set nullptr to "pImmediateContext", use default device.
		/// </summary>
		void ActivateModelConstants( const Constants::PerModel::Common &enableData, const Donya::ConstantDesc &settingDesc, ID3D11DeviceContext *pImmediateContext = nullptr );
		/// <summary>
		/// Disable a configuration of drawing some model.<para></para>
		/// If set nullptr to "pImmediateContext", use default device.
		/// </summary>
		void DeactivateModelConstants( ID3D11DeviceContext *pImmediateContext = nullptr ) const;
		/// <summary>
		/// Call a draw method by each subset of each mesh of the model.<para></para>
		/// If set nullptr to "pImmediateContext", use default device.
		/// </summary>
		void Render( const Donya::Model &model, const Donya::ConstantDesc &meshSettings, const Donya::ConstantDesc &subsetSettings, ID3D11DeviceContext *pImmediateContext = nullptr );
	private:
		bool EnableSkinned();

		Constants::PerMesh::Common MakeConstantsCommon( const Donya::Model &model, size_t meshIndex ) const;
		Constants::PerMesh::Bone   MakeConstantsBone  ( const Donya::Model &model, size_t meshIndex ) const;
		void UpdateConstantsPerMeshSkinned( const Donya::Model &model, size_t meshIndex, const Donya::ConstantDesc &meshSettings, ID3D11DeviceContext *pImmediateContext );
		void UpdateConstantsPerMeshStatic ( const Donya::Model &model, size_t meshIndex, const Donya::ConstantDesc &meshSettings, ID3D11DeviceContext *pImmediateContext );
		void ActivateCBPerMesh( const Donya::ConstantDesc &meshSettings, ID3D11DeviceContext *pImmediateContext );
	};
}
