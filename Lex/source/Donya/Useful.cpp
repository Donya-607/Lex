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

#include "Constant.h"
#include "Donya.h"

#pragma comment( lib, "shlwapi.lib" ) // Use PathRemoveFileSpecA(), PathAddBackslashA(), In AcquireDirectoryFromFullPath().

namespace Donya
{
	float NormalizeRadian( float radian )
	{
		// http://hima-tubusi.blogspot.com/2016/12/blog-post_12.html
		return atan2f( sinf( radian ), cosf( radian ) );
	}
	float NormalizeDegree( float degree )
	{
		return ToDegree( NormalizeRadian( ToRadian( degree ) ) );
	}

	bool Equal( float L, float R, float maxRelativeDiff )
	{
	#if		0 // see https://marycore.jp/prog/c-lang/compare-floating-point-number/

		return ( fabsf( L - R ) <= maxRelativeDiff * fmaxf( 1.0f, fmaxf( fabsf( L ), fabsf( R ) ) ) ) ? true : false;

	#elif	0 // see http://berobemin2.hatenablog.com/entry/2016/02/27/231856

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
	#if DEBUG_MODE
		OutputDebugStringA( string );
	#endif // DEBUG_MODE
	}
	void OutputDebugStr( const wchar_t	*string )
	{
	#if DEBUG_MODE
		OutputDebugStringW( string );
	#endif // DEBUG_MODE
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

	std::vector<unsigned int> SeparateDigits( unsigned int value, int storeDigits )
	{
		const int MAX_DIGIT = ( storeDigits <= 0 ) ? INT_MAX : storeDigits;

		std::vector<unsigned int> digits{};
		if ( 0 < storeDigits )
		{
			digits.reserve( storeDigits );
		}

		int i = 0;
		while ( 0 < value && i < MAX_DIGIT )
		{
			digits.emplace_back( value % 10 );
			value /= 10;

			i++;
		}

		while ( i < storeDigits )
		{
			digits.emplace_back( value % 10 );
			i++;
		}

		return digits;
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

	std::string		MultiToUTF8( const std::string		&source )
	{
		return WideToUTF8( MultiToWide( source ) );
	}
	std::string		UTF8ToMulti( const std::string		&source )
	{
		return WideToMulti( UTF8ToWide( source ) );
	}

#undef IS_SETTING_LOCALE_NOW
#undef USE_WIN_API

#pragma endregion

	static std::mutex mutexFullPathName{};

	std::string  ToFullPath( std::string filePath )
	{
		// GetFullPathName() is thread unsafe.
		// Because a current directory always have possibility will be changed by another thread.
		// See https://stackoverflow.com/questions/54814130/is-there-a-thread-safe-version-of-getfullpathname

		std::lock_guard<std::mutex> enterCS( mutexFullPathName );

		auto bufferSize = GetFullPathNameA( filePath.c_str(), NULL, NULL, nullptr );
		std::unique_ptr<char[]> buffer = std::make_unique<char[]>( bufferSize );

		/* auto result = */GetFullPathNameA( filePath.c_str(), bufferSize, buffer.get(), nullptr );

		return std::string{ buffer.get() };
	}

	std::string  ExtractFileDirectoryFromFullPath( std::string fullPath )
	{
		size_t pathLength = fullPath.size();
		if ( !pathLength ) { return ""; }
		// else

		std::unique_ptr<char[]> directory = std::make_unique<char[]>( pathLength );
		for ( size_t i = 0; i < pathLength; ++i )
		{
			directory[i] = fullPath[i];
		}

		PathRemoveFileSpecA( directory.get() );
		PathAddBackslashA( directory.get() );

		return std::string{ directory.get() };
	}
	std::wstring ExtractFileDirectoryFromFullPath( std::wstring fullPath )
	{
		return
		MultiToWide
		(
			ExtractFileDirectoryFromFullPath
			(
				WideToMulti( fullPath )
			)
		);
	}

	std::string  ExtractFileNameFromFullPath( std::string fullPath )
	{
		const std::string fileDirectory = ExtractFileDirectoryFromFullPath( fullPath );
		if ( fileDirectory.empty() ) { return ""; }
		// else

		return fullPath.substr( fileDirectory.size() );
	}
	std::wstring ExtractFileNameFromFullPath( std::wstring fullPath )
	{
		return
		MultiToWide
		(
			ExtractFileNameFromFullPath
			(
				WideToMulti( fullPath )
			)
		);
	}
}