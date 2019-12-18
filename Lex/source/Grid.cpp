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
		if ( ImGui::TreeNode( u8"���`��e�X�g" ) )
		{
			ImGui::DragInt( u8"�`��{��", &drawCount );
			ImGui::DragFloat( u8"�`��Ԋu", &drawInterval );

			ImGui::DragFloat3( u8"�n�_", &start.x );
			ImGui::DragFloat3( u8"�I�_", &end.x );

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
