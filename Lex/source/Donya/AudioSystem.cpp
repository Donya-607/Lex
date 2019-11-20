#include "AudioSystem.h"

#include <fmod.hpp>
#include <fmod_studio.hpp>
#include <fmod_errors.h>
#include <functional>

#include <string>

#include "Constant.h"
#include "Useful.h"

bool FMODFailed( FMOD_RESULT fr )
{
	return	(
				( fr != FMOD_OK )
				&& ( fr != FMOD_ERR_INVALID_HANDLE )
				&& ( fr != FMOD_ERR_CHANNEL_STOLEN )
			)
			? true
			: false;
}
void OutputDebugFMODErrorString( FMOD_RESULT fr )
{
#if DEBUG_MODE

	std::wstring str{};
	str += L"[OutputDebugString] ";
	str += Donya::UTF8ToWide( FMOD_ErrorString( fr ) );
	str += L'\n';
	Donya::OutputDebugStr( str.data() );

#endif // DEBUG_MODE
}
void OutputDebugFMODErrorStringAndExit( FMOD_RESULT fr, const wchar_t *expression )
{
	OutputDebugFMODErrorString( fr );

	_ASSERT_EXPR( 0, expression );
	std::exit( EXIT_FAILURE );
}

// Doing FMODFailed() and OutputDebugFMODErrorString(). returns FMODFailed() result.
bool OutputDebugErrorStringIfFMODFailed( FMOD_RESULT fr )
{
	if ( FMODFailed( fr ) )
	{
		OutputDebugFMODErrorString( fr );
		return true;
	}
	// else
	return false;
}

namespace Donya
{
	class AudioSystem::Channels
	{
	private:
		std::vector<FMOD::Channel *> channels;
	public:
		Channels() : channels() {}
		~Channels() { std::vector<FMOD::Channel *>().swap( channels ); }
	public:
		FMOD::Channel **RequireNullChannel()
		{
			if ( channels.empty() || channels.back() != nullptr )
			{
				channels.push_back( nullptr );
			}

			return &channels.back();
		}
	private:

		// These method processing from latest channel. because I think the target of who want to operate is recently-played channel.

