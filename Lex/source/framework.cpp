#include "Framework.h"

#include <array>
#include <algorithm>
#include <thread>

#include "Donya/Blend.h"
#include "Donya/Donya.h"
#include "Donya/Keyboard.h"
#include "Donya/Sound.h"
#include "Donya/UseImGui.h"
#include "Donya/Useful.h"
#include "Donya/WindowsUtil.h"
 
#include "Camera.h"
#include "Common.h"
#include "Loader.h"
#include "Music.h"

using namespace DirectX;

Framework::Framework() :
	pSceneMng( nullptr )
{}
Framework::~Framework() = default;

bool Framework::Init()
{
	bool  loadResult = LoadSounds();
	if ( !loadResult )
	{
		_ASSERT_EXPR( 0, L"Failed : One or more sounds were not loaded." );
		return false;
	}

	pSceneMng = std::make_unique<SceneMng>();
	pSceneMng->Init( Scene::Type::Lex );

	return true;
}

void Framework::Uninit()
{
	pSceneMng->Uninit();
}

void Framework::Update( float elapsedTime /*Elapsed seconds from last frame*/ )
{
#if DEBUG_MODE

	DebugShowInformation();

#endif // DEBUG_MODE

	pSceneMng->Update( elapsedTime );
}

void Framework::Draw( float elapsedTime/*Elapsed seconds from last frame*/ )
{
	Donya::Blend::Activate( Donya::Blend::Mode::ALPHA );

	pSceneMng->Draw( elapsedTime );
}

bool Framework::LoadSounds()
{
	using Donya::Sound::Load;
	using Music::ID;

	struct Bundle
	{
		ID id;
		std::string fileName;
		bool isEnableLoop;
	public:
		Bundle() : id(), fileName(), isEnableLoop( false ) {}
		Bundle( ID id, const char *fileName, bool isEnableLoop ) : id( id ), fileName( fileName ), isEnableLoop( isEnableLoop ) {}
		Bundle( ID id, const std::string &fileName, bool isEnableLoop ) : id( id ), fileName( fileName ), isEnableLoop( isEnableLoop ) {}
	};

	const std::array<Bundle, ID::MUSIC_COUNT> bandles =
	{
		{	// ID, FilePath, isEnableLoop
			// { ID::BGM,					"./Data/Sounds/BGM/Title.wav",				true  },
			
			{ ID::ItemChoose,			"./Data/Sounds/SE/UI/Choose.wav",			false },
			{ ID::ItemDecision,			"./Data/Sounds/SE/UI/Decision.wav",			false },
		},
	};

	bool result = true, successed = true;

	for ( size_t i = 0; i < ID::MUSIC_COUNT; ++i )
	{
		result = Load( bandles[i].id, bandles[i].fileName.c_str(), bandles[i].isEnableLoop );
		if ( !result ) { successed = false; }
	}

	return successed;
}

#if USE_IMGUI
#include "Donya/Easing.h"
#endif // USE_IMGUI
void Framework::DebugShowInformation()
{
#if USE_IMGUI

#if SHOW_EASING_BEHAVIORS

	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( "Easing Test" ) )
		{
			using namespace Donya::Easing;

			static float time = 0.0f;
			ImGui::SliderFloat( u8"ŽžŠÔ", &time, 0.0f, 1.0f );
			ImGui::Text( "" );
			static Type type = Type::In;
			{
				int iType = scast<int>( type );

				std::string caption = "Type : ";
				if ( type == Type::In    ) { caption += "In";    }
				if ( type == Type::Out   ) { caption += "Out";   }
				if ( type == Type::InOut ) { caption += "InOut"; }

				ImGui::SliderInt( caption.c_str(), &iType, 0, 2 );

				type = scast<Type>( iType );
			}

			constexpr int EASING_KIND_COUNT = GetKindCount();
			for ( unsigned int i = 0; i < EASING_KIND_COUNT; ++i )
			{
				float result = Ease( scast<Kind>( i ), type, time );
				ImGui::SliderFloat( KindName( i ), &result, -2.0f, 2.0f );
			}

			ImGui::TreePop();
		}

		ImGui::End();
	}
#endif // SHOW_EASING_BEHAVIORS

#endif // USE_IMGUI
}
