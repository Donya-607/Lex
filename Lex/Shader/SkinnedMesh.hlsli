struct VS_IN
{
	float4	pos		: POSITION;
	float4	normal	: NORMAL;
	float2	texCoord: TEXCOORD;
	uint4	bones	: BONES;
	float4	weights : WEIGHTS;
};

struct VS_OUT
{
	float4 pos		: SV_POSITION;
	float4 wsPos	: POSITION;
	float4 normal	: NORMAL;
	float2 texCoord	: TEXCOORD1;
};

cbuffer CBPerFrame : register( b0 )
{
	float4	cbEyePosition;
	float4	cbLightColor;
	float4	cbLightDirection;
};

cbuffer CBPerModel : register( b1 )
{
	float4 cbMaterialColor;
};

static const int MAX_BONE_COUNT = 64;
cbuffer CBPerMesh : register( b2 )
{
	row_major float4x4	cbWorldViewProjection;
	row_major float4x4	cbWorld;
	row_major float4x4	cbBoneTransforms[MAX_BONE_COUNT];
};

cbuffer CBPerSubset : register( b3 )
{
	float4	cbAmbient;
	float4	cbBump;
	float4	cbDiffuse;
	float4	cbEmissive;
	float4	cbSpecular;
}