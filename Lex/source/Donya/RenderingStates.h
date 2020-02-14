#pragma once

struct D3D11_DEPTH_STENCIL_DESC;
struct D3D11_RASTERIZER_DESC;
struct D3D11_SAMPLER_DESC;
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace Donya
{
	// The Blend-State is there at "Blend.h".
	// Because the timing of creating this file was different from "Blend.h".
	// And I think the using frequency by a user is lower than "Blend.h".

	/// <summary>
	/// Provides caching system for global access.
	/// </summary>
	namespace DepthStencil
	{
		/// <summary>
		/// Create and cache a depth-stencil-state object to an internal map.<para></para>
		/// If you set nullptr(default) to "pDevice", I use default(library's) device.<para></para>
		/// Returns true if succeeded a creation, or already created.<para></para>
		/// I recommend using enum value to the identifier.
		/// </summary>
		bool CreateState( int identifier, const D3D11_DEPTH_STENCIL_DESC &registerDesc, ID3D11Device *pDevice = nullptr );

		/// <summary>
		/// Returns true if the identifier already used for creating. This confirmation is also used internally at the create method.
		/// </summary>
		bool IsAlreadyExists( int identifier );

		/// <summary>
		/// Activate the specify depth-stencil-state.<para></para>
		/// If you set nullptr(default) to "pImmediateContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed activation(maybe the identifier does not cache?).
		/// </summary>
		bool Activate( int identifier, ID3D11DeviceContext *pImmediateContext = nullptr );
		/// <summary>
		/// Deactivate current depth-stencil-state.<para></para>
		/// If you set nullptr(default) to "pImmediateContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed deactivation(maybe the identifier does not cache?).
		/// </summary>
		void Deactivate( ID3D11DeviceContext *pImmediateContext = nullptr );

		/// <summary>
		/// Release all a cached depth-stencil-states. The identifiers of released state will be invalid.
		/// </summary>
		void ReleaseAllCachedStates();
	}

	/// <summary>
	/// Provides caching system for global access.
	/// </summary>
	namespace Rasterizer
	{
		/// <summary>
		/// Create and cache a rasterizer-state object to an internal map.<para></para>
		/// If you set nullptr(default) to "pDevice", I use default(library's) device.<para></para>
		/// Returns true if succeeded a creation, or already created.<para></para>
		/// I recommend using enum value to the identifier.
		/// </summary>
		bool CreateState( int identifier, const D3D11_RASTERIZER_DESC &registerDesc, ID3D11Device *pDevice = nullptr );

		/// <summary>
		/// Returns true if the identifier already used for creating. This confirmation is also used internally at the create method.
		/// </summary>
		bool IsAlreadyExists( int identifier );

		/// <summary>
		/// Activate the specify rasterizer-state.<para></para>
		/// If you set nullptr(default) to "pImmediateContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed activation(maybe the identifier does not cache?).
		/// </summary>
		bool Activate( int identifier, ID3D11DeviceContext *pImmediateContext = nullptr );
		/// <summary>
		/// Deactivate current rasterizer-state.<para></para>
		/// If you set nullptr(default) to "pImmediateContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed deactivation(maybe the identifier does not cache?).
		/// </summary>
		void Deactivate( ID3D11DeviceContext *pImmediateContext = nullptr );

		/// <summary>
		/// Release all a cached rasterizer-states. The identifiers of released state will be invalid.
		/// </summary>
		void ReleaseAllCachedStates();
	}

	/// <summary>
	/// Provides caching system for global access.
	/// </summary>
	namespace Sampler
	{
		/// <summary>
		/// Create and cache a sampler-state object to an internal map.<para></para>
		/// If you set nullptr(default) to "pDevice", I use default(library's) device.<para></para>
		/// I recommend using enum value to the identifier.
		/// </summary>
		bool CreateState( int identifier, const D3D11_SAMPLER_DESC &registerDesc, ID3D11Device *pDevice = nullptr );

		/// <summary>
		/// Returns true if the identifier already used for creating. This confirmation is also used internally at the create method.
		/// </summary>
		bool IsAlreadyExists( int identifier );

		/// <summary>
		/// Activate the specify sampler-state.<para></para>
		/// If you set nullptr(default) to "pImmediateContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed activation(maybe the identifier does not cache?).
		/// </summary>
		bool Activate( int identifier, unsigned int setSlot, bool setVS = true, bool setPS = true, ID3D11DeviceContext *pImmediateContext = nullptr );

		/// <summary>
		/// Activate the specify sampler-state.<para></para>
		/// If you set nullptr(default) to "pImmediateContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed activation(maybe the identifier does not cache?).
		/// </summary>
		// FUTURE : Implement set a many samplers.
		// bool Activate( std::vector<int> identifier, unsigned int setSlot, bool setVS = true, bool setPS = true, ID3D11DeviceContext *pImmediateContext = nullptr );

		/// <summary>
		/// Deactivate current sampler-state.<para></para>
		/// If you set nullptr(default) to "pImmediateContext", I use default(library's) immediate-context.<para></para>
		/// Returns false if failed deactivation(maybe the identifier does not cache?).
		/// </summary>
		void Deactivate( ID3D11DeviceContext *pImmediateContext = nullptr );

		/// <summary>
		/// Release all a cached sampler-states. The identifiers of released state will be invalid.
		/// </summary>
		void ReleaseAllCachedStates();
	}
}
