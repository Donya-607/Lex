#include "Animation.h"

#include "Donya/Constant.h"
#include "Donya/Sprite.h"

SpriteSheet::SpriteSheet() :
	spriteSheet( NULL ),
	sheetSize(), partSize(),
	partCount()
{

}
SpriteSheet::~SpriteSheet()
{

}

void SpriteSheet::Init()
{
	spriteSheet	= NULL;
	sheetSize	= {};
	partSize	= {};
	partCount	= {};
}

bool SpriteSheet::LoadSheet( std::wstring filePath, size_t maxInstanceCount )
{
	spriteSheet = Donya::Sprite::Load( filePath, maxInstanceCount );
	
	Donya::Sprite::GetTextureSize( spriteSheet, &sheetSize.x, &sheetSize.y );

	return true;
}

void SpriteSheet::SetPartSize( int x, int y )
{
	SetPartSize( { x, y } );
}
void SpriteSheet::SetPartSize( Donya::Int2 size )
{
	partSize = size;
}
void SpriteSheet::SetPartCount( int x, int y )
{
	SetPartCount( { x, y } );
}
void SpriteSheet::SetPartCount( Donya::Int2 divCounts )
{
	partCount = divCounts;
}

Donya::Int2 SpriteSheet::CalcPlace( int divCount ) const
{
	int maxCount = partCount.x * partCount.y;
	if ( maxCount <= divCount ) { return Donya::Int2{}; }

	int x = divCount % partCount.x;
	int y = ( partCount.y == 1 ) ? 0 : scast<int>( divCount / partCount.x );

	return Donya::Int2{ x, y };
}



Anime::Anime() :
	SpriteSheet(),
	timer( 0 ), current( 0 ), maxSheets( 0 ), speed( 0 ),
	first( -1 ), last( -1 ),
	isDone( false ), isLoop( false )
{
	
}
Anime::Anime( int maxSheets, int speed, bool isLoop ) :
	SpriteSheet(),
	timer( 0 ), current( 0 ), maxSheets( maxSheets ), speed( speed ),
	first( -1 ), last( -1 ),
	isDone( false ), isLoop( isLoop )
{
	
}
Anime::Anime( int maxSheets, int speed, int rangeFirst, int rangeLast, bool isLoop ) :
	SpriteSheet(),
	timer( 0 ), current( 0 ), maxSheets( maxSheets ), speed( speed ),
	first( rangeFirst ), last( rangeLast ),
	isDone( false ), isLoop( isLoop )
{

}

Anime &Anime::operator = ( const SpriteSheet &sheet )
{
	spriteSheet	= sheet.GetSpriteIdentifier();
	sheetSize	= sheet.GetSheetSize();
	partSize	= sheet.GetPartSize();
	partCount	= sheet.GetPartCount();

	return *this;
}

void Anime::Update()
{
	if ( speed < 0 )			{ return; }
	if ( !isLoop && isDone )	{ return; }
	// else

	timer++;
	isDone = false;

	if ( timer < speed )		{ return; }
	// else

	timer = 0;

	Advance();
}

Donya::Int2 Anime::CalcTexturePartPos() const
{
	Donya::Int2 place = CalcPlace( Current() );
	Donya::Int2 size  = GetPartSize();

	return Donya::Int2{ place.x * size.x, place.y * size.y };
}

void Anime::Advance()
{
	int startValue = IsEnableRangeLoop() ? first : 0;
	int lastValue  = IsEnableRangeLoop() ? last  : maxSheets - 1;

	if ( isLoop )
	{
		current = ( lastValue <= current ) ? startValue : current + 1;
		if ( current == startValue )
		{
			isDone = true;
		}
	}
	else
	{
		current++;

		if ( lastValue < current )
		{
			current = lastValue;
			isDone  = true;
		}
	}
}
