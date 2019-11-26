#include "Color.h"

#include "Blend.h"

namespace Donya
{
	namespace Color
	{
		float FilteringAlpha( float alpha )
	{
		if ( !Donya::Blend::IsEnabledATC() ) { return alpha; }
		// else

		// ( 0.0f ~ 1.0f ) to ( 0.5f ~ 1.0f ).

		alpha *= 0.5f;
		alpha += 0.5f;
		return alpha;
	}
	}
}
