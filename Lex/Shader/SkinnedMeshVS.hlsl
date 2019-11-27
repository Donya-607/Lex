#include "SkinnedMesh.hlsli"

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

void ApplyBoneMatrices( uint4 boneIndices, float4 boneWeights, inout float4 inoutPosition, inout float4 inoutNormal )
{
	const float4 inPosition	= inoutPosition;
	const float4 inNormal	= float4( inoutNormal.xyz, 0.0f );
	float3 resultPos		= { 0, 0, 0 };
	float3 resultNormal		= { 0, 0, 0 };

	float weight = 0;
	row_major float4x4 transform = 0;
	for ( int i = 0; i < 4/* float4 */; ++i )
	{
		weight			= boneWeights[i];
		transform		= cbBoneTransforms[boneIndices[i]];

		resultPos		+= ( weight * mul( inPosition,	transform ) ).xyz;
		resultNormal	+= ( weight * mul( inNormal,	transform ) ).xyz;
	}

	inoutPosition	= float4( resultPos,    1.0f );
	inoutNormal		= float4( resultNormal, 0.0f );
}

VS_OUT main( VS_IN vin )
{
	ApplyBoneMatrices( vin.bones, vin.weights, vin.pos, vin.normal );
	// vin.normal.w = 0; // This process already done at ApplyBoneMatrices().
	
	VS_OUT vout		= ( VS_OUT )( 0 );
	vout.pos		= mul( vin.pos, cbWorldViewProjection );
	vout.wsPos		= mul( vin.pos, cbWorld );
	vout.normal		= normalize( mul( vin.normal, cbWorld ) );
	vout.texCoord	= vin.texCoord;
	
	return vout;
}