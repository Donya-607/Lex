#include "Model.hlsli"

struct VS_IN
{
	float4	pos			: POSITION;
	float4	normal		: NORMAL;
	float2	texCoord	: TEXCOORD0;
	float4	weights		: WEIGHTS;
	uint4	bones		: BONES;
};

static const uint MAX_BONE_COUNT = 64U;
cbuffer CBPerMesh : register( b2 )
{
	row_major
	float4x4	cbAdjustMatrix;
	row_major
	float4x4	cbBoneTransforms[MAX_BONE_COUNT];
};

void ApplyBoneMatrices( float4 boneWeights, uint4 boneIndices, inout float4 inoutPosition, inout float4 inoutNormal )
{
	const float4 inPosition	= float4( inoutPosition.xyz, 1.0f );
	const float4 inNormal	= float4( inoutNormal.xyz,   0.0f );
	float3 resultPos		= { 0, 0, 0 };
	float3 resultNormal		= { 0, 0, 0 };
	float  weight			= 0;
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
	vin.pos.w		= 1.0f;
	vin.normal.w	= 0.0f;
	ApplyBoneMatrices( vin.weights, vin.bones, vin.pos, vin.normal );

	float4x4 W		= mul( cbAdjustMatrix, cbWorld );
	float4x4 WVP	= mul( W, cbViewProj );

	VS_OUT vout		= ( VS_OUT )( 0 );
	vout.wsPos		= mul( vin.pos, W );
	vout.svPos		= mul( vin.pos, WVP );
	vout.normal		= normalize( mul( vin.normal, W ) );
	vout.texCoord	= vin.texCoord;
	return vout;
}
