#pragma once

#include "Vector.h"

namespace Donya
{
	namespace Color
	{
		enum class Code : int
		{
			AQUA		= 0x00FFFF,	// #00FFFF
			BLACK		= 0x000000,	// #000000
			BLUE		= 0x0000FF,	// #0000FF
			CYAN		= AQUA,		// #00FFFF
			DARK_GRAY	= 0x575757,	// #575757
			FUCHSIA		= 0xCC1669,	// #CC1669
			GRAY		= 0x808080,	// #808080
			GREEN		= 0x008000,	// #008000
			LIME		= 0x00FF00,	// #00FF00
			LIGHT_GRAY	= 0xD3D3D3,	// #D3D3D3
			MAGENTA		= 0xFF00FF,	// #FF00FF
			MAROON		= 0x800000,	// #800000
			NAVY		= 0x1F2F54,	// #1F2F54
			OLIVE		= 0x808000,	// #808000
			ORANGE		= 0xF39800,	// #F39800
			PURPLE		= 0xA758A8,	// #A758A8
			RED			= 0xFF0000,	// #FF0000
			SILVER		= 0xC0C0C0,	// #C0C0C0
			TEAL		= 0x006956,	// #006956
			WHITE		= 0xFFFFFF,	// #FFFFFF
			YELLOW		= 0xFFFF00,	// #FFFF00
		};

		static constexpr Donya::Vector3 MakeColor( int  color )
		{
			return Donya::Vector3
			{
				static_cast<float>( ( color >> 16 ) & 0xff ) / 255.0f,
				static_cast<float>( ( color >> 8  ) & 0xff ) / 255.0f,
				static_cast<float>( ( color >> 0  ) & 0xff ) / 255.0f,
			};
		}
		static constexpr Donya::Vector3 MakeColor( Code color )
		{
			return MakeColor( static_cast<int>( color ) );
		}

		/// <summary>
		/// Filtering the alpha( 0.0f ~ 1.0f ) by current blend mode.
		/// </summary>
		float FilteringAlpha( float alpha );
	}
}
