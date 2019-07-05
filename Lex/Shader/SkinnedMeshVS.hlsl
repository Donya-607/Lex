#include "SkinnedMesh.hlsli"

/*
cbuffer CONSTANT_BUFFER : register( b0 )
{
	row_major float4x4	worldViewProjection;
	row_major float4x4	world;
	float4				lightDirection;
	float4				materialColor;
};

cbuffer MATERIAL_CONSTANT_BUFFER : register( b1 )
{
	float4	ambient;
	float4	bump;
	float4	diffuse;
	float4	emissive;
	float4	specular;
	float	shininess;
}
*/

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
	float4 nLight	= normalize( -lightDirection );

	VS_OUT vout;
	vout.pos		= mul( vin.pos, worldViewProjection );

	// vout.color		= ( diffuse * materialColor ) * max( dot( nLight, nNorm ), 0.0f ); // Lambert's cosine law
	vout.color		= materialColor * max( dot( nLight, nNorm ), 0.0f );
	vout.color.a	= materialColor.a;

	vout.eyeVector	= eyePosition - normalize( vout.pos );

	vout.normal		= nNorm;

	vout.texCoord	= vin.texCoord;

	return vout;
}