#pragma once

#include <d3d11.h>
#include <vector>
#include <wrl.h>

#include "Donya/CBuffer.h"
#include "Donya/Donya.h"
#include "Donya/Shader.h"
#include "Donya/RenderingStates.h"
#include "Donya/Vector.h"

#include "ModelCommon.h"

namespace Donya
{
	namespace Model
	{
		namespace Impl
		{
			/// <summary>
			/// The base class of a primitive.
			/// </summary>
			class PrimitiveModel
			{
			protected:
				struct ConstantBase
				{
					Donya::Vector4x4	matWorld;
					Donya::Vector4x4	matViewProj;
					Donya::Vector4		drawColor;
					Donya::Vector3		lightDirection;
					float				lightBias = 0.5f; // Used to adjust the lighting influence. ( 1.0f - bias ) + ( light * bias )
				};
			private:
				Microsoft::WRL::ComPtr<ID3D11Buffer> pBufferPos;
			protected:
				bool wasCreated = false;
			public:
				virtual bool Create() = 0;
			protected:
				HRESULT CreateBufferPos( const std::vector<Vertex::Pos> &source );
			public:
				virtual void SetVertexBuffers() const;
				virtual void SetIndexBuffer() const;
				virtual void SetPrimitiveTopology() const = 0;
			public:
				virtual void CallDraw() const = 0;
			};

			template<typename PrimitiveConstant>
			class PrimitiveRenderer
			{
			protected:
				int idDS = 0;
				int idRS = 0;
				Donya::VertexShader	VS;
				Donya::PixelShader	PS;
				Donya::CBuffer<PrimitiveConstant> cbuffer;
			public:
				virtual bool Create() = 0;
			public:
				void ActivateVertexShader()		{ VS.Activate();	}
				void ActivatePixelShader()		{ PS.Activate();	}
				void DeactivateVertexShader()	{ VS.Deactivate();	}
				void DeactivatePixelShader()	{ PS.Deactivate();	}
			public:
				bool ActivateDepthStencil()
				{
					return Donya::DepthStencil::Activate( idDS, Donya::GetImmediateContext() );
				}
				bool ActivateRasterizer()
				{
					return Donya::Rasterizer::Activate( idRS, Donya::GetImmediateContext() );
				}
				void DeactivateDepthStencil()
				{
					Donya::DepthStencil::Deactivate( Donya::GetImmediateContext() );
				}
				void DeactivateRasterizer()
				{
					Donya::Rasterizer::Deactivate( Donya::GetImmediateContext() );
				}
			public:
				void UpdateConstant( const PrimitiveConstant &source )
				{
					cbuffer.data = source;
				}
				void ActivateConstant( unsigned int setSlot, bool setVS, bool setPS )
				{
					cbuffer.Activate( setSlot, setVS, setPS );
				}
				void DeactivateConstant()
				{
					cbuffer.Deactivate();
				}
			};
		}

		/// <summary>
		/// A unit size cube model. The center is origin(0.0f), min is -0.5f, max is +0.5f.
		/// </summary>
		class Cube : public Impl::PrimitiveModel
		{
		public:
			struct Constant : public Impl::PrimitiveModel::ConstantBase
			{};
		private:
			Microsoft::WRL::ComPtr<ID3D11Buffer> pIndexBuffer;
		public:
			bool Create() override;
		private:
			HRESULT CreateBufferIndex( const std::vector<size_t> &source );
		public:
			void SetIndexBuffer() const override;
			void SetPrimitiveTopology() const override;
		public:
			void CallDraw() const override;
		};
		/// <summary>
		/// Provides a shader and a constant buffer for the Cubes.
		/// </summary>
		class CubeRenderer : public Impl::PrimitiveRenderer<Cube::Constant>
		{
		public:
			bool Create() override;
		public:
			void ActivateConstant();
		public:
			/// <summary>
			/// Call the SetVertexBuffers(), SetIndexBuffer(), SetPrimitiveTopology(), CallDraw() of the cube.
			/// </summary>
			void Draw( const Cube &cube );
		};


		/// <summary>
		/// A unit size sphere model. The center is origin(0.0f), radius is 0.5f.
		/// </summary>
		class Sphere : public Impl::PrimitiveModel
		{
		public:
			struct Constant : public Impl::PrimitiveModel::ConstantBase
			{};
		private:
			size_t sliceCountH = NULL;
			size_t sliceCountV = NULL;
			size_t indexCount  = NULL;
			Microsoft::WRL::ComPtr<ID3D11Buffer> pIndexBuffer;
		public:
			Sphere( size_t sliceCountH = 12U, size_t sliceCountV = 6U );
		public:
			bool Create() override;
		private:
			HRESULT CreateBufferIndex( const std::vector<size_t> &source );
		public:
			void SetIndexBuffer() const override;
			void SetPrimitiveTopology() const override;
		public:
			void CallDraw() const override;
		};
		/// <summary>
		/// Provides a shader and a constant buffer for the Spheres.
		/// </summary>
		class SphereRenderer : public Impl::PrimitiveRenderer<Sphere::Constant>
		{
		public:
			bool Create() override;
		public:
			void ActivateConstant();
		public:
			/// <summary>
			/// Call the SetVertexBuffers(), SetIndexBuffer(), SetPrimitiveTopology(), CallDraw() of the cube.
			/// </summary>
			void Draw( const Sphere &sphere );
		};
	}
}
