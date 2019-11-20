#pragma once

#include <memory>

#include "Vector.h"

namespace Donya
{

	class Gamepad
	{
	public:
		enum PadNumber
		{
			PAD_1 = 0,
			PAD_2 = 1,
			PAD_3 = 2,
			PAD_4 = 3
		};
		enum Button
		{
			UP = 0,		// Up-key of d-pad.
			DOWN,		// Down-key of d-pad.
			LEFT,		// Left-key of d-pad.
			RIGHT,		// Right-key of d-pad.
			START,		// The center-button of right-side.
			SELECT,		// The center-button of left-side.
			PRESS_L,	// Press of Left-stick(Left-thumb).
			PRESS_R,	// Press of Right-stick(Right-thumb).
			LB,			// The L-button of shoulder.
			RB,			// The R-button of shoulder.
			A,			// Following to XInput style.
			B,			// Following to XInput style.
			X,			// Following to XInput style.
			Y,			// Following to XInput style.
			LT,			// The L-trigger.
			RT,			// The R-trigger.

			TERMINATION_OF_BUTTON_TYPES // End flag. This is invalid parameter.
		};
		enum class StickDirection
		{
			UP = 0,
			DOWN,
			LEFT,
			RIGHT,

			TERMINATION_OF_STICK_DIRECTIONS
		};
	protected:
		struct Impl;
		std::unique_ptr<Impl> pImpl;
	public:
		Gamepad( PadNumber padNumber );
		virtual ~Gamepad() = 0;
	public:
		PadNumber PadNo() const;
		bool IsConnected() const;
	public:
		unsigned int Press( Button kind ) const;
		bool Trigger( Button kind ) const;
		bool Release( Button kind ) const;

		unsigned int PressStick( StickDirection direction, bool leftStick = true ) const;
		bool TriggerStick( StickDirection direction, bool leftStick = true ) const;
		bool ReleaseStick( StickDirection direction, bool leftStick = true ) const;

		/// <summary>
		/// Returns (x, y) is [-1.0f ~ 1.0f].
		/// </summary>
		Vector2 LeftStick() const;
		/// <summary>
		/// Returns (x, y) is [-1.0f ~ 1.0f].
		/// </summary>
		Vector2 RightStick() const;
	};

	class XInput : public Gamepad
	{
	public:
		/// <summary>
		/// Please call when end the application.
		/// </summary>
		static void Uninit();
	private:
		struct Impl;
		std::unique_ptr<Impl> pXImpl;
	public:
		XInput( Gamepad::PadNumber padNumber );
		XInput( const XInput & );
		XInput &operator = ( const XInput & );
		~XInput() override;
	public:
		void Update();

		/// <summary>
		/// If you want vibrate always, set -1 to "vibrateFrame".<para></para>
		/// These strength range is 0.0f ~ 1.0f.
		/// </summary>
		void Vibrate( int vibrateFrame, float leftStrength, float rightStrength );
		/// <summary>
		/// If you want vibrate always, set -1 to "vibrateFrame".<para></para>
		/// The strength range is 0.0f ~ 1.0f.
		/// </summary>
		void Vibrate( int vibrateFrame, float strength );

		void StopVibration();
	private:
		void UpdateInputArray();
	};

}
