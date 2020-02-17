#pragma once

#include <cstddef>	// Use size_t.
#include <memory>

#include "Loader.h"
#include "ModelCommon.h"

struct ID3D11Device;

namespace Donya
{
	namespace Model
	{

	#pragma region Model

		class Model;

		/// <summary>
		/// Returns an identifier of the made model, or NULL when failed making.<para></para>
		/// The made model will be cached.<para></para>
		/// If set nullptr to "pDevice", use default device.
		/// </summary>
		size_t MakeModel( const Donya::Loader &loadedSource, ModelUsage usage, ID3D11Device *pDevice = nullptr );
		/// <summary>
		/// Returns pointer that point to internal cache directly, or nullptr when passed identifier is not valid.<para></para>
		/// You must not delete the model by it!
		/// </summary>
		const std::unique_ptr<Model> *AcquireRawModel( size_t modelIdentifier );
		/// <summary>
		/// Remove the internally cache of a model.<para></para>
		/// Returns false if passed identifier is not valid.
		/// </summary>
		bool RemoveModelCache( size_t modelidentifier );
		/// <summary>
		/// Remove the all internally caches of model.
		/// </summary>
		void ClearModelCache();

	// region Model
	#pragma endregion

	#pragma region Renderer

		/// <summary>
		/// Returns an identifier of the made renderer, or NULL when failed making.<para></para>
		/// The made renderer will be cached.<para></para>
		/// If set nullptr to "pDevice", use default device.
		/// </summary>
		size_t MakeRenderer( ModelUsage usage, ID3D11Device *pDevice = nullptr );

	// region Renderer
	#pragma endregion

	}
}
