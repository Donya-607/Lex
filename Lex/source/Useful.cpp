#include "Useful.h"

#include <cmath>
#include <crtdbg.h>
#include <d3d11.h>
#include <float.h>
#include <fstream>
#include <locale>
#include <mutex>
#include <Shlwapi.h>	// Use PathRemoveFileSpecA(), PathAddBackslashA(), In AcquireDirectoryFromFullPath().
#include <vector>
#include <Windows.h>

#include "Common.h"
#include "Donya.h"

#pragma comment( lib, "shlwapi.lib" ) // Use PathRemoveFileSpecA(), PathAddBackslashA(), In AcquireDirectoryFromFullPath().

namespace Donya
{
	bool CreateBuffers( ID3D11Buffer **vertexBuffer, unsigned int verticesWholeByteWidth, const void *verticesRawData, ID3D11Buffer **indexBuffer, unsigned int indecesWholeByteWidth, const void *indicesRawData, ID3D11Buffer **constantBuffer, unsigned int sizeofConstantBuffer )
	{
		HRESULT hr = S_OK;
		ID3D11Device *pDevice = Donya::GetDevice();

		// Create VertexBuffer
		if ( vertexBuffer != nullptr )
		{
			if ( verticesRawData == nullptr ) { return false; }

			D3D11_BUFFER_DESC d3d11BufferDesc{};
			d3d11BufferDesc.ByteWidth				= verticesWholeByteWidth;
			d3d11BufferDesc.Usage					= D3D11_USAGE_IMMUTABLE;
			d3d11BufferDesc.BindFlags				= D3D11_BIND_VERTEX_BUFFER;
			d3d11BufferDesc.CPUAccessFlags			= 0;
			d3d11BufferDesc.MiscFlags				= 0;
			d3d11BufferDesc.StructureByteStride		= 0;

			D3D11_SUBRESOURCE_DATA d3d11SubResourceData{};
			d3d11SubResourceData.pSysMem			= verticesRawData;
			d3d11SubResourceData.SysMemPitch		= 0;
			d3d11SubResourceData.SysMemSlicePitch	= 0;

			hr = pDevice->CreateBuffer( &d3d11BufferDesc, &d3d11SubResourceData, vertexBuffer );
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateBuffer()" );
		}
		// Create IndexBuffer
		if ( indexBuffer != nullptr )
		{
			if ( indicesRawData == nullptr ) { return false; }

			D3D11_BUFFER_DESC d3dIndexBufferDesc{};
			d3dIndexBufferDesc.ByteWidth			= indecesWholeByteWidth;
			d3dIndexBufferDesc.Usage				= D3D11_USAGE_IMMUTABLE;
			d3dIndexBufferDesc.BindFlags			= D3D11_BIND_INDEX_BUFFER;
			d3dIndexBufferDesc.CPUAccessFlags		= 0;
			d3dIndexBufferDesc.MiscFlags			= 0;
			d3dIndexBufferDesc.StructureByteStride	= 0;

			D3D11_SUBRESOURCE_DATA d3dSubresourceData{};
			d3dSubresourceData.pSysMem				= indicesRawData;
			d3dSubresourceData.SysMemPitch			= 0;
			d3dSubresourceData.SysMemSlicePitch		= 0;

			hr = pDevice->CreateBuffer( &d3dIndexBufferDesc, &d3dSubresourceData, indexBuffer );
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateBuffer()" );
		}
		// Create ConstantBuffer
		if ( constantBuffer != nullptr )
		{
			D3D11_BUFFER_DESC d3dConstantBufferDesc{};
			d3dConstantBufferDesc.ByteWidth           = sizeofConstantBuffer;
			d3dConstantBufferDesc.Usage               = D3D11_USAGE_DEFAULT;
			d3dConstantBufferDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
			d3dConstantBufferDesc.CPUAccessFlags      = 0;
			d3dConstantBufferDesc.MiscFlags           = 0;
			d3dConstantBufferDesc.StructureByteStride = 0;

			hr = pDevice->CreateBuffer( &d3dConstantBufferDesc, nullptr, constantBuffer );
			_ASSERT_EXPR( SUCCEEDED( hr ), L"Failed : CreateBuffer()" );
		}

		return true;
	}

