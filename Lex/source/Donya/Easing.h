#pragma once

#include <array>

#define _USE_MATH_DEFINES
#include <math.h>

namespace Donya
{
	namespace Easing
	{
		enum class Kind
		{
			Linear,

			Back,
			Bounce,
			Circular,
			Cubic,
			Elastic,
			Exponential,
			Quadratic,
			Quartic,
			Quintic,
			Smooth,
			Sinusoidal,
			SoftBack,
			Step,

			ENUM_TERMINATION
		};

		/// <summary>
		/// constexpr version.
		/// </summary>
		enum class CKind
		{
			BACK,
			CUBIC,
			ELASTIC,
			QUADRATIC,
			QUARTIC,
			QUINTIC,
			SMOOTH,
			SOFTBACK,
			STEP,

			ENUM_TERMINATION
		};

		/// <summary>
		/// [In : Accelerating from zero velocity.]<para></para>
		/// [Out : Decelerating from zero velocity.]<para></para>
		/// [InOut : Acceleration until halfway, then deceleration.]
		/// </summary>
		enum class Type
		{
			In,		// Accelerating from zero velocity.
			Out,	// Decelerating from zero velocity.
			InOut	// Acceleration until halfway, then deceleration.
		};

		/// <summary>
		/// These ease functions returns value is ease-in.
		/// </summary>
		namespace Impl
		{
			// see http://gizma.com/easing/#l, https://trap.jp/post/162/

			static constexpr float AdjustTimeIn( float t )
			{
				return t;
			}
			static constexpr float AdjustTimeOut( float t )
			{
				return 1.0f - t;
			}
			static constexpr float AdjustTimeInOut( float t )
			{
				return ( t < 0.5f )
				? 2.0f * t
				: 2.0f - ( 2.0f * t );
			}
			static constexpr float AdjustTime( Type type, float t )
			{
				if ( type == Type::In )
				{
					return AdjustTimeIn( t );
				}
				// else
				if ( type == Type::Out )
				{
					return AdjustTimeOut( t );
				}
				// else
				return AdjustTimeInOut( t );
			}

			static constexpr float AdjustResultIn( float result )
			{
				return result;
			}
			static constexpr float AdjustResultOut( float result )
			{
				return 1.0f - result;
			}
			static constexpr float AdjustResultInOut( float rawTime, float result )
			{
				return ( rawTime < 0.5f )
					? result * 0.5f
					: 1.0f - ( result * 0.5f );
			}
			static constexpr float AdjustResult( Type type, float rawTime, float result )
			{
				if ( type == Type::In )
				{
					return AdjustResultIn( result );
				}
				// else
				if ( type == Type::Out )
				{
					return AdjustResultOut( result );
				}
				// else
				return AdjustResultInOut( rawTime, result );
			}

			static constexpr float Linear( float t )
			{
				return t;
			}

			static constexpr float Back( float t )
			{
				return t * t * ( 2.70158f * t - 1.70158f );
			}
			static constexpr float Cubic( float t )
			{
				return t * t * t;
			}
			static constexpr float Elastic( float t )
			{
				return
				   56.0f * t * t * t * t * t
				- 105.0f * t * t * t * t
				+  60.0f * t * t * t
				-  10.0f * t * t
				;
			}
			static constexpr float Quadratic( float t )
			{
				return t * t;
			}
			static constexpr float Quartic( float t )
			{
				return t * t * t * t;
			}
			static constexpr float Quintic( float t )
			{
				return t * t * t * t * t;
			}
			static constexpr float Smooth( float t )
			{
				return t * t * ( 3.0f - t ) * 0.5f;
			}
			static constexpr float SoftBack( float t )
			{
				return t * t * ( 2.0f * t - 1.0f );
			}
			static constexpr float Step( float t )
			{
				constexpr float INCREMENT = 0.1f;
				int i = 0;
				for ( ; INCREMENT * i < t; ++i ) {}

				return INCREMENT * i;
			}

			static constexpr float PI_2_F = static_cast<float>( M_PI_2 );
			static float Bounce( float t )
			{
				// see https://trap.jp/post/162/

				float pow{}, bounce = 4.0f, condition = pow + 1.0f;
				while ( t <= condition )
				{
					bounce -= 1.0f;
					pow = powf( 2.0f, bounce );
					condition = ( pow - 1.0f ) / 11.0f;
				}

				return
				1.0f /
				powf( 4.0f, 3.0f - bounce ) -
				7.5625f * powf( ( pow * 3.0f - 2.0f ) / 22.0f - t, 2.0f );
			}
			static float Circular( float t )
			{
				return 1.0f - sqrtf( fmaxf( 0.0f, 1.0f - t * t ) );
			}
			static float Exponential( float t )
			{
				return powf( 2.0f, -( 1.0f - t ) * 10.0f );
			}
			static float Sinusoidal( float t )
			{
				return 1.0f - cosf( t * PI_2_F );
			}
		}

