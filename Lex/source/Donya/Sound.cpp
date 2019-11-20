#include "Sound.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "AudioSystem.h"
#include "Constant.h"		// Use for DEBUG_MODE.

#if DEBUG_MODE

#include "Useful.h"			// Use for OutputDebugStr().

#endif // DEBUG_MODE

namespace Donya
{
	namespace Sound
	{
		typedef std::unordered_map<int, size_t> SoundHandleMap;

		// Instances does not create until use.
		static std::unique_ptr<AudioSystem>		pAudio{ nullptr };
		static std::unique_ptr<SoundHandleMap>	pSoundHandles{ nullptr };

		void Init()
		{
			pAudio			= std::make_unique<AudioSystem>();
			pSoundHandles	= std::make_unique<SoundHandleMap>();
		}
		void Uninit()
		{
			if ( !pAudio ) { return; }	// Already Uninitialized.
			// else

			pAudio->ReleaseAll();

			pAudio.reset( nullptr ); // Doing release of sounds by AudioSystem::destructor.
			pSoundHandles.reset( nullptr );
		}

		void InitIfNullptr()
		{
			if ( pAudio == nullptr ) { Init(); }
		}

		size_t GetHandleOrNull( int id )
		{
			// HACK:"pSoundHandles" may be null.

			auto it =  pSoundHandles->find( id );
			if ( it == pSoundHandles->end() ) { return NULL; }
			// else
			return it->second;
		}

		void Update()
		{
			InitIfNullptr();

			pAudio->Update();
		}

		bool Load( int id, std::string fileName, bool isEnableLoop )
		{
			size_t handle = GetHandleOrNull( id );
			if ( handle != NULL ) { return true; }	// already loaded.
			// else

			handle = pAudio->Load( fileName.c_str(), isEnableLoop );

			if ( handle == NULL )
			{

			#if DEBUG_MODE

				std::wstring errorMessage = L"[Load Error] ";
				errorMessage += Donya::MultiToWide( fileName );

				Donya::OutputDebugStr( errorMessage.c_str() );

			#endif // DEBUG_MODE

				return false;
			}
			// else

			pSoundHandles->insert( std::make_pair( id, handle ) );

			return true;
		}

		bool Play( int id )
		{
			// TODO:I want user can specify play mode(ex:loop).

			InitIfNullptr();

			size_t handle = GetHandleOrNull( id );
			if ( handle == NULL ) { return false; }
			// else
			return pAudio->Play( handle );
		}

		bool Pause( int id, bool isEnableForAll )
		{
			InitIfNullptr();

			size_t handle = GetHandleOrNull( id );
			if ( handle == NULL ) { return false; }
			// else
			return pAudio->Pause( handle, isEnableForAll );
		}

		bool Resume( int id, bool isEnableForAll, bool fromTheBeginning )
		{
			InitIfNullptr();

			size_t handle = GetHandleOrNull( id );
			if ( handle == NULL ) { return false; }
			// else
			return pAudio->Resume( handle, isEnableForAll, fromTheBeginning );
		}

		bool Stop( int id, bool isEnableForAll )
		{
			InitIfNullptr();

			size_t handle = GetHandleOrNull( id );
			if ( handle == NULL ) { return false; }
			// else
			return pAudio->Stop( handle, isEnableForAll );
		}

		bool SetVolume( int id, float volume, bool isEnableForAll )
		{
			InitIfNullptr();

			size_t handle = GetHandleOrNull( id );
			if ( handle == NULL ) { return false; }
			// else
			return pAudio->SetVolume( handle, volume, isEnableForAll );
		}

		bool AppendFadePoint( int id, float sec, float destVol, bool isEnableForAll )
		{
			InitIfNullptr();

			size_t handle = GetHandleOrNull( id );
			if ( handle == NULL ) { return false; }
			// else
			return pAudio->AppendFadePoint( handle, sec, destVol, isEnableForAll );
		}

		int  GetNowPlayingSoundsCount()
		{
			InitIfNullptr();

			return pAudio->GetNowPlayingSoundsCount();
		}
	}
}