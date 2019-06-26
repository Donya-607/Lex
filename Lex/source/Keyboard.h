#ifndef INCLUDED_KEYBOARD_H_
#define INCLUDED_KEYBOARD_H_

#define USE_STATIC_ARRAY_FOR_KEYBOARD_UPDATE ( true )

namespace Donya
{

#if USE_STATIC_ARRAY_FOR_KEYBOARD_UPDATE

	/// <summary>
	/// Using GetKeyboardState().
	/// </summary>
	namespace Keyboard
	{
		/// <summary>
		/// Please call every frame.
		/// </summary>
		void Update();

		enum Mode
		{
			PRESS = 0,
			TRIGGER,
			RELEASE
		};

		bool State( int virtualKeycode, Mode inputMode = TRIGGER );
		bool Press( int virtualKeycode );
		bool Trigger( int virtualKeycode );
		bool Release( int virtualKeycode );

		/// <summary>
		/// State() by focus of VK_LSHIFT, VK_RSHIFT.
		/// </summary>
		bool Shifts( Mode inputMode = PRESS );
	}

#else

	/// <summary>
	/// You can only focus on the specified key.
	/// </summary>
	class Keyboard
	{
	public:
		enum class Mode : int
		{
			PRESS = 0,
			TRIGGER,
			RELEASE
		};
	private:
		int vKeycode;
		int currentState;
		int previousState;
	public:
		Keyboard( int virtualKeycode ) :
			vKeycode( virtualKeycode ),
			currentState( 0 ),
			previousState( 0 )
		{}
		~Keyboard() = default;
		Keyboard( const Keyboard & ) = delete;
		Keyboard( const Keyboard && ) = delete;
		Keyboard & operator = ( const Keyboard & ) = delete;
		Keyboard & operator = ( const Keyboard && ) = delete;
	public:
		bool State( Keyboard::Mode inputMode );
	};

#endif // USE_STATIC_ARRAY_FOR_KEYBOARD_UPDATE

}

#endif // !INCLUDED_KEYBOARD_H_
