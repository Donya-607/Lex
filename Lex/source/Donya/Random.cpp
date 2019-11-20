#include "Random.h"

#include <random>

#include "Constant.h"
#include "Useful.h"

#undef min
#undef max

void Swap( int *L, int *R )
{
	if ( !L || !R ) { return; }
	// else

	auto tmp = *L;
	*L = *R;
	*R = tmp;
}
void Swap( float *L, float *R )
{
	if ( !L || !R ) { return; }
	// else

	auto tmp = *L;
	*L = *R;
	*R = tmp;
}

namespace Donya
{

	class Random::Impl
	{
	public:
		std::random_device	rd;
		std::mt19937		mt;
	public:
		Impl() : rd(), mt( rd() ) {}
	};

	Random::Random() : impl( std::make_unique<Random::Impl>() ) {}
	Random::~Random() = default;

	unsigned int	Random::_Int()							const
	{
		return impl->mt();
	}
	unsigned int	Random::_Int( int max )					const
	{
		if ( max == 0 ) { return 0; }
		return _Int() % max;
	}
	int				Random::_Int( int min, int max )		const
	{
		if ( max <= min ) { Swap( &min, &max ); }
		return _Int( max ) + min;
	}
	float			Random::_Float()						const
	{
		return scast<float>( impl->mt() ) / impl->mt.max();
	}
	float			Random::_Float( float max )				const
	{
		if ( ZeroEqual( max ) ) { return 0.0f; }
		return _Float() * max;
	}
	float			Random::_Float( float min, float max )	const
	{
		if ( max <= min ) { Swap( &min, &max ); }
		return min + ( _Float() * ( max - min ) );
	}
}