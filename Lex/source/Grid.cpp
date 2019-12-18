#include "Grid.h"

#include "Donya/UseImGui.h"

GridLine::GridLine() :
	drawCount( 1 ), drawInterval( 1.0f ),
	start( 0.0f, 0.0f, -0.5f ), end( 0.0f, 0.0f, 0.5f ),
	line()
{}
GridLine::~GridLine() = default;

void GridLine::Init()
{
	if ( !line.Init() )
	{
		_ASSERT_EXPR( 0, L"Failed : The line's initialize does not finished." );
	}
}
void GridLine::Uninit()
{
	line.Uninit();
}

void GridLine::Update()
{
#if USE_IMGUI
	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"線描画テスト" ) )
		{
			ImGui::DragInt( u8"描画本数", &drawCount );
			ImGui::DragFloat( u8"描画間隔", &drawInterval );

			ImGui::DragFloat3( u8"始点", &start.x );
			ImGui::DragFloat3( u8"終点", &end.x );

			ImGui::TreePop();
		}

		ImGui::End();
	}
#endif // USE_IMGUI
}

void GridLine::Draw( const Donya::Vector4x4 &matVP ) const
{
	const Donya::Vector3 offset{ drawInterval, 0.0f, 0.0f };
	for ( int i = 0; i < drawCount; ++i )
	{
		line.Reserve
		(
			start + ( offset * scast<float>( i ) ),
			end   + ( offset * scast<float>( i ) )
		);
	}
	
	line.Flush( matVP );
}