		static constexpr int GetKindCount()
		{
			return static_cast<int>( Kind::ENUM_TERMINATION );
		}
		static constexpr int GetTypeCount()
		{
			return 3; // static_cast<int>( Type::InOut ) + 1;
		}
		static constexpr const char *KindName( int easingKind )
		{
			switch ( static_cast<Kind>( easingKind ) )
			{
			case Kind::Linear:			return "Linear";
			case Kind::Back:			return "Back";
			case Kind::Bounce:			return "Bounce";
			case Kind::Circular:		return "Circular";
			case Kind::Cubic:			return "Cubic";
			case Kind::Elastic:			return "Elastic";
			case Kind::Exponential:		return "Exponential";
			case Kind::Quadratic:		return "Quadratic";
			case Kind::Quartic:			return "Quartic";
			case Kind::Quintic:			return "Quintic";
			case Kind::Smooth:			return "Smooth";
			case Kind::Sinusoidal:		return "Sinusoidal";
			case Kind::SoftBack:		return "SoftBack";
			case Kind::Step:			return "Step";
			}

			return "Unexpected easing kind !";
		}
		static constexpr const char *TypeName( int easingType )
		{
			switch ( static_cast<Type>( easingType ) )
			{
			case Type::In:		return "In";
			case Type::Out:		return "Out";
			case Type::InOut:	return "InOut";
			}

			return "Unexpected easing type !";
		}

		/// <summary>
		/// Return value range is basically 0.0f ~ 1.0f(depends on type).<para></para>
		/// Please set 0.0f ~ 1.0f to "currentTime".
		/// </summary>
		static float Ease( Kind kind, Type type, float currentTime )
		{
			if ( kind == Kind::ENUM_TERMINATION ) { return NULL; }
			// else

			float adjustedTime = Impl::AdjustTime( type, currentTime );

			switch ( kind )
			{
			case Donya::Easing::Kind::Linear:
				return Impl::Linear( currentTime );
			case Donya::Easing::Kind::Back:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Back( adjustedTime )
				);
			case Donya::Easing::Kind::Bounce:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Bounce( adjustedTime )
				);
			case Donya::Easing::Kind::Circular:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Circular( adjustedTime )
				);
			case Donya::Easing::Kind::Cubic:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Cubic( adjustedTime )
				);
			case Donya::Easing::Kind::Elastic:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Elastic( adjustedTime )
				);
			case Donya::Easing::Kind::Exponential:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Exponential( adjustedTime )
				);
			case Donya::Easing::Kind::Quadratic:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Quadratic( adjustedTime )
				);
			case Donya::Easing::Kind::Quartic:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Quartic( adjustedTime )
				);
			case Donya::Easing::Kind::Quintic:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Quintic( adjustedTime )
				);
			case Donya::Easing::Kind::Smooth:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Smooth( adjustedTime )
				);
			case Donya::Easing::Kind::Sinusoidal:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Sinusoidal( adjustedTime )
				);
			case Donya::Easing::Kind::SoftBack:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::SoftBack( adjustedTime )
				);
			case Donya::Easing::Kind::Step:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Step( adjustedTime )
				);
			default: break;
			}
			
			return NULL;
		}
		/// <summary>
		/// Return value range is basically 0.0f ~ 1.0f(depends on type).<para></para>
		/// Please set 0.0f ~ 1.0f to "currentTime".
		/// </summary>
		static constexpr float Ease( CKind kind, Type type, float currentTime )
		{
			if ( kind == CKind::ENUM_TERMINATION ) { return NULL; }
			// else

			float adjustedTime = Impl::AdjustTime( type, currentTime );

			switch ( kind )
			{
			case Donya::Easing::CKind::BACK:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Back( adjustedTime )
				);
			case Donya::Easing::CKind::CUBIC:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Cubic( adjustedTime )
				);
			case Donya::Easing::CKind::ELASTIC:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Elastic( adjustedTime )
				);
			case Donya::Easing::CKind::QUADRATIC:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Quadratic( adjustedTime )
				);
			case Donya::Easing::CKind::QUARTIC:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Quartic( adjustedTime )
				);
			case Donya::Easing::CKind::QUINTIC:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Quintic( adjustedTime )
				);
			case Donya::Easing::CKind::SMOOTH:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Smooth( adjustedTime )
				);
			case Donya::Easing::CKind::SOFTBACK:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::SoftBack( adjustedTime )
				);
			case Donya::Easing::CKind::STEP:
				return Impl::AdjustResult
				(
					type, currentTime,
					Impl::Step( adjustedTime )
				);
			default: break;
			}

			return NULL;
		}

	}
}
