struct VS_OUT
{
	float4 pos		: SV_POSITION;
	float4 color	: COLOR;
	float2 texCoord	: TEXCOORD;
};

cbuffer CONSTANT_BUFFER : register( b0 )
{
	row_major float4x4	worldViewProjection;
	row_major float4x4	world;
	float4				lightDirection;
	float4				materialColor;
};