	bool Equal( float L, float R, float maxRelativeDiff )
	{
	#if		0 // reference is https://marycore.jp/prog/c-lang/compare-floating-point-number/

		return ( fabsf( L - R ) <= maxRelativeDiff * fmaxf( 1.0f, fmaxf( fabsf( L ), fabsf( R ) ) ) ) ? true : false;

	#elif	0 // reference is http://berobemin2.hatenablog.com/entry/2016/02/27/231856

		float diff = fabsf( L - R );
		L = fabsf( L );
		R = fabsf( R );

		float &largest = ( L > R ) ? L : R;
		return ( diff <= largest * maxRelativeDiff ) ? true : false;

	#else // using std::isgreaterequal() and std::islessequal()

		return ( std::isgreaterequal<float>( L, R ) && std::islessequal<float>( L, R ) ) ? true : false;

	#endif
	}

	void OutputDebugStr( const char		*string )
	{
		OutputDebugStringA( string );
	}
	void OutputDebugStr( const wchar_t	*string )
	{
		OutputDebugStringW( string );
	}

	bool IsExistFile( const std::string &wholePath )
	{
		std::ifstream ifs( wholePath );
		return ifs.is_open();
	}
	bool IsExistFile( const std::wstring &wholePath )
	{
		std::wifstream ifs( wholePath );
		return ifs.is_open();
	}

#pragma region Convert Character Functions

#define USE_WIN_API ( true )
#define IS_SETTING_LOCALE_NOW ( false )

	// these convert function referenced to https://nekko1119.hatenablog.com/entry/2017/01/02/054629

	std::wstring	MultiToWide( const std::string	&source, int codePage )
	{
		// MultiByteToWideChar() : http://www.t-net.ne.jp/~cyfis/win_api/sdk/MultiByteToWideChar.html
		const int destSize = MultiByteToWideChar( codePage, 0U, source.data(), -1, nullptr, NULL );
		std::vector<wchar_t> dest( destSize, L'\0' );

		if ( MultiByteToWideChar( codePage, 0U, source.data(), -1, dest.data(), dest.size() ) == 0 )
		{
			throw std::system_error{ scast<int>( GetLastError() ), std::system_category() };
		}
		// else

		dest.resize( std::char_traits<wchar_t>::length( dest.data() ) );
		dest.shrink_to_fit();

		return std::wstring( dest.begin(), dest.end() );
	}
	std::string		WideToMulti( const std::wstring	&source, int codePage )
	{
		// WideCharToMultiByte() : http://www.t-net.ne.jp/~cyfis/win_api/sdk/WideCharToMultiByte.html
		const int destSize = WideCharToMultiByte( codePage, 0U, source.data(), -1, nullptr, NULL, NULL, NULL );
		std::vector<char> dest( destSize, '\0' );

		if ( WideCharToMultiByte( codePage, 0U, source.data(), -1, dest.data(), dest.size(), NULL, NULL ) == 0 )
		{
			throw std::system_error{ scast<int>( ::GetLastError() ), std::system_category() };
		}
		// else

		dest.resize( std::char_traits<char>::length( dest.data() ) );
		dest.shrink_to_fit();

		return std::string( dest.begin(), dest.end() );
	}

