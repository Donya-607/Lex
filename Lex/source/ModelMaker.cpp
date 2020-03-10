#include "ModelMaker.h"

#include <d3d11.h>
#include <string>
#include <memory>
#include <unordered_map>

#include "Donya/Donya.h"	// Use GetDevice().
#include "Donya/Useful.h"	// Use CombineHash().

#include "Model.h"
#include "ModelRenderer.h"

namespace Donya
{
	namespace Model
	{
		size_t MakeHash( ModelUsage usage )
		{
			return std::hash<ModelUsage>{}( usage );
		}
		size_t MakeHash( const Donya::Loader &loader, ModelUsage usage )
		{
			size_t first	= std::hash<std::string>{}( loader.GetAbsoluteFilePath() );
			size_t second	= MakeHash( usage );

			return Donya::CombineHash( first, second );
		}

	#pragma region Model

		static std::unordered_map<size_t, std::unique_ptr<Model>> modelMap{};

		size_t MakeModel( const Donya::Loader &loader, ModelUsage usage, ID3D11Device *pDevice )
		{
			if ( !pDevice )
			{
				pDevice = Donya::GetDevice();
			}

			const size_t hash = MakeHash( loader, usage );

			if ( modelMap.find( hash ) != modelMap.end() )
			{
				return hash;
			}
			// else

			Model model{ loader.GetModelSource(), loader.GetFileDirectory(), usage, pDevice };
			modelMap.insert
			(
				std::make_pair
				(
					hash,
					std::make_unique<Model>( std::move( model ) )
				)
			);

			return hash;
		}

		const std::unique_ptr<Model> *AcquireRawModel( size_t id )
		{
			const auto found = modelMap.find( id );
			if ( found == modelMap.end() )
			{
				return nullptr;
			}
			// else

			return &found->second;
		}

		bool RemoveModelCache( size_t id )
		{
			const auto found = modelMap.find( id );
			if ( found == modelMap.end() )
			{
				return false;
			}
			// else

			modelMap.erase( found );

			return true;
		}
		void ClearModelCache()
		{
			modelMap.clear();
		}

	// region Model
	#pragma endregion

	#pragma region Renderer

		static std::unordered_map<size_t, std::unique_ptr<Renderer>> rendererMap{};

		size_t MakeRenderer( ModelUsage usage, ID3D11Device *pDevice )
		{
			if ( !pDevice )
			{
				pDevice = Donya::GetDevice();
			}

			const size_t hash = MakeHash( usage );

			if ( rendererMap.find( hash ) != rendererMap.end() )
			{
				return hash;
			}
			// else

			Renderer renderer{ usage, pDevice };
			rendererMap.insert
			(
				std::make_pair
				(
					hash,
					std::make_unique<Renderer>( std::move( renderer ) )
				)
			);

			return hash;
		}

		const std::unique_ptr<Renderer> *AcquireRawRenderer( size_t id )
		{
			const auto found = rendererMap.find( id );
			if ( found == rendererMap.end() )
			{
				return nullptr;
			}
			// else

			return &found->second;
		}

		bool RemoveRendererCache( size_t id )
		{
			const auto found = rendererMap.find( id );
			if ( found == rendererMap.end() )
			{
				return false;
			}
			// else

			rendererMap.erase( found );

			return true;
		}
		void ClearRendererCache()
		{
			rendererMap.clear();
		}

	// region Renderer
	#pragma endregion

	}
}
