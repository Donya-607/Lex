#pragma once

#include <string>

#include "Donya/Useful.h"

class SpriteSheet
{
protected:
	size_t		spriteSheet;	// Sprite source.
	Donya::Int2	sheetSize;		// Whole-size of sprite-sheet.
	Donya::Int2	partSize;		// Whole-size of specified part size.
	Donya::Int2	partCount;		// Division count. 1-based.
public:
	SpriteSheet();
	virtual ~SpriteSheet();
public:
	void Init();

	/// <summary>
	/// Returns false if load failed.<para></para>
	/// "maxInstanceCount" is max count of concurrently drawable.
	/// </summary>
	bool LoadSheet( std::wstring filePath, size_t maxInstanceCount = 32U );
public:
	/// <summary>
	/// Size of part. Whole-size.
	/// </summary>
	void SetPartSize( int x, int y );
	/// <summary>
	/// Size of part. Whole-size.
	/// </summary>
	void SetPartSize( Donya::Int2 size );
	/// <summary>
	/// Division count. 1-based.
	/// </summary>
	void SetPartCount( int x, int y );
	/// <summary>
	/// Division count. 1-based.
	/// </summary>
	void SetPartCount( Donya::Int2 divCounts );
public:
	size_t GetSpriteIdentifier() const { return spriteSheet; }

	/// <summary>
	/// Calculate order:<para></para>
	/// Scan from left to right.<para></para>
	/// Scan from up to down.<para></para>
	/// If "divCount" over max-count of specified devision count, returns { 0, 0 }.
	/// </summary>
	Donya::Int2 CalcPlace( int divCount ) const;

	Donya::Int2 GetSheetSize() const { return sheetSize; }
	Donya::Int2 GetPartSize()  const { return partSize;  }
	Donya::Int2 GetPartCount() const { return partCount; }
};

class Anime : public SpriteSheet
{
private:
	int			timer;
	int			current;	// Current number of sheets.
	int			maxSheets;	// Number of sheets.
	int			speed;		// Per frame.
	int			first;		// Playing range, contain this count. if this is -1, the ranged loop is not enable.
	int			last;		// Playing range, contain this count. if this is -1, the ranged loop is not enable.
	bool		isDone;		// Timing of loop the anime, or end of anime.
	bool		isLoop;
public:
	Anime();
	Anime( int maxSheets, int speed, bool isLoop );
	Anime( int maxSheets, int speed, int rangeFirst, int rangeLast, bool isLoop );
	Anime &operator = ( const SpriteSheet &sheet );
public:
	void Update();
public:
	/// <summary>
	/// Returns current number of sheets.
	/// </summary>
	int Current() const { return current; }
	/// <summary>
	/// If "speedPerFrame" less than 0, we not play animation.
	/// </summary>
	void Reset( int numberOfSheets, int speedPerFrame, bool isEnableLoop, bool isRestartBeginning = true )
	{
		maxSheets	= numberOfSheets;
		speed		= speedPerFrame;
		first		= -1;
		last		= -1;
		isLoop		= isEnableLoop;

		if ( isRestartBeginning ) { current = 0; }
	}
	/// <summary>
	/// If "speedPerFrame" less than 0, we not play animation.
	/// </summary>
	void Reset( int numberOfSheets, int speedPerFrame, int rangeFirst, int rangeLast, bool isEnableLoop, bool isRestartBeginning = true )
	{
		maxSheets	= numberOfSheets;
		speed		= speedPerFrame;
		first		= rangeFirst;
		last		= rangeLast;
		isLoop		= isEnableLoop;

		if ( isRestartBeginning )
		{
			current = first;
		}
		else if ( current < first || last < current )
		{
			current = first;
		}

		if ( current < 0 ) { current = 0; }
	}
	bool IsDone() const { return isDone; }
	bool IsEnableRangeLoop() const
	{
		return ( first == -1 || last == -1 ) ? false : true;
	}
public:
	Donya::Int2 CalcTexturePartPos() const;
private:
	void Advance();
};