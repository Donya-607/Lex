#include "SkinnedMesh.hlsli"

/*
cbuffer CONSTANT_BUFFER : register( b0 )
{
	row_major float4x4	worldViewProjection;
	row_major float4x4	world;
	float4				lightDirection;
	float4				materialColor;
};
*/

struct VS_IN
{
	float4 pos		: POSITION;
	float4 normal	: NORMAL;
};

VS_OUT main( float4 pos : POSITION, float4 normal : NORMAL )
{
	normal.w = 0;
	float4 norm		= normalize( mul( normal, world ) );
	float4 light	= normalize( -lightDirection );

	VS_OUT vout;
	vout.pos		= mul( pos, worldViewProjection );

	vout.color		= materialColor * max( dot( light, norm ), 0.0f );
	vout.color.a	= materialColor.a;

	return vout;
}