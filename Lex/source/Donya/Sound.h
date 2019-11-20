#ifndef INCLUDED_DONYA_SOUND_H_
#define INCLUDED_DONYA_SOUND_H_

#include <string>

namespace Donya
{
	/// <summary>
	/// This namespace is facade-pattern.<para></para>
	/// For users to use AudioSystem from anywhere.
	/// </summary>
	namespace Sound
	{
		/// <summary>
		/// Doing initialize and load sounds.
		/// </summary>
		void Init();
		/// <summary>
		/// Doing release sounds and uninitialize.
		/// </summary>
		void Uninit();

		/// <summary>
		/// Please call every frame.
		/// </summary>
		void Update();

		/// <summary>
		/// The soundIdentifier is became identifier of the sound of another sound function.<para></para>
		/// Please set relative-path or whole-path to fileName.<para></para>
		/// If load successed or already loaded, returns true.
		/// </summary>
		bool Load( int soundIdentifier, std::string fileName, bool isEnableLoop );

		/// <summary>
		/// If failed play sound, or the identifier is incorrect, returns false.
		/// </summary>
		bool Play( int soundIdentifier );
		// TODO:I want user can specify play mode(ex:loop).

		/// <summary>
		/// If you want apply for all, set true to "isEnableForAll".<para></para>
		/// If failed pause sound, or the identifier is incorrect, returns false.
		/// </summary>
		bool Pause( int soundIdentifier, bool isEnableForAll = false );

		/// <summary>
		/// If you want apply for all, set true to "isEnableForAll".<para></para>
		/// If failed resume sound, or the identifier is incorrect, returns false.
		/// </summary>
		bool Resume( int soundIdentifier, bool isEnableForAll = false, bool fromTheBeginning = false );

		/// <summary>
		/// If you want apply for all, set true to "isEnableForAll".<para></para>
		/// If failed stop sound, or the identifier is incorrect, returns false.
		/// </summary>
		bool Stop( int soundIdentifier, bool isEnableForAll = false );

		/// <summary>
		/// "The volume level can be below 0 to invert a signal and above 1 to amplify the signal. Note that increasing the signal level too far may cause audible distortion."<para></para>
		/// If you want apply for all, set true to "isEnableForAll".<para></para>
		/// If failed setting volume of sound, or the identifier is incorrect, returns false.
		/// </summary>
		bool SetVolume( int soundIdentifier, float volume, bool isEnableForAll = false );

		/// <summary>
		/// Append the fade-point to sound identified by handle.<para></para>
		/// Take seconds is specified by "takeSeconds".<para></para>
		/// The volume at destination specified by "destinationVolume".<para></para>
		/// If you want apply for all, set true to "isEnableForAll".<para></para>
		/// If failed setting volume of sound, or the identifier is incorrect, returns false.
		/// </summary>
		bool AppendFadePoint( int soundIdentifier, float takeSeconds, float destinationVolume, bool isEnableForAll = false );

		/// <summary>
		/// If failed count, returns -1.
		/// </summary>
		int  GetNowPlayingSoundsCount();
	}
}

#endif // !INCLUDED_DONYA_SOUND_H_