		/// <summary>
		/// Apply to first found channel, or all(when "applyAll" is true).<para></para>
		/// If is there null channel or an error occurred, the channel will be remove. then I continue pause until successful.<para></para>
		/// Return true if not failed.
		/// </summary>
		bool SetPauseState( bool setPause, bool applyAll, bool resumeFromTheBeginning = false )
		{
			bool			failed		= false;
			size_t			offset		= 0;
			size_t			arraySize	= channels.size();
			FMOD_RESULT		fr			= FMOD_OK;
			FMOD::Channel	*pChannel	= nullptr;

			auto PopBack = [&]()
			{
				channels.pop_back();
				arraySize--;
			};

			while ( !channels.empty() )
			{
				if ( arraySize <= offset ) { break; }
				// else

				pChannel = *( channels.rbegin() + offset );

				if ( pChannel == nullptr )
				{
					PopBack();
					continue;
				}
				// else

				bool nowPaused = false;
				fr = pChannel->getPaused( &nowPaused );
				if ( FMODFailed( fr ) )
				{
					failed = true;
					PopBack();
					continue;
				}
				// else

				if ( nowPaused == setPause ) { offset++; continue; }
				// else

				if ( !setPause && resumeFromTheBeginning )
				{
					fr = pChannel->setPosition( 0, FMOD_TIMEUNIT_MS );
					if ( OutputDebugErrorStringIfFMODFailed( fr ) )
					{
						failed = true;
						PopBack();
						continue;
					}
				}

				fr = pChannel->setPaused( setPause ? true : false );
				if ( FMODFailed( fr ) )
				{
					failed = true;
					PopBack();
					continue;
				}
				// else

				if ( applyAll )
				{
					offset++;
					continue;
				}
				// else
				return ( failed ) ? false : true;
			}

			return ( failed ) ? false : true;
		}
		/// <summary>
		/// Apply to first found channel, or all(when "applyAll" is true).<para></para>
		/// If is there null channel or an error occurred, the channel will be remove. then I continue stop until successful.<para></para>
		/// Return true if not failed.
		/// </summary>
		bool StopAndRemove( bool applyAll )
		{
			bool			failed		= false;
			size_t			offset		= 0;
			size_t			arraySize	= channels.size();
			FMOD_RESULT		fr			= FMOD_OK;
			FMOD::Channel	*pChannel	= nullptr;

			auto PopBack = [&]()
			{
				channels.pop_back();
				arraySize--;
			};

			while ( !channels.empty() )
			{
				if ( arraySize <= offset ) { break; }
				// else

				pChannel = *( channels.rbegin() + offset );

				if ( pChannel == nullptr )
				{
					PopBack();
					continue;
				}
				// else

				// Applies only to now playing channel. because if stop pausing channel, user don't understand result.
				bool isPlaying = false;
				fr = pChannel->isPlaying( &isPlaying );
				if ( FMODFailed( fr ) )
				{
					failed = true;
					PopBack();
					continue;
				}
				// else

				if ( !isPlaying ) { offset++; continue; }
				// else

				fr = pChannel->stop();
				if ( FMODFailed( fr ) )
				{
					failed = true;
					PopBack();
					continue;
				}
				// else

				// Remove stoped channel.
				PopBack();

				if ( applyAll ) { continue; }
				// else
				return ( failed ) ? false : true;
			}

			return ( failed ) ? false : true;
		}
		/// <summary>
		/// Apply to first found channel, or all(when "applyAll" is true).<para></para>
		/// If is there null channel or an error occurred, the channel will be remove. then I continue setting volume until successful.<para></para>
		/// Return true if not failed.
		/// </summary>
		bool SetVolume( float volume, bool applyAll )
		{
			bool			failed		= false;
			size_t			offset		= 0;
			size_t			arraySize	= channels.size();
			FMOD_RESULT		fr			= FMOD_OK;
			FMOD::Channel	*pChannel	= nullptr;

			auto PopBack = [&]()
			{
				channels.pop_back();
				arraySize--;
			};

			while ( !channels.empty() )
			{
				if ( arraySize <= offset ) { break; }
				// else

				pChannel = *( channels.rbegin() + offset );

				if ( pChannel == nullptr )
				{
					PopBack();
					continue;
				}
				// else

				fr = pChannel->setVolume( volume );
				if ( FMODFailed( fr ) )
				{
					failed = true;
					PopBack();
					continue;
				}
				// else

				if ( applyAll )
				{
					offset++;
					continue;
				}
				// else
				return ( failed ) ? false : true;
			}

			return ( failed ) ? false : true;
		}
		/// <summary>
		/// Apply to first found channel, or all(when "applyAll" is true).<para></para>
		/// If is there null channel or an error occurred, the channel will be remove. then I continue append fade-point until successful.<para></para>
		/// Return true if not failed.
		/// </summary>
		bool AppendFadePoint( float takeSecond, float destVolume, bool applyAll )
		{
			bool				failed		= false;
			size_t				offset		= 0;
			size_t				arraySize	= channels.size();
			FMOD_RESULT			fr			= FMOD_OK;
			FMOD::Channel		*pChannel	= nullptr;

			int					mixerRate	= 0;
			float				oldVolume	= 0.0f;
			unsigned long long	DSPClock	= 0ull;	// Reference clock, which is the parent channel group.
			unsigned long long	distance	= 0ull;	// "DSPClock" + "distance" = destination clock.
			FMOD::System		*pSystem	= nullptr;

			auto PopBack = [&]()
			{
				channels.pop_back();
				arraySize--;
			};

			while ( !channels.empty() )
			{
				if ( arraySize <= offset ) { break; }
				// else

				pChannel = *( channels.rbegin() + offset );

				if ( pChannel == nullptr )
				{
					PopBack();
					continue;
				}
				// else

				// see https://qa.fmod.com/t/how-do-i-fade-in-and-fade-out-a-channel-with-stop/11738/2

			#pragma region FetchRequiredVariables

				fr = pChannel->getVolume( &oldVolume );
				if ( FMODFailed( fr ) )
				{
					failed = true;
					PopBack();
					continue;
				}
				// else

				fr = pChannel->getSystemObject( &pSystem );
				if ( FMODFailed( fr ) )
				{
					failed = true;
					PopBack();
					continue;
				}
				// else

				fr = pSystem->getSoftwareFormat( &mixerRate, 0, 0 );
				if ( FMODFailed( fr ) )
				{
					failed = true;
					PopBack();
					continue;
				}
				// else
				
				fr = pChannel->getDSPClock( 0, &DSPClock );
				if ( FMODFailed( fr ) )
				{
					failed = true;
					PopBack();
					continue;
				}
				// else

			// region FetchRequiredVariables
			#pragma endregion

				// Avoid warning C26451.
				double dMixerRate  = scast<double>( mixerRate  );
				double dTakeSecond = scast<double>( takeSecond );
				distance = scast<unsigned long long>( dMixerRate * dTakeSecond );

				fr = pChannel->addFadePoint( DSPClock, oldVolume );
				if ( FMODFailed( fr ) )
				{
					failed = true;
					PopBack();
					continue;
				}
				// else
				fr = pChannel->addFadePoint( DSPClock + distance, destVolume );
				if ( FMODFailed( fr ) )
				{
					failed = true;
					PopBack();
					continue;
				}
				// else

				if ( applyAll )
				{
					offset++;
					continue;
				}
				// else
				return ( failed ) ? false : true;
			}

			return ( failed ) ? false : true;
		}
	public:
		/// <summary>
		/// Pause one out of channels of not pausing.<para></para>
		/// If found invalid channel, remove the channel.<para></para>
		/// Returns true if successed pause.
		/// </summary>
		bool PauseOne()
		{
			return SetPauseState( /* setPause = */ true, /* applyAll = */ false );
		}
		/// <summary>
		/// Pause all channels of not pausing.<para></para>
		/// If found invalid channel, remove the channel.<para></para>
		/// Returns true if successed pause of all in channels.
		/// </summary>
		bool PauseAll()
		{
			return SetPauseState( /* setPause = */ true, /* applyAll = */ true );
		}

		/// <summary>
		/// Resume one out of channels of now pausing.<para></para>
		/// If found invalid channel, remove the channel.<para></para>
		/// Returns true if successed resume.
		/// </summary>
		bool ResumeOne( bool fromTheBeginning )
		{
			return SetPauseState( /* setPause = */ false, /* applyAll = */ false, fromTheBeginning );
		}
		/// <summary>
		/// Resume all channels of now pausing.<para></para>
		/// If found invalid channel, remove the channel.<para></para>
		/// Returns true if successed resume of all in channels.
		/// </summary>
		bool ResumeAll( bool fromTheBeginning )
		{
			return SetPauseState( /* setPause = */ false, /* applyAll = */ true, fromTheBeginning );
		}

		/// <summary>
		/// Stop one out of channels.<para></para>
		/// If found invalid channel, remove the channel.<para></para>
		/// Returns true if successed stop.
		/// </summary>
		bool StopOne()
		{
			return StopAndRemove( /* applyAll = */ false );
		}
		/// <summary>
		/// Stop all channels.<para></para>
		/// If found invalid channel, remove the channel.<para></para>
		/// Returns true if successed stop of all in channels.
		/// </summary>
		bool StopAll()
		{
			return StopAndRemove( /* applyAll = */ true );
		}

		/// <summary>
		/// Set the volume of sound one out of channels.<para></para>
		/// If found invalid channel, remove the channel.<para></para>
		/// Returns true if setting volume is successed.
		/// </summary>
		bool SetVolumeOne( float volume )
		{
			return SetVolume( volume, /* applyAll = */ false );
		}
		/// <summary>
		/// Set the volume of sound all channels.<para></para>
		/// If found invalid channel, remove the channel.<para></para>
		/// Returns true if setting volume is successed of all in channels.
		/// </summary>
		bool SetVolumeAll( float volume )
		{
			return SetVolume( volume, /* applyAll = */ true );
		}

		/// <summary>
		/// Append fade-point of sound one out of channels.<para></para>
		/// If found invalid channel, remove the channel.<para></para>
		/// Returns true if append fade-point is successed.
		/// </summary>
		bool AppendFadePointOne( float takeSecond, float destVolume )
		{
			return AppendFadePoint( takeSecond, destVolume, /* applyAll = */ false );
		}
		/// <summary>
		/// Append fade-point of sound all channels.<para></para>
		/// If found invalid channel, remove the channel.<para></para>
		/// Returns true if append fade-point is successed of all in channels.
		/// </summary>
		bool AppendFadePointAll( float takeSecond, float destVolume )
		{
			return AppendFadePoint( takeSecond, destVolume, /* applyAll = */ true );
		}

