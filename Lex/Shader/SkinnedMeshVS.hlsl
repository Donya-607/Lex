#include "SkinnedMesh.hlsli"

struct VS_IN
{
	float4 pos		: POSITION;
	float4 normal	: NORMAL;
	float2 texCoord	: TEXCOORD;
};

VS_OUT main( VS_IN vin )
{
	vin.normal.w = 0;
	float4 nNorm	= normalize( mul( vin.normal, world ) );
	float4 nLight	= normalize( lightDir );

	VS_OUT vout = (VS_OUT)( 0 );
	vout.pos		= mul( vin.pos, worldViewProjection );

	// vout.color		= diffuse * max( dot( -nLight, nNorm ), 0.0f ); // Lambert's cosine law
	// vout.color.a	= 1.0f;

	// vout.eyeVector	= eyePosition - normalize( vout.pos );

	vout.normal		= nNorm;

	vout.texCoord	= vin.texCoord;

	return vout;
}