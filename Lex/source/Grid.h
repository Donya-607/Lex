#pragma once

#include "Donya/GeometricPrimitive.h"
#include "Donya/Vector.h"

class GridLine
{
private:
	static constexpr unsigned int MAX_LINE_COUNT = 512U;
private:
	float					lineYaw;		// Used to degree at Update(), used to radian at Draw().
	Donya::Vector2			lineLength;		// Half length. 'Y' is used to 'Z'.
	Donya::Vector2			drawInterval;	// 'Y' is used to 'Z'.
	Donya::Geometric::Line	line;
public:
	GridLine();
	~GridLine();
public:
	void Init();
	void Uninit();

	void Update();

	void Draw( const Donya::Vector4x4 &matViewProjection ) const;
};
