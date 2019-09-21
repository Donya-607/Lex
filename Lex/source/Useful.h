#pragma once

#include <string>

struct ID3D11Buffer;

namespace Donya
{
	bool Equal( float L, float R, float maxRelativeDiff = 1.192092896e-07F/* FLT_EPSILON */ );

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

	/// <summary>
	/// If fullPath is invalid, returns ""(You can error-check with std::string::empty());
	/// </summary>
	std::string ExtractFileDirectoryFromFullPath( std::string fullPath );
	/// <summary>
	/// If fullPath is invalid, returns ""(You can error-check with std::string::empty());
	/// </summary>
	std::string ExtractFileNameFromFullPath( std::string fullPath );
}
