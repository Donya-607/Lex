#include "ModelMaker.h"

#include <d3d11.h>
#include <string>
#include <memory>
#include <unordered_map>

#include "Donya/Donya.h"	// Use GetDevice().
#include "Donya/Useful.h"	// Use CombineHash().

#include "Model.h"

namespace Donya
{
	static std::unordered_map<size_t, std::unique_ptr<Model>> modelMap{};

	size_t MakeHash( const Donya::Loader &loader, ModelUsage usage )
	{
		size_t first	= std::hash<std::string>{}( loader.GetAbsoluteFilePath() );
		size_t second	= std::hash<ModelUsage> {}( usage );

		return Donya::CombineHash( first, second );
	}

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

		Model model{ loader.TmpModelSource(), usage, pDevice };
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
	void ClearAllModelCache()
	{
		modelMap.clear();
	}
}
