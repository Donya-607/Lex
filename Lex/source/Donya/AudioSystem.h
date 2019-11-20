#ifndef INCLUDED_DONYA_AUDIO_SYSTEM_H_
#define INCLUDED_DONYA_AUDIO_SYSTEM_H_

#include <memory>
#include <unordered_map>

namespace FMOD
{
	class System;
	class Sound;
	class Channel;
	namespace Studio
	{
		class System;
	}
}

namespace Donya
{
	/// <summary>
	/// This class Provides audio system by FMOD.<para></para>
	/// The constructor is doing initialize, the destructor is doing uninitialize.
	/// </summary>
	class AudioSystem
	{
	private:
		class Channels;
	private:
		FMOD::System			*pLowSystem;
		FMOD::Studio::System	*pSystem;

		// I should replace raw-pointer to smart-ptr.
		std::unordered_map<size_t, FMOD::Sound *>	sounds;
		std::unordered_map<size_t, std::unique_ptr<Channels>>	channels;
	public:
		AudioSystem();
		~AudioSystem();
		AudioSystem( const AudioSystem &  ) = delete;
		AudioSystem( const AudioSystem && ) = delete;
		AudioSystem & operator = ( const AudioSystem &  ) = delete;
		AudioSystem & operator = ( const AudioSystem && ) = delete;
	public:
		/// <summary>
		/// Please call every frame.
		/// </summary>
		void Update();
	public:
		/// <summary>
		/// Please set relative-path or whole-path to fileName.<para></para>
		/// If load successed, returns unique handle of sound.<para></para>
		/// If load failed, returns NULL. <para></para>
		/// If fileName is already loaded, returns that loaded handle.
		/// </summary>
		size_t Load( std::string fileName, bool isEnableLoop );

		/// <summary>
		/// Play the sound identified by handle.<para></para>
		/// If failed play, or not found, returns false.
		/// </summary>
		bool Play( size_t soundHandle );

		/// <summary>
		/// Pause the sound identified by handle.<para></para>
		/// If you want apply for all, set true to "isEnableForAll".<para></para>
		/// If failed pause, or not found, returns false.
		/// </summary>
		bool Pause( size_t soundHandle, bool isEnableForAll = false );
		
		/// <summary>
		/// Resume the sound identified by handle.<para></para>
		/// If you want apply for all, set true to "isEnableForAll".<para></para>
		/// If failed resume, or not found, returns false.
		/// </summary>
		bool Resume( size_t soundHandle, bool isEnableForAll = false, bool fromTheBeginning = false );

		/// <summary>
		/// Stop the sound identified by handle.<para></para>
		/// If you want apply for all, set true to "isEnableForAll".<para></para>
		/// If failed stop, or not found, returns false.
		/// </summary>
		bool Stop( size_t soundHandle, bool isEnableForAll = false );

		/// <summary>
		/// Stop the sound identified by handle.<para></para>
		/// "The volume level can be below 0 to invert a signal and above 1 to amplify the signal. Note that increasing the signal level too far may cause audible distortion."<para></para>
		/// If you want apply for all, set true to "isEnableForAll".<para></para>
		/// If failed set to volume, or not found, returns false.
		/// </summary>
		bool SetVolume( size_t soundHandle, float volume, bool isEnableForAll = false );

		/// <summary>
		/// Append the fade-point to sound identified by handle.<para></para>
		/// Take seconds is specified by "takeSeconds".<para></para>
		/// The volume at destination specified by "destinationVolume".<para></para>
		/// If you want apply for all, set true to "isEnableForAll".<para></para>
		/// If failed this method, or not found, returns false.
		/// </summary>
		bool AppendFadePoint( size_t soundHandle, float takeSeconds, float destinationVolume, bool isEnableForAll = false );

		/// <summary>
		/// Release the sound identified by handle.<para></para>
		/// If failed release, or not found, returns false.
		/// </summary>
		bool Release( size_t soundHandle );

		/// <summary>
		/// Release every sound.<para></para>
		/// If even one could not released, returns false.
		/// </summary>
		bool ReleaseAll();
	public:
		/// <summary>
		/// If failed count, returns -1.
		/// </summary>
		int GetNowPlayingSoundsCount();
	};
}

#endif // !INCLUDED_DONYA_AUDIO_SYSTEM_H_
