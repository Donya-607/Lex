#include "Grid.h"

#include "Donya/Useful.h"
#include "Donya/UseImGui.h"

GridLine::GridLine() :
	lineYaw( 0.0f ), lineLength( 16.0f, 16.0f ),
	drawInterval( 0.5f, 0.5f ),
	line( /* masInstanceCount = */ MAX_LINE_COUNT )
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
		if ( ImGui::TreeNode( u8"グリッド線" ) )
		{
			ImGui::DragFloat ( u8"線の角度", &lineYaw );
			ImGui::DragFloat2( u8"線の半径", &lineLength.x,   0.5f  );
			ImGui::DragFloat2( u8"線の間隔", &drawInterval.x, 0.05f );

			const Donya::Int2 drawingCount
			{
				( ZeroEqual( drawInterval.x ) ) ? 1 : scast<int>( ( lineLength.x * 2.0f ) / drawInterval.x ) + 1/* Edge line*/,
				( ZeroEqual( drawInterval.y ) ) ? 1 : scast<int>( ( lineLength.y * 2.0f ) / drawInterval.y ) + 1/* Edge line*/
			};
			ImGui::Text( u8"本数：[Ｘ：%d][Ｚ：%d][計：%d]", drawingCount.x - 1, drawingCount.y - 1, drawingCount.x + drawingCount.y - 2 );
			ImGui::Text( u8"最大本数：[%d]", MAX_LINE_COUNT );

			ImGui::TreePop();
		}

		ImGui::End();
	}
#endif // USE_IMGUI
}

void GridLine::Draw( const Donya::Vector4x4 &matVP ) const
{
	const Donya::Quaternion rotation	= Donya::Quaternion::Make( Donya::Vector3::Up(), ToRadian( lineYaw ) );
	const Donya::Vector3 base			= rotation.RotateVector( { -lineLength.x, 0.0f, -lineLength.y } );
	const Donya::Vector3 startX			= /*rotation.RotateVector*/{ 0.0f, 0.0f, 0.0f };
	const Donya::Vector3 startZ			= /*rotation.RotateVector*/{ 0.0f, 0.0f, 0.0f };
	const Donya::Vector3 endX			= rotation.RotateVector( { lineLength.x * 2.0f, 0.0f, 0.0f } );
	const Donya::Vector3 endZ			= rotation.RotateVector( { 0.0f, 0.0f, lineLength.y * 2.0f } );
	const Donya::Vector3 offsetX		= rotation.RotateVector( { drawInterval.x, 0.0f, 0.0f } );
	const Donya::Vector3 offsetZ		= rotation.RotateVector( { 0.0f, 0.0f, drawInterval.y } );

	const Donya::Int2 loopCount
	{
		( ZeroEqual( drawInterval.x ) ) ? 1 : scast<int>( ( lineLength.x * 2.0f ) / drawInterval.x ) + 1/* Edge line*/,
		( ZeroEqual( drawInterval.y ) ) ? 1 : scast<int>( ( lineLength.y * 2.0f ) / drawInterval.y ) + 1/* Edge line*/
	};

	for ( int z = 0; z < loopCount.y; ++z )
	{
		line.Reserve
		(
			base + startX + ( offsetZ * scast<float>( z ) ),
			base + endX   + ( offsetZ * scast<float>( z ) )
		);
	}
	for ( int x = 0; x < loopCount.x; ++x )
	{
		line.Reserve
		(
			base + startZ + ( offsetX * scast<float>( x ) ),
			base + endZ   + ( offsetX * scast<float>( x ) )
		);
	}
	
	line.Flush( matVP );
}
