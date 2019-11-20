#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Vector.h" // Use Donya::Int2.

constexpr float PI		= 3.14159265359f;
constexpr float EPSILON	= 1.192092896e-07F;	// FLT_EPSILON. smallest such that 1.0+FLT_EPSILON != 1.0

template<typename T, size_t size>
constexpr size_t ArraySize( const T( & )[size] ) { return size; }

constexpr float ToRadian( float degree	) { return degree *  0.01745f/* PI / 180 */;	}
constexpr float ToDegree( float radian	) { return radian * 57.29577f/* 180 / PI */;	}

constexpr bool ZeroEqual( float x		) { return ( -EPSILON < x && x < EPSILON );		}

namespace Donya
{
	/// <summary>
	/// For example, if you pass 316 to CountBits(), returns 5.<para></para>
	/// ( 316 is 100111100, number of 1 of 100111100 has is 5 )
	/// </summary>
	constexpr int CountBits( int integer )
	{
		// see http://www.mwsoft.jp/programming/java/java_lang_integer_bit_count.html

		integer = integer - ( ( integer >> 1 ) & 0x55555555 );
		integer = ( integer & 0x33333333 ) + ( ( integer >> 2 ) & 0x33333333 );
		integer = ( integer + ( integer >> 4 ) ) & 0x0f0f0f0f;
		integer = integer + ( integer >> 8 );
		integer = integer + ( integer >> 16 );
		return integer & 0x3f;
	}

	/// <summary>
	/// Returns:<para></para>
	/// value ~ 0 : -1, <para></para>
	/// value = 0 : 0, <para></para>
	/// 0 ~ value : 1.
	/// </summary>
	constexpr int SignBit( int value )
	{
		return	( !value )
				? 0
				: ( ( value < 0 ) ? -1 : 1 );
	}
	/// <summary>
	/// Returns:<para></para>
	/// value ~ 0 : -1, <para></para>
	/// value = 0 : 0, <para></para>
	/// 0 ~ value : 1.
	/// </summary>
	constexpr int SignBit( float value )
	{
		return	( ZeroEqual( value ) )
				? 0
				: ( ( value < 0.0f ) ? -1 : 1 );
	}

	/// <summary>
	/// Returns [-pi ~ +pi].
	/// </summary>
	float NormalizeRadian( float radian );
	/// <summary>
	/// Returns [-180.0f ~ +180.0f].
	/// </summary>
	float NormalizeDegree( float degree );

	bool Equal( float L, float R, float maxRelativeDiff = EPSILON );

	/// <summary>
	/// Wrapper of OutputDebugStringA().
	/// </summary>
	void OutputDebugStr( const char		*string );
	/// <summary>
	/// Wrapper of OutputDebugStringW().
	/// </summary>
	void OutputDebugStr( const wchar_t	*string );

	bool IsExistFile( const std::string &wholePath );
	bool IsExistFile( const std::wstring &wholePath );

	/// <summary>
	/// Store specify numbers per digits.<para></para>
	/// If the number does not have enough digit, filled by zero.<para></para>
	/// If set value of less equal zero to "storeDigits", returns digits is decided by value.<para></para>
	/// If you call with ( 26, 4 ), returns array : [0]:6, [1]:2, [2]:0, [3]:0.<para></para>
	/// If you call with ( 12345, 4 ), returns array : [0]:5, [1]:4, [2]:3, [3]:2.<para></para>
	/// If you call with ( 2345, -1 ), returns array : [0]:5, [1]:4, [2]:3, [3]:2.
	/// </summary>
	std::vector<unsigned int> SeparateDigits( unsigned int value, int storeDigits = -1 );

#pragma region Convert Character Functions

	/// <summary>
	/// Convert char of Shift_JIS( ANSI ) to wchar_t.
	/// </summary>
	const wchar_t	*MultiToWide( const char		*source );
	/// <summary>
	/// Convert wchar_t to char of Shift_JIS( ANSI ).
	/// </summary>
	const char		*WideToMulti( const wchar_t		*source );
	/// <summary>
	/// Convert std::string( Shift_JIS( ANSI ) ) to std::wstring.
	/// </summary>
	std::wstring	MultiToWide( const std::string	&source );
	/// <summary>
	/// Convert std::wstring to std::string( Shift_JIS( ANSI ) ).
	/// </summary>
	std::string		WideToMulti( const std::wstring	&source );

	/// <summary>
	/// Convert std::string( UTF-8 ) to std::wstring.
	/// </summary>
	std::wstring	UTF8ToWide( const std::string	&source );
	/// <summary>
	/// Convert std::wstring to std::string( UTF-8 ).
	/// </summary>
	std::string		WideToUTF8( const std::wstring	&source );

	/// <summary>
	/// Convert std::string( UTF-8 ) to std::string( Shift_JIS( ANSI ) ).
	/// </summary>
	std::string		MultiToUTF8( const std::string &source );
	/// <summary>
	/// Convert std::string( Shift_JIS( ANSI ) ). std::string( UTF-8 ).
	/// </summary>
	std::string		UTF8ToMulti( const std::string &source );

#pragma endregion

	std::string ToFullPath( std::string filePath );

	/// <summary>
	/// If fullPath is invalid, returns ""(You can error-check with std::string::empty()).
	/// </summary>
	std::string ExtractFileDirectoryFromFullPath( std::string fullPath );
	/// <summary>
	/// If fullPath is invalid, returns ""(You can error-check with std::string::empty()).
	/// </summary>
	std::wstring ExtractFileDirectoryFromFullPath( std::wstring fullPath );
	/// <summary>
	/// If fullPath is invalid, returns ""(You can error-check with std::string::empty());
	/// </summary>
	std::string ExtractFileNameFromFullPath( std::string fullPath );
	/// <summary>
	/// If fullPath is invalid, returns ""(You can error-check with std::string::empty());
	/// </summary>
	std::wstring ExtractFileNameFromFullPath( std::wstring fullPath );
}
