#include "ScreenShake.h"

#include "Counter.h"

namespace Donya
{
	namespace ScreenShake
	{
		class Shift
		{
			Kind	kind;
			float	amplitude;			// not used by shift. updated every frame.
			float	decel;				// using when kind is MOMENT.
			float	shift;				// actually used by shift of draw coordinate. updated when interval is zero.
			Counter<float> interval;
			Counter<float> countDown;	// using when kind is PERMANENCE.
		public:
			Shift() : amplitude( 0.0f ), decel( 0.0f ), shift( 0.0f ), kind( MOMENT ), interval() {}
		public:
			void Update( float elapsedTime )
			{
				if ( !IsActive() ) { return; }
				// else

				interval.Update( elapsedTime );

				switch ( kind )
				{
				case MOMENT:		Moment( elapsedTime );		break;
				case PERMANENCE:	Permanence( elapsedTime );	break;
				}
			}

			void Set( Kind newKind, float initAmplitude, float deceleration, float time, float newInterval )
			{
				kind		= newKind;
				amplitude	= initAmplitude;
				decel		= deceleration;
				shift		= initAmplitude;

				interval.Set( newInterval, 1.0f );
				countDown.Set( time, 1.0f );
			}
			void Stop()
			{
				amplitude = decel = shift = 0.0f;
			}

			float Get() const
			{
				return shift;
			}
		private:
			void Moment( float elapsedTime )
			{
				amplitude -= decel * elapsedTime;
				if ( amplitude <= 0.0f )
				{
					amplitude	= 0.0f;
					shift		= 0.0f;
				}

				UpdateShift();
			}
			void Permanence( float elapsedTime )
			{
				countDown.Update( elapsedTime );

				if ( countDown.IsDone() )
				{
					Stop();
				}

				UpdateShift();
			}

			// TODO:こいつの戻り値をboolにして，shiftを変える瞬間にだけamplitudeを変えるようにするのはどうか。これでtrueが返ってくるまでの時間を記録し，amplitudeをdecel * その時間で引いたらいい感じになりそう
			void UpdateShift()
			{
				if ( !interval.IsDone() ) { return; }
				// else
				
				float sign = shift;

				shift = amplitude;

				shift *= ( 0 < sign ) ? -1 : 1;

				interval.Reset( 0.0f );
			}

			bool IsActive() const
			{
				return ( 0 < amplitude ) ? true : false;
			}
		};

		static Shift x{}, y{};

		void Update( float elapsedTime )
		{
			x.Update( elapsedTime );
			y.Update( elapsedTime );
		}

		static bool isEnableShakeState = true;
		void SetEnableState( bool state )	{ isEnableShakeState = state;	}
		bool GetEnableState()				{ return isEnableShakeState;	}

		void CalcDefaults( float amplitude, float *pDecelertaion, float *pTime, float *pInterval )
		{
			constexpr float DEFAULT_DECEL_FORCE	= 0.25f;
			constexpr float DEFAULT_TIME		= 1.0f;
			constexpr float DEFAULT_INTERVAL	= 0.05f;
			*pDecelertaion	= amplitude * DEFAULT_DECEL_FORCE;
			*pTime			= DEFAULT_TIME;
			*pInterval		= DEFAULT_INTERVAL;
		}

		void SetX( Kind kind, float initAmplitude, float mDeceleration, float pTime, float interval )
		{
			x.Set( kind, initAmplitude, mDeceleration, pTime, interval );
		}
		void SetX( Kind kind, float initAmplitude )
		{
			float decel = 0, time = 0, interval = 0;
			CalcDefaults( initAmplitude, &decel, &time, &interval );

			SetX( kind, initAmplitude, decel, time, interval );
		}

		void SetY( Kind kind, float initAmplitude, float mDeceleration, float pTime, float interval )
		{
			y.Set( kind, initAmplitude, mDeceleration, pTime, interval );
		}
		void SetY( Kind kind, float initAmplitude )
		{
			float decel = 0, time = 0, interval = 0;
			CalcDefaults( initAmplitude, &decel, &time, &interval );

			SetY( kind, initAmplitude, decel, time, interval );
		}

		void StopX() { x.Stop(); }
		void StopY() { y.Stop(); }

		float GetX() { return x.Get(); }
		float GetY() { return y.Get(); }
	}
}