#pragma once

#include <memory>

#include "Template.h"

namespace Donya
{
	class Random : Donya::Singleton<Random>
	{
		// HACK : Is this singleton necessary ?

		friend Donya::Singleton<Random>;

		// TODO:I want implement this :
		// http://marupeke296.com/TIPS_No18_Randoms.html
		// http://marupeke296.com/TIPS_No16_flaotrandom.html

	private:
		class Impl;
		std::unique_ptr<Impl> impl;
	private:
		Random();
	public:
		virtual ~Random();
		Random( const Random & ) = delete;
		Random & operator = ( const Random & ) = delete;
	public:
		unsigned int	_Int	()							const; // Returns 0 ~ std::random_device::max().
		unsigned int	_Int	( int max )					const; // Returns 0 ~ ( max - 1 ).
		int				_Int	( int min, int max )		const; // Returns min  ~ ( max - 1 ).
		float			_Float	()							const; // Returns 0.0f ~ 1.0f.
		float			_Float	( float max )				const; // Returns 0.0f ~ max.
		float			_Float	( float min, float max )	const; // Returns 0.0f ~ 1.0f.
	public:
		/// <summary>
		/// Returns 0 ~ std::random_device::max()
		/// </summary>
		inline static unsigned int	GenerateInt() { return Get()._Int(); }
		/// <summary>
		/// Returns 0  ~ ( max - 1 ).
		/// </summary>
		inline static unsigned int	GenerateInt( int max ) { return Get()._Int( max ); }
		/// <summary>
		/// Returns min  ~ ( max - 1 ).
		/// </summary>
		inline static int			GenerateInt( int min, int max ) { return Get()._Int( min, max ); }
		/// <summary>
		/// Returns 0.0f ~ 1.0f.
		/// </summary>
		inline static float			GenerateFloat() { return Get()._Float(); }
		/// <summary>
		/// Returns 0.0f ~ max.
		/// </summary>
		inline static float			GenerateFloat( float max ) { return Get()._Float( max ); }
		/// <summary>
		/// Returns min = max.
		/// </summary>
		inline static float			GenerateFloat( float min, float max ) { return Get()._Float( min, max ); }
	private:
		template<typename ReturnType>
		inline static ReturnType	ChooseImpl() { return ReturnType(); }

		template<typename ReturnType, typename First, typename... Rest>
		inline static ReturnType	ChooseImpl( size_t randIndex, const First &first, const Rest &... rest )
		{
			return ( !randIndex ) ? first : Get().ChooseImpl<ReturnType>( randIndex - 1, rest... );
		}
	};

	template<typename ReturnType, typename First, typename... Rest>
	inline ReturnType Choose( const First &first, const Rest &... rest )
	{
		size_t randIndex = Random::GenerateInt( sizeof...( rest ) );
		return ( !randIndex ) ? first : Random::ChooseImpl<ReturnType>( randIndex - 1, rest... );
	}
}