	const wchar_t	*MultiToWide( const char			*source )
	{
		return MultiToWide( std::string( source ) ).c_str();
	}
	const char		*WideToMulti( const wchar_t			*source )
	{
		return WideToMulti( std::wstring( source ) ).data();
	}
	std::wstring	MultiToWide( const std::string		&source )
	{
	#if USE_WIN_API

		return MultiToWide( source, CP_ACP );

	#else

		size_t resultLength = 0;
		std::vector<WCHAR> dest( source.size() + 1/*terminate*/, L'\0' );

	#if IS_SETTING_LOCALE_NOW
		errno_t err = _mbstowcs_s_l	// multi byte str to wide char str.
		(
			&resultLength,
			dest.data(),
			dest.size(),
			source.data(),
			_TRUNCATE,
			_create_locale( LC_ALL, "JPN" )
		);
	#else
		errno_t err = mbstowcs_s	// multi byte str to wide char str.
		(
			&resultLength,
			dest.data(),
			dest.size(),
			source.data(),
			_TRUNCATE
		);
	#endif // IS_SETTING_LOCALE_NOW

		if ( err != 0 ) { _ASSERT_EXPR( 0, L"Failed : MultiToWide()" ); };

		dest.resize( std::char_traits<wchar_t>::length( dest.data() ) );
		dest.shrink_to_fit();

		return std::wstring{ dest.begin(), dest.end() };

	#endif // USE_WIN_API
	}
	std::string		WideToMulti( const std::wstring		&source )
	{
	#if USE_WIN_API

		return WideToMulti( source, CP_ACP );

	#else

		size_t resultLength = 0;
		std::vector<char> dest( source.size() * sizeof( wchar_t ) + 1, '\0' );
	#if IS_SETTING_LOCALE_NOW
		errno_t err = _wcstombs_s_l	// wide char str to multi byte str.
		(
			&resultLength,
			dest.data(),
			dest.size(),
			source.data(),
			_TRUNCATE,
			_create_locale( LC_ALL, "JPN" )
		);
	#else
		errno_t err = wcstombs_s	// wide char str to multi byte str.
		(
			&resultLength,
			dest.data(),
			dest.size(),
			source.data(),
			_TRUNCATE
		);
	#endif // IS_SETTING_LOCALE_NOW
		if ( err != 0 ) { _ASSERT_EXPR( 0, L"Failed : WideToMulti()" ); };

		dest.resize( std::char_traits<char>::length( dest.data() ) );
		dest.shrink_to_fit();

		return std::string{ dest.begin(), dest.end() };

	#endif // USE_WIN_API
	}

	std::wstring	UTF8ToWide( const std::string		&source )
	{
		return MultiToWide( source, CP_UTF8 );
	}
	std::string		WideToUTF8( const std::wstring		&source )
	{
		// std::wstring_convert is deprecated in C++17.

		return WideToMulti( source, CP_UTF8 );
	}

	std::string		MultiToUTF8( const std::string &source )
	{
		return WideToUTF8( MultiToWide( source ) );
	}
	std::string		UTF8ToMulti( const std::string &source )
	{
		return WideToMulti( UTF8ToWide( source ) );
	}

#undef IS_SETTING_LOCALE_NOW
#undef USE_WIN_API

#pragma endregion

	std::string ExtractFileDirectoryFromFullPath( std::string fullPath )
	{
		size_t pathLength = fullPath.size();
		if ( !pathLength ) { return ""; }
		// else

		static std::mutex makePathMutex{};
		std::lock_guard<std::mutex> lock( makePathMutex );

		std::unique_ptr<char[]> directory = std::make_unique<char[]>( pathLength );
		for ( size_t i = 0; i < pathLength; ++i )
		{
			directory[i] = fullPath[i];
		}

		PathRemoveFileSpecA( directory.get() );
		PathAddBackslashA( directory.get() );

		return std::string{ directory.get() };
	}

	std::string ExtractFileNameFromFullPath( std::string fullPath )
	{
		const std::string fileDirectory = ExtractFileDirectoryFromFullPath( fullPath );
		if ( fileDirectory.empty() ) { return ""; }
		// else

		return fullPath.substr( fileDirectory.size() );
	}
}