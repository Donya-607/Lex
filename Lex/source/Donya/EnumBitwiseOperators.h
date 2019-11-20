#ifndef INCLUDED_ENUM_BITWISE_OPERATORS_H_
#define INCLUDED_ENUM_BITWISE_OPERATORS_H_

#include <type_traits>

#if 1

// by https://jumble-note.blogspot.com/2017/12/c.html

/// <summary>
/// Provides bitwise operations.
/// </summary>
#define ENUM_ATTR_BITFLAG( T )													\
	constexpr T operator | (const T lhs, const T rhs)							\
	{																			\
		using U = typename std::underlying_type<T>::type;						\
		return static_cast<T>(static_cast<U>(lhs) | static_cast<U>(rhs));		\
	}																			\
																				\
	constexpr T operator & (const T lhs, const T rhs)							\
	{																			\
		using U = typename std::underlying_type<T>::type;						\
		return static_cast<T>(static_cast<U>(lhs) & static_cast<U>(rhs));		\
	}																			\
																				\
	constexpr T operator ^ (const T lhs, const T rhs)							\
	{																			\
		using U = typename std::underlying_type<T>::type;						\
		return static_cast<T>(static_cast<U>(lhs) ^ static_cast<U>(rhs));		\
	}																			\
																				\
	constexpr T operator ~ (const T val)										\
	{																			\
		using U = typename std::underlying_type<T>::type;						\
		return static_cast<T>(~static_cast<U>(val));							\
	}																			\
																				\
	inline T& operator |= (T& lhs, const T& rhs)								\
	{																			\
		using U = typename std::underlying_type<T>::type;						\
		return lhs = static_cast<T>(static_cast<U>(lhs) | static_cast<U>(rhs));	\
	}																			\
																				\
	inline T& operator &= (T& lhs, const T& rhs)								\
	{																			\
		using U = typename std::underlying_type<T>::type;						\
		return lhs = static_cast<T>(static_cast<U>(lhs) & static_cast<U>(rhs));	\
	}																			\
																				\
	inline T& operator ^= (T& lhs, const T& rhs)								\
	{																			\
		using U = typename std::underlying_type<T>::type;						\
		return lhs = static_cast<T>(static_cast<U>(lhs) ^ static_cast<U>(rhs));	\
	}

#else

// by https://qiita.com/akinomyoga/items/a5df34fc78efd86fa4ef

namespace Donya
{
	template<typename Enum, typename T = Enum>
	struct ProvidesBitwiseOperators : std::false_type {};

	namespace ProvidesBitwiseOperatorsDetail
	{
		enum class fallback_enum {};

		template<typename T> using safe_underlying_t = typename std::conditional<
			!std::is_enum<T>::value, T,
			typename std::underlying_type<
			typename std::conditional<std::is_enum<T>::value, T, fallback_enum>::type>::type>::type;

		template<typename T1, typename T2, bool ForceLHS = false>
		using result_t = typename std::enable_if<
			( ProvidesBitwiseOperators<T1, T2>::value || ProvidesBitwiseOperators<T2, T1>::value ) &&
			std::is_same<safe_underlying_t<T1>, safe_underlying_t<T2>>::value,
			typename std::conditional<ForceLHS || std::is_enum<T1>::value, T1, T2>::type>::type;

		template<typename T>
		constexpr safe_underlying_t<T> peel( T flags ) { return static_cast<safe_underlying_t<T>>( flags ); }

		template<typename T>
		constexpr result_t<T, T>	operator ~ ( T flags ) { return static_cast<T>( ~peel( flags ) ); }
		template<typename T1, typename T2>
		constexpr result_t<T1, T2>	operator | ( T1 flags1, T2 flags2 )
		{
			return result_t<T1, T2>( peel( flags1 ) | peel( flags2 ) );
		}
		template<typename T1, typename T2>
		constexpr result_t<T1, T2>	operator & ( T1 flags1, T2 flags2 )
		{
			return result_t<T1, T2>( peel( flags1 ) & peel( flags2 ) );
		}
		template<typename T1, typename T2>
		constexpr result_t<T1, T2>	operator ^ ( T1 flags1, T2 flags2 )
		{
			return result_t<T1, T2>( peel( flags1 ) ^ peel( flags2 ) );
		}
		template<typename T1, typename T2>
		constexpr result_t<T1, T2, true> &	operator |= ( T1& flags1, T2 flags2 )
		{
			return flags1 = static_cast<T1>( flags1 | flags2 );
		}
		template<typename T1, typename T2>
		constexpr result_t<T1, T2, true> &	operator &= ( T1& flags1, T2 flags2 )
		{
			return flags1 = static_cast<T1>( flags1 & flags2 );
		}
		template<typename T1, typename T2>
		constexpr result_t<T1, T2, true> &	operator ^= ( T1& flags1, T2 flags2 )
		{
			return flags1 = static_cast<T1>( flags1 ^ flags2 );
		}
	}
}

using Donya::ProvidesBitwiseOperatorsDetail::operator ~;
using Donya::ProvidesBitwiseOperatorsDetail::operator |;
using Donya::ProvidesBitwiseOperatorsDetail::operator &;
using Donya::ProvidesBitwiseOperatorsDetail::operator ^;
using Donya::ProvidesBitwiseOperatorsDetail::operator |=;
using Donya::ProvidesBitwiseOperatorsDetail::operator &=;
using Donya::ProvidesBitwiseOperatorsDetail::operator ^=;

#endif // 1

#endif //INCLUDED_ENUM_BITWISE_OPERATORS_H_