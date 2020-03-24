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
				virtual void Draw() const = 0;
			};

			template<typename PrimitiveConstant>
			class PrimitiveRenderer
			{
				// TODO : Should include the states(depthStencil, rasterizer).
			protected:
				int idDS = 0;
				int idRS = 0;
				Donya::VertexShader	VS;
				Donya::PixelShader	PS;
				Donya::CBuffer<PrimitiveConstant> cbuffer;
				Donya::CBuffer<Donya::Vector4x4>  cbufferVP;
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
				void UpdateVP( const Donya::Vector4x4 &viewProjectionMatrix )
				{
					cbufferVP.data = viewProjectionMatrix;
				}
				void ActivateConstant( unsigned int setSlot, bool setVS, bool setPS )
				{
					cbuffer.Activate( setSlot, setVS, setPS );
				}
				void ActivateVP( unsigned int setSlot, bool setVS, bool setPS )
				{
					cbufferVP.Activate( setSlot, setVS, setPS );
				}
				void DeactivateConstant()
				{
					cbuffer.Deactivate();
				}
				void DeactivateVP()
				{
					cbufferVP.Deactivate();
				}
			};
		}

		/// <summary>
		/// A unit size cube model. The center is origin(0.0f), min is -0.5f, max is +0.5f.
		/// </summary>
		class Cube : public Impl::PrimitiveModel
		{
		public:
			struct Constant
			{
				Donya::Vector4x4	world;
				Donya::Vector4		color;
				Donya::Vector3		lightDirection;
				float				lightBias = 1.0f; // Used to adjust the lighting influence.
			};
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
			void Draw() const override;
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
			void ActivateVP();
		};
	}
}
