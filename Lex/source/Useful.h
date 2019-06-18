#ifndef _INCLUDED_USEFUL_H_
#define _INCLUDED_USEFUL_H_

#include <string>

struct ID3D11Buffer;

namespace Donya
{
	/// <summary>
	/// I doing ID3D11Device::CreateBuffer to vertex buffer, index buffer, constant buffer.<para></para>
	/// You can setting nullptr in ID3D11Buffer. in that case, I skip CreateBuffer.<para></para>
	/// If I can't created, returns false.
	/// </summary>
	bool CreateBuffers
	(
		ID3D11Buffer **vertexBuffer,	unsigned int verticesWholeByteWidth,	const void *verticesRawData,
		ID3D11Buffer **indexBuffer,		unsigned int indecesWholeByteWidth,		const void *indicesRawData,
		ID3D11Buffer **constantBuffer,	unsigned int sizeofConstantBuffer
	);

	bool Equal( float L, float R, float maxRelativeDiff );

	/// <summary>
	/// Wrapper of OutputDebugStringA().
	/// </summary>
	void OutputDebugStr( const char		*string );
	/// <summary>
	/// Wrapper of OutputDebugStringW().
	/// </summary>
	void OutputDebugStr( const wchar_t	*string );

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

#pragma endregion

}

#endif // _INCLUDED_USEFUL_H_