#include "GamepadXInput.h"

#pragma comment( lib, "Xinput.lib" )

#include <algorithm>
#include <array>
#include <Windows.h> // You must include this before <Xinput.h>.
#include <Xinput.h>
#include <vector>

#include "Constant.h"
#include "Template.h"
#include "Useful.h"

#undef max
#undef min

namespace Donya
{
#pragma region Gamepad

	struct Gamepad::Impl
	{
		PadNumber padNo;
		
		struct Stick
		{
			Vector2 thumb; // -1.0f ~ 1.0f
			std::array<unsigned int, scast<int>( StickDirection::TERMINATION_OF_STICK_DIRECTIONS )> button;
			std::array<unsigned int, scast<int>( StickDirection::TERMINATION_OF_STICK_DIRECTIONS )> prevButton;
		public:
			Stick() : thumb(), button(), prevButton() {}
		public:
			void UpdateButton()
			{
				int signX = Donya::SignBit( thumb.x );
				int signY = Donya::SignBit( thumb.y );

				auto IncrementOrZero = []( unsigned int *pOutput, bool increment )
				{
					*pOutput =	( increment )
								? *pOutput + 1
								: 0;
				};

				prevButton = button;

				IncrementOrZero( &button[scast<int>( StickDirection::UP )],    signY == +1 );
				IncrementOrZero( &button[scast<int>( StickDirection::DOWN )],  signY == -1 );
				IncrementOrZero( &button[scast<int>( StickDirection::LEFT )],  signX == -1 );
				IncrementOrZero( &button[scast<int>( StickDirection::RIGHT )], signX == +1 );
			}
		};
		Stick stickL;
		Stick stickR;
		
		std::array<unsigned int, Button::TERMINATION_OF_BUTTON_TYPES> input;		// You can access to this element with enum of Gamepad::Button.
		std::array<unsigned int, Button::TERMINATION_OF_BUTTON_TYPES> prevInput;	// You can access to this element with enum of Gamepad::Button.

		bool isConnected;
	public:
		Impl( PadNumber padNumber ) :
			padNo( padNumber ),
			stickL(), stickR(), input(), prevInput(),
			isConnected( false )
		{
			
		}
		~Impl() = default;
	};

	Gamepad::Gamepad( PadNumber padNo ) : pImpl( std::make_unique<Gamepad::Impl>( padNo ) )
	{

	}
	Gamepad::~Gamepad()
	{
		pImpl.reset( nullptr );
	}

	Gamepad::PadNumber Gamepad::PadNo() const
	{
		return pImpl->padNo;
	}
	bool Gamepad::IsConnected() const
	{
		return pImpl->isConnected;
	}

	bool IsValidKind( Gamepad::Button kind )
	{
		return ( kind == Gamepad::Button::TERMINATION_OF_BUTTON_TYPES ) ? false : true;
	}

	unsigned int Gamepad::Press( Button kind ) const
	{
		if ( !IsValidKind( kind ) ) { return NULL; }
		// else

		return pImpl->input[kind];
	}
	bool Gamepad::Trigger( Button kind ) const
	{
		return ( Press( kind ) == 1 ) ? true : false;
	}
	bool Gamepad::Release( Button kind ) const
	{
		if ( !IsValidKind( kind ) )		{ return false; }
		if ( !pImpl->prevInput[kind] )	{ return false; }
		if ( pImpl->input[kind] )		{ return false; }
		// else
		return true;
	}

	bool IsValidDirection( Gamepad::StickDirection dir )
	{
		return ( dir == Gamepad::StickDirection::TERMINATION_OF_STICK_DIRECTIONS ) ? false : true;
	}

	unsigned int Gamepad::PressStick( StickDirection dir, bool leftStick ) const
	{
		if ( !IsValidDirection( dir ) ) { return NULL; }
		// else

		int iDir = scast<int>( dir );

		return ( leftStick ) ? pImpl->stickL.button[iDir] : pImpl->stickR.button[iDir];
	}
	bool Gamepad::TriggerStick( StickDirection dir, bool leftStick ) const
	{
		return ( PressStick( dir, leftStick ) == 1 ) ? true : false;
	}
	bool Gamepad::ReleaseStick( StickDirection dir, bool leftStick ) const
	{
		if ( !IsValidDirection( dir ) )	{ return false; }

		auto &input = ( leftStick ) ? pImpl->stickL : pImpl->stickR;
		int iDir = scast<int>( dir );

		if ( !input.prevButton[iDir] )	{ return false; }
		if ( input.button[iDir] )		{ return false; }
		// else
		return true;
	}

	Vector2 Gamepad::LeftStick() const
	{
		return pImpl->stickL.thumb;
	}
	Vector2 Gamepad::RightStick() const
	{
		return pImpl->stickR.thumb;
	}

	// region Gamepad
#pragma endregion

#pragma region XInput

	constexpr int XINPUT_BUTTONS_ARRAY_SIZE = Gamepad::Button::TERMINATION_OF_BUTTON_TYPES - 2/*The LT, RT is not contain in constants of XInput buttons*/;
	constexpr std::array<unsigned int, XINPUT_BUTTONS_ARRAY_SIZE>
	RequireXinputButtonArray()
	{
		constexpr std::array<unsigned int, XINPUT_BUTTONS_ARRAY_SIZE> XINPUT_BUTTONS
		{
			XINPUT_GAMEPAD_DPAD_UP,
			XINPUT_GAMEPAD_DPAD_DOWN,
			XINPUT_GAMEPAD_DPAD_LEFT,
			XINPUT_GAMEPAD_DPAD_RIGHT,
			XINPUT_GAMEPAD_START,
			XINPUT_GAMEPAD_BACK,
			XINPUT_GAMEPAD_LEFT_THUMB,
			XINPUT_GAMEPAD_RIGHT_THUMB,
			XINPUT_GAMEPAD_LEFT_SHOULDER,
			XINPUT_GAMEPAD_RIGHT_SHOULDER,
			XINPUT_GAMEPAD_A,
			XINPUT_GAMEPAD_B,
			XINPUT_GAMEPAD_X,
			XINPUT_GAMEPAD_Y
		};

		return XINPUT_BUTTONS;
	}

	struct XInput::Impl
	{
		XINPUT_STATE	state;
		int				vibrateTimer;
	public:
		Impl() : state(), vibrateTimer( 0 )
		{

		}
		~Impl() = default;
	public:
		static constexpr int VIBRATION_RANGE = 65535;
		XINPUT_VIBRATION MakeVibration( float leftStrength, float rightStrength )
		{
			// Note:I include <algorithm>, but I can't use std::clamp, why??
			auto Clamp = []( float value, float low, float high )
			{
				return std::max( low, std::min( high, value ) );
			};
			leftStrength  = Clamp( leftStrength,  0.0f, 1.0f );
			rightStrength = Clamp( rightStrength, 0.0f, 1.0f );

			XINPUT_VIBRATION vibration{};
			vibration.wLeftMotorSpeed  = scast<WORD>( leftStrength  * VIBRATION_RANGE );
			vibration.wRightMotorSpeed = scast<WORD>( rightStrength * VIBRATION_RANGE );
			return vibration;
		}
		XINPUT_VIBRATION MakeVibration( float strength )
		{
			float left  = strength;
			float right = strength * 0.5f; // Note:magic number. :(
			return MakeVibration( left, right );
		}

		/// <summary>
		/// Returns false if the controller does not connected.
		/// </summary>
		bool SetVibration( Gamepad::PadNumber padNo, int vibrateFrame, XINPUT_VIBRATION vibration )
		{
			auto result = XInputSetState( padNo, &vibration );

			if ( result == ERROR_DEVICE_NOT_CONNECTED )
			{
				// Error process here if necessary.
				return false;
			}
			// else

			vibrateTimer = vibrateFrame;

			return true;
		}
		/// <summary>
		/// Returns false if the controller does not connected.
		/// </summary>
		bool StopVibration( Gamepad::PadNumber padNo )
		{
			XINPUT_VIBRATION stop{};
			stop.wLeftMotorSpeed  = 0;
			stop.wRightMotorSpeed = 0;

			auto result = XInputSetState( padNo, &stop );

			if ( result == ERROR_DEVICE_NOT_CONNECTED )
			{
				// Error process here if necessary.
				return false;
			}
			// else

			return true;
		}

		void UpdateVibration( Gamepad::PadNumber padNo )
		{
			if ( vibrateTimer <= 0 ) { return; }
			// else

			vibrateTimer--;
			if ( vibrateTimer <= 0 )
			{
				StopVibration( padNo );
			}
		}
	};

	XInput::XInput( PadNumber padNo ) : Gamepad( padNo ), pXImpl( std::make_unique<XInput::Impl>() )
	{

	}
	XInput::XInput( const XInput &ref ) : Gamepad( ref.PadNo() ), pXImpl( Donya::Clone( ref.pXImpl ) )
	{

	}
	XInput &XInput::operator = ( const XInput &ref )
	{
		pXImpl = Donya::Clone( ref.pXImpl );

		return *this;
	}
	XInput::~XInput()
	{
		pXImpl.reset( nullptr );
	}

#pragma warning( push )
#pragma warning( disable : 4995 )
	void XInput::Uninit()
	{
		// HACK: error C4995: 名前が避けられた #pragma として記述されています。
		XInputEnable( FALSE );
	}
#pragma warning( pop )

	void XInput::Update()
	{
		auto result = XInputGetState( PadNo(), &pXImpl->state );

		if ( result == ERROR_DEVICE_NOT_CONNECTED )
		{
			pImpl->isConnected = false;
			return;
		}
		if ( result != ERROR_SUCCESS ) { return; }
		// else

		pImpl->isConnected = true;

		UpdateInputArray();

		pXImpl->UpdateVibration( PadNo() );
	}

	void XInput::UpdateInputArray()
	{
		pImpl->prevInput = pImpl->input;

		auto &pad = pXImpl->state.Gamepad;

		constexpr auto XINPUT_BUTTONS = RequireXinputButtonArray();
		constexpr int BUTTONS_SIZE = scast<int>( XINPUT_BUTTONS.size() );
		for ( size_t i = 0; i < BUTTONS_SIZE; ++i )
		{
			// Button::UP ~ Button::Y.

			if ( pad.wButtons & XINPUT_BUTTONS[i] )
			{
				pImpl->input[i]++;
			}
			else
			{
				pImpl->input[i] = 0;
			}
		}

		// Button::LT, Button::RT.
		{
			auto IsOverThreshold = []( int LRTrigger )->bool
			{
				return	( XINPUT_GAMEPAD_TRIGGER_THRESHOLD < abs( LRTrigger ) )
						? true
						: false;
			};

			pImpl->input[Button::LT] =	IsOverThreshold( pad.bLeftTrigger )
										? pImpl->input[Button::LT] + 1
										: 0;
			pImpl->input[Button::RT] =	IsOverThreshold( pad.bRightTrigger )
										? pImpl->input[Button::RT] + 1
										: 0;
		}

		// Stick.
		{
			auto IsOverDeadZone = []( bool isRight, int param )->bool
			{
				int deadZone =	( isRight )
								? XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE
								: XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;

				return ( deadZone < abs( param ) ) ? true : false;
			};
			auto NormalizeThumb = []( int thumbParam )->float
			{
				// see https://docs.microsoft.com/ja-jp/windows/win32/api/xinput/ns-xinput-xinput_gamepad

				constexpr float EDGE_POS = 32768.0f;
				return scast<float>( thumbParam ) / EDGE_POS;
			};

			// Left.
			pImpl->stickL.thumb.x =	( IsOverDeadZone( /* isRight = */ false, pad.sThumbLX ) )
									? NormalizeThumb( pad.sThumbLX )
									: 0;
			pImpl->stickL.thumb.y =	( IsOverDeadZone( /* isRight = */ false, pad.sThumbLY ) )
									? NormalizeThumb( pad.sThumbLY )
									: 0;
			// Right.
			pImpl->stickR.thumb.x =	( IsOverDeadZone( /* isRight = */ true,  pad.sThumbRX ) )
									? NormalizeThumb( pad.sThumbRX )
									: 0;
			pImpl->stickR.thumb.y =	( IsOverDeadZone( /* isRight = */ true,  pad.sThumbRY ) )
									? NormalizeThumb( pad.sThumbRY )
									: 0;

			pImpl->stickL.UpdateButton();
			pImpl->stickR.UpdateButton();
		}
	}

	void XInput::Vibrate( int vibrateFrame, float leftStrength, float rightStrength )
	{
		auto vibration = pXImpl->MakeVibration( leftStrength, rightStrength );
		bool result = pXImpl->SetVibration( PadNo(), vibrateFrame, vibration );

		if ( !result )
		{
			pImpl->isConnected = false;
		}
	}
	void XInput::Vibrate( int vibrateFrame, float strength )
	{
		auto vibration = pXImpl->MakeVibration( strength );
		bool result = pXImpl->SetVibration( PadNo(), vibrateFrame, vibration );

		if ( !result )
		{
			pImpl->isConnected = false;
		}
	}

	void XInput::StopVibration()
	{
		pXImpl->StopVibration( PadNo() );
	}

	// region XInput
#pragma endregion

}