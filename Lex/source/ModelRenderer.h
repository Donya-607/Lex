#pragma once

#include <array>			// Use std::array for bone-transforms.
#include <d3d11.h>
#include <memory>

#include "Donya/CBuffer.h"
#include "Donya/Vector.h"

#include "ModelMaker.h"		// Use for ModelUsage, and specify to friend to making function.

namespace Donya
{
	template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	namespace Strategy
	{
		/// <summary>
		/// Use for toggle a constant-buffer by the specification of vertex type.<para></para>
		/// But this must update also a member of constants because the actual constant-buffer type is known only this.<para></para>
		/// So this uses also constant-buffer.
		/// </summary>
		class IConstantsPerMesh
		{
		public:
			/// <summary>
			/// Create the class derived from me.<para></para>
			/// Returns the pointer to derived class, or pointer to nullptr if failed the creation.
			/// </summary>
			template<class DerivedConstants>
			static std::unique_ptr<IConstantsPerMesh> Create()
			{
				DerivedConstants  tmp{};
				bool  succeeded = tmp.CreateBuffer();
				if ( !succeeded )
				{
					_ASSERT_EXPR( 0, L"Failed : The creation of ModelRenderer's constant-buffer." );
					return std::make_unique<IConstantsPerMesh>( nullptr );
				}
				// else

				return std::make_unique<IConstantsPerMesh>( std::move( tmp ) );
			}
		protected:
			IConstantsPerMesh() = default;
			IConstantsPerMesh( const IConstantsPerMesh & ) = default;
			IConstantsPerMesh &operator = ( const IConstantsPerMesh & ) = default;
			IConstantsPerMesh( IConstantsPerMesh && ) = default;
			IConstantsPerMesh &operator = ( IConstantsPerMesh && ) = default;
			virtual bool CreateBuffer()	= 0;
		public:
			virtual void Activate()		= 0;
			virtual void Deactivate()	= 0;
		};

		class SkinnedConstantsPerMesh : public IConstantsPerMesh
		{
		public:
			static constexpr unsigned int MAX_BONE_COUNT = 64U;
		public:
			struct Constants
			{
				using BoneMatricesType = std::array<Donya::Vector4x4, MAX_BONE_COUNT>;

				Donya::Vector4x4 adjustMatrix;		// Model space. This matrix contain a global-transform, and coordinate-conversion matrix.
				BoneMatricesType boneTransforms;	// This matrix transforms to world space of game from bone space in initial-pose.
			};
		private:
			Donya::CBuffer<Constants> cbuffer;
		private:
			bool CreateBuffer()	override;
		public:
			void Activate()		override;
			void Deactivate()	override;
		};
		class StaticConstantsPerMesh : public IConstantsPerMesh
		{
		public:
			struct Constants
			{
				Donya::Vector4x4 adjustMatrix; // Model space. This matrix contain a global-transform, and coordinate-conversion matrix.
			};
		private:
			Donya::CBuffer<Constants> cbuffer;
		private:
			bool CreateBuffer()	override;
		public:
			void Activate()		override;
			void Deactivate()	override;
		};
	}

	/// <summary>
	/// Render a 3D-model.
	/// </summary>
	class ModelRenderer
	{
	private:
		// Static members. Use for default shading.
	private:
		/// <summary>
		/// This constants mainly contain scene info.
		/// </summary>
		struct OtherConstants
		{
			Donya::Vector4x4	matWVP;
			Donya::Vector4		lightColor;
			Donya::Vector4		lightDirection;
			Donya::Vector4		drawColor;
		};
		struct DefaultStatus
		{
			int idDSState;
			int idRSState;
			int idPSSampler;
			Donya::CBuffer<OtherConstants> CBForDefault;
		};
		static std::unique_ptr<DefaultStatus> pDefaultStatus;
	public:
		/// <summary>
		/// Initialization of default shading statuses.<para></para>
		/// Please call only once a time when the initialize of application(before a game-loop).<para></para>
		/// If set nullptr to "pDevice", use default device.
		/// </summary>
		static bool InitDefaultStatus( ID3D11Device *pDevice = nullptr );
	private:
		static void AssignStatusIdentifiers( DefaultStatus *pStatus );
	public:
	// Instance members.
	public:
		struct ConstantsPerSubset
		{
			Donya::Vector4 ambient;
			Donya::Vector4 diffuse;
			Donya::Vector4 specular;
		};
	private:
		std::shared_ptr<Strategy::IConstantsPerMesh> pCBPerMesh;
		Donya::CBuffer<ConstantsPerSubset> CBPerSubset;
	private:
		// This making function declared at "ModelMaker.h".
		friend size_t MakeRenderer( ModelUsage usage, ID3D11Device *pDevice );
		/// <summary>
		/// If set nullptr to "pDevice", use default device.
		/// </summary>
		ModelRenderer( Donya::ModelUsage usage, ID3D11Device *pDevice = nullptr );
	private:
		bool AssignSpecifiedCBuffer( Donya::ModelUsage usage );
	};
}
