#include "SkinnedMesh.hlsli"

struct VS_IN
{
	float4	pos		: POSITION;
	float4	normal	: NORMAL;
	float2	texCoord: TEXCOORD;
	uint4	bones	: BONES;
	float4	weights : WEIGHTS;
};

float4 VisualizeBoneInfluence( uint4 boneIndices, float4 weights )
{
	float4 influence = { 0.0f, 0.0f, 0.0f, 1.0f };
	for ( int i = 0; i < 4; ++i )
	{
		float weight = weights[i];
		if ( weight <= 0.0f ) { continue; }
		// else
		switch ( boneIndices[i] )
		{
		case 0: influence.r = weight; break;
		case 1: influence.g = weight; break;
		case 2: influence.b = weight; break;
		default: break;
		}
	}

	return influence;
}

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

	// vout.color = VisualizeBoneInfluence( vin.bones, vin.weights );
	vout.color.rgba = 1.0f;

	return vout;
}