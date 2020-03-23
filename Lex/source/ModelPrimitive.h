#pragma once

#include <d3d11.h>
#include <vector>
#include <wrl.h>

#include "Donya/CBuffer.h"
#include "Donya/Shader.h"
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
			public:
				virtual bool Create() = 0;
			protected:
				HRESULT CreateBufferPos( const std::vector<Vertex::Pos> &source );
			public:
				void SetVertexBuffers() const;
				void SetIndexBuffer() const;
				virtual void SetPrimitiveTopology() const = 0;
			public:
				virtual void Draw() = 0;
			};

			template<typename PrimitiveConstant>
			class PrimitiveRenderer
			{
			private:
				Donya::VertexShader	VS;
				Donya::PixelShader	PS;
				Donya::CBuffer<PrimitiveConstant> cbuffer;
			public:
				virtual bool Create() = 0;
			public:
				void ActivateShader()	{ VS.Activate();	}
				void DeactivateShader()	{ VS.Deactivate();	}
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
			struct Constant
			{
				Donya::Vector4x4	world;
				Donya::Vector4		color;
				Donya::Vector3		lightDirection;
				float				lightBias = 1.0f; // Used to adjust the lighting influence.
			};
		public:
			bool Create() override;
		public:
			void SetPrimitiveTopology() const override;
		public:
			void Draw() override;
		};
		/// <summary>
		/// Provides a shader and a constant buffer for the Cubes.
		/// </summary>
		class CubeRenderer : public Impl::PrimitiveRenderer<Cube::Constant>
		{
		public:
			bool Create() override;
		};
	}
}
