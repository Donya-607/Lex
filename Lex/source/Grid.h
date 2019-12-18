#pragma once

#include "Donya/GeometricPrimitive.h"
#include "Donya/Vector.h"

class GridLine
{
private:
	int				drawCount;
	float			drawInterval;
	Donya::Vector3	start;
	Donya::Vector3	end;
	Donya::Geometric::Line line;
public:
	GridLine();
	~GridLine();
public:
	void Init();
	void Uninit();

	void Update();

	void Draw( const Donya::Vector4x4 &matViewProjection ) const;
};