		void ReleaseAll()
		{
			FMOD_RESULT fr = FMOD_OK;
			FMOD::Channel *pChannel = nullptr;
			while ( !channels.empty() )
			{
				pChannel = channels.back();
				if ( pChannel == nullptr )
				{
					channels.pop_back();
					continue;
				}
				// else

				fr = pChannel->stop();
				OutputDebugErrorStringIfFMODFailed( fr );

				channels.pop_back();
			}
		}
	};

	// see https://books.google.co.jp/books?id=-2Z9DwAAQBAJ&pg=PA233&lpg=PA233&dq=FMOD+API+%E3%83%AA%E3%83%95%E3%82%A1%E3%83%AC%E3%83%B3%E3%82%B9&source=bl&ots=Lr5E9R80lv&sig=ACfU3U2qol2OMj9f0DF5vOof5vc6XrDbOw&hl=ja&sa=X&ved=2ahUKEwjzrInMn-HiAhVIWbwKHRemCB0Q6AEwAnoECAkQAQ#v=onepage&q=FMOD%20API%20%E3%83%AA%E3%83%95%E3%82%A1%E3%83%AC%E3%83%B3%E3%82%B9&f=false

	AudioSystem::AudioSystem() :
		pLowSystem( nullptr ), pSystem( nullptr ),
		sounds(), channels()
	{
		FMOD_RESULT fr = FMOD_OK;

	#if DEBUG_MODE
		fr = FMOD::Debug_Initialize( FMOD_DEBUG_LEVEL_WARNING );
		if ( FMODFailed( fr ) )
		{
			OutputDebugFMODErrorStringAndExit( fr, L"Failed : FMOD debug initialize." );
		}
		// else
	#endif // DEBUG_MODE

		fr = FMOD::Studio::System::create( &pSystem );
		if ( FMODFailed( fr ) )
		{
			OutputDebugFMODErrorStringAndExit( fr, L"Failed : FMOD Studio system create." );
		}
		// else
		fr = pSystem->initialize( 512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr );
		if ( FMODFailed( fr ) )
		{
			OutputDebugFMODErrorStringAndExit( fr, L"Failed : FMOD Studio initialize." );
		}
		// else
		fr = pSystem->getLowLevelSystem( &pLowSystem );
		if ( FMODFailed( fr ) )
		{
			OutputDebugFMODErrorStringAndExit( fr, L"Failed : FMOD low system get." );
		}
		// else
	}
	AudioSystem::~AudioSystem()
	{
		ReleaseAll();

		FMOD_RESULT fr = FMOD_OK;
		fr = pSystem->release();	// Release Studio::System object and everything created under it.
		if ( FMODFailed( fr ) )
		{
			OutputDebugFMODErrorStringAndExit( fr, L"Failed : FMOD update." );
			return;
		}
		// else
	}

	void AudioSystem::Update()
	{
		FMOD_RESULT fr = FMOD_OK;
		fr = pSystem->update();	// FMOD::Studio::update() is also calling FMOD::update().
		if ( FMODFailed( fr ) )
		{
			OutputDebugFMODErrorStringAndExit( fr, L"Failed : FMOD update." );
			return;
		}
		// else
	}

	size_t AudioSystem::Load( std::string fileName, bool isEnableLoop )
	{
		size_t hash = std::hash<std::string>()( fileName );
		if ( hash == NULL )	// NULL using error code.
		{
			hash = 1;
		}

		decltype( sounds )::iterator it = sounds.find( hash );
		if ( it != sounds.end() ) { return it->first; } // it->first == hash
		// else

		FMOD_RESULT fr = FMOD_OK;
		FMOD_MODE mode = FMOD_DEFAULT;
		mode |= ( isEnableLoop ) ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;

		FMOD::Sound *pSound = nullptr;
		fr = pLowSystem->createSound( fileName.c_str(), mode, nullptr, &pSound );
		if ( OutputDebugErrorStringIfFMODFailed( fr ) ) { return NULL; }
		// else

		sounds.insert( std::make_pair( hash, pSound ) );
		channels.insert( std::make_pair( hash, std::make_unique<Channels>() ) );

		return hash;
	}

	bool AudioSystem::Play( size_t handle )
	{
		decltype( sounds )::iterator itrSound = sounds.find( handle );
		if ( itrSound == sounds.end() ) { return false; }
		// else

		FMOD_RESULT fr = FMOD_OK;

		decltype( channels )::iterator itrChannel = channels.find( handle );
		FMOD::Channel **pChannel = itrChannel->second->RequireNullChannel();

		fr = pLowSystem->playSound( itrSound->second, NULL, false, pChannel );
		if ( OutputDebugErrorStringIfFMODFailed( fr ) ) { return false; }
		// else

		return true;
	}

	bool AudioSystem::Pause( size_t handle, bool isEnableForAll )
	{
		decltype( sounds )::iterator itrSound = sounds.find( handle );
		if ( itrSound == sounds.end() ) { return false; }
		// else

		decltype( channels )::iterator itrChannel = channels.find( handle );
		return ( isEnableForAll ) ? itrChannel->second->PauseAll() : itrChannel->second->PauseOne();
	}

	bool AudioSystem::Resume( size_t handle, bool isEnableForAll, bool fromBeginning )
	{
		decltype( sounds )::iterator itrSound = sounds.find( handle );
		if ( itrSound == sounds.end() ) { return false; }
		// else

		decltype( channels )::iterator itrChannel = channels.find( handle );
		return ( isEnableForAll ) ? itrChannel->second->ResumeAll( fromBeginning ) : itrChannel->second->ResumeOne( fromBeginning );
	}

	bool AudioSystem::Stop( size_t handle, bool isEnableForAll )
	{
		decltype( sounds )::iterator itrSound = sounds.find( handle );
		if ( itrSound == sounds.end() ) { return false; }
		// else

		decltype( channels )::iterator itrChannel = channels.find( handle );
		return ( isEnableForAll ) ? itrChannel->second->StopAll() : itrChannel->second->StopOne();
	}

	bool AudioSystem::SetVolume( size_t handle, float volume, bool isEnableForAll )
	{
		decltype( sounds )::iterator itrSound = sounds.find( handle );
		if ( itrSound == sounds.end() ) { return false; }
		// else

		decltype( channels )::iterator itrChannel = channels.find( handle );
		return ( isEnableForAll ) ? itrChannel->second->SetVolumeAll( volume ) : itrChannel->second->SetVolumeOne( volume );
	}

	bool AudioSystem::AppendFadePoint( size_t handle, float sec, float destVol, bool isEnableForAll )
	{
		decltype( sounds )::iterator itrSound = sounds.find( handle );
		if ( itrSound == sounds.end() ) { return false; }
		// else

		decltype( channels )::iterator itrChannel = channels.find( handle );
		return ( isEnableForAll ) ? itrChannel->second->AppendFadePointAll( sec, destVol ) : itrChannel->second->AppendFadePointOne( sec, destVol );
	}

	bool AudioSystem::Release( size_t handle )
	{
		decltype( sounds )::iterator itrSound = sounds.find( handle );
		if ( itrSound == sounds.end() ) { return false; }
		// else

		FMOD_RESULT fr = FMOD_OK;
		fr = itrSound->second->release();

		if ( OutputDebugErrorStringIfFMODFailed( fr ) ) { return false; }
		// else

		decltype( channels )::iterator itrChannel = channels.find( handle );
		itrChannel->second->ReleaseAll();

		return true;
	}

	bool AudioSystem::ReleaseAll()
	{
		FMOD_RESULT fr = FMOD_OK;
		FMOD_RESULT irrelevant = FMOD_OK;
		FMOD_RESULT *appResult = &fr;

		for ( auto &it : sounds )
		{
			*appResult = it.second->release();
			if ( FMODFailed( *appResult ) )
			{
				appResult = &irrelevant;
			}
		}

		if ( OutputDebugErrorStringIfFMODFailed( fr ) ) { return false; }
		// else

		for ( auto &it : channels )
		{
			it.second->ReleaseAll();
			it.second.reset( nullptr );
		}

		sounds.clear();
		channels.clear();

		return true;
	}

	int AudioSystem::GetNowPlayingSoundsCount()
	{
		int rv = 0;
		FMOD_RESULT fr = FMOD_OK;

		fr = pLowSystem->getChannelsPlaying( &rv, NULL );
		if ( OutputDebugErrorStringIfFMODFailed( fr ) ) { return -1; }
		// else

		return rv;
	}

}