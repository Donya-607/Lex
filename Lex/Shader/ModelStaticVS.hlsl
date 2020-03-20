#include "Model.hlsli"

struct VS_IN
{
	float4 pos		: POSITION;
	float4 normal	: NORMAL;
	float2 texCoord	: TEXCOORD0;
};

cbuffer CBPerMesh : register( b2 )
{
	row_major
	float4x4	cbAdjustMatrix;
};

VS_OUT main( VS_IN vin )
{
	vin.pos.w		= 1.0f;
	vin.normal.w	= 0.0f;

	float4x4 W		= mul( cbAdjustMatrix, cbWorld );
	float4x4 WVP	= mul( W, cbViewProj );

	VS_OUT vout		= ( VS_OUT )( 0 );
	vout.wsPos		= mul( vin.pos, W );
	vout.svPos		= mul( vin.pos, WVP );
	vout.normal		= normalize( mul( vin.normal, W ) );
	vout.texCoord	= vin.texCoord;
	return vout;
}
