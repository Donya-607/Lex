#pragma once

#include <string>

#include "Looper.h"
#include "Vector.h"

namespace Donya
{
	namespace Sprite
	{
		/// <summary>
		/// This class provide helper methods of Donya::Sprite.
		/// </summary>
		class Sheet
		{
		private:
			size_t			sprId;
			Donya::Int2		sheetSize; // Whole-size.
			Donya::Looper	anime;
		public:
			Sheet();
			virtual ~Sheet();
		public:
			/// <summary>
			/// This will be reset loaded sprite data.
			/// </summary>
			void Init();

			/// <summary>
			/// Returns false if load failed.<para></para>
			/// "maxInstanceCount" is max count of concurrently drawable.
			/// </summary>
			bool Load( std::wstring filePath, size_t maxInstanceCount = 32U );
		public:
			/// <summary>
			/// Returns ID is can pass to Donnya::Sprite::Draw functions.
			/// </summary>
			size_t GetIdentifier() const { return sprId; }

			/// <summary>
			/// Returns whole-size of sprite.
			/// </summary>
			Donya::Int2 GetSpriteSize() const { return sheetSize; }

			/// <summary>
			/// Returns reference of animation.<para></para>
			/// If you use animation, please call this and update every frame.
			/// </summary>
			Donya::Looper &GetAnimeRef();

			/// <summary>
			/// Returns 0-based coordinate of part of sprite.<para></para>
			/// We expect argument : The index is 0-based, The partCount is 1-based.<para></para>
			/// We can't handle negative value.<para></para>
			/// If we received unexpected argument, returns { 0, 0 }.
			/// </summary>
			Donya::Int2 CalcPartCoord( unsigned int index0Based, Donya::Int2 partCount1Based ) const;
		};
	}
}
