#pragma once

namespace Donya
{
	namespace ScreenShake
	{
		/// <summary>
		/// MOMENT : Amplitude is weaken gradually.<para></para>
		/// PERMANENCE : Amplitude is immutable.
		/// </summary>
		enum Kind
		{
			MOMENT = 0,
			PERMANENCE,
		};

		/// <summary>
		/// Please call every frame.
		/// </summary>
		void Update( float elapsedTime );

		/// <summary>
		/// This is state machine.<para></para>
		/// If enable true, Donya::Single, Donya::Batch classes apply of shift of draw coordinate.
		/// </summary>
		void SetEnableState( bool state );
		/// <summary>
		/// true is enable.
		/// </summary>
		bool GetEnableState();

		/// <summary>
		/// Shake parameter will overwrite.<para></para>
		/// initAmplitude : unit is pixel.<para></para>
		/// momentDeceleration : (Kind::MOMENT) amount of weakening of amplitude per frame.<para></para>
		/// permanenceSecond : (Kind::PERMANENCE) length of shake, unit is second.<para></para>
		/// interval : interval of update of shift a draw coordinate.
		/// </summary>
		void SetX( Kind kind, float initAmplitude, float momentDeceleration, float permanenceSecond, float interval );
		/// <summary>
		/// Shake parameter will overwrite.<para></para>
		/// initAmplitude : unit is pixel.<para></para>
		/// deceleration, time, interval will default.
		/// </summary>
		void SetX( Kind kind, float initAmplitude );

		/// <summary>
		/// Shake parameter will overwrite.<para></para>
		/// initAmplitude : unit is pixel.<para></para>
		/// momentDeceleration : (Kind::MOMENT) amount of weakening of amplitude per frame.<para></para>
		/// permanenceSecond : (Kind::PERMANENCE) length of shake, unit is second.<para></para>
		/// interval : interval of update of shift a draw coordinate.
		/// </summary>
		void SetY( Kind kind, float initAmplitude, float momentDeceleration, float permanenceSecond, float interval );
		/// <summary>
		/// Shake parameter will overwrite.<para></para>
		/// initAmplitude : unit is pixel.<para></para>
		/// deceleration, time, interval will default.
		/// </summary>
		void SetY( Kind kind, float initAmplitude );

		/// <summary>
		/// Stop shift of draw coordinate immediately.
		/// </summary>
		void StopX();
		/// <summary>
		/// Stop shift of draw coordinate immediately.
		/// </summary>
		void StopY();

		/// <summary>
		/// Returns current shift of draw coordinate.
		/// </summary>
		float GetX();
		/// <summary>
		/// Returns current shift of draw coordinate.
		/// </summary>
		float GetY();
	}
}
