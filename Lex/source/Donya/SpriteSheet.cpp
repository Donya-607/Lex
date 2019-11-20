#include "SpriteSheet.h"

#include <algorithm> // Use std::max(), std::min().

#include "Constant.h"
#include "Sprite.h"

#undef max
#undef min

namespace Donya
{
	namespace Sprite
	{
		Sheet::Sheet() :
			sprId( NULL ),
			sheetSize(),
			anime()
		{

		}
		Sheet::~Sheet() = default;

		void Sheet::Init()
		{
			sprId		= NULL;
			sheetSize	= {};
			anime.SetCurrentIndex( 0, 0 );
			anime.SetRange( 0, 0 );
			anime.SetRegulation( true, false );
			anime.SetWaitFrame( -1 );
		}

		bool Sheet::Load( std::wstring filePath, size_t maxInstanceCount )
		{
			sprId = Donya::Sprite::Load( filePath, maxInstanceCount );

			// If load failed, the "sprId" will NULL,
			// and this GetTextureSize() is returns false if send to NULL.
			// so We can check result of load.
			bool result = Donya::Sprite::GetTextureSize
			(
				sprId,
				&sheetSize.x,
				&sheetSize.y
			);

			return result;
		}

		Donya::Looper &Sheet::GetAnimeRef()
		{
			return anime;
		}

		Donya::Int2 Sheet::CalcPartCoord( unsigned int index, Donya::Int2 partCount ) const
		{
			// We can't handle negative value.
			partCount.x = std::max( 0, partCount.x );
			partCount.y = std::max( 0, partCount.y );

			// The "maxCount" will be 0 if the partCount's x or y is 0.
			// In that case, We will return by the next if-statement.
			// This prevent zero divide.

			int maxCount = partCount.x * partCount.y;
			if ( scast<unsigned int>( maxCount ) <= index ) { return Donya::Int2{}; }

			int x = index % partCount.x;
			int y = ( partCount.y == 1 ) ? 0 : scast<int>( index / partCount.x );

			return Donya::Int2{ x, y };
		}

	}
}
