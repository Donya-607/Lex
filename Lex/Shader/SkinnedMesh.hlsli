struct VS_OUT
{
	float4 pos		: SV_POSITION;
	float4 color	: COLOR;
	float4 normal	: NORMAL;
	float2 texCoord	: TEXCOORD1;
};

static const int MAX_BONE_COUNT = 32;

cbuffer CONSTANT_BUFFER : register( b0 )
{
	row_major float4x4	cbWorldViewProjection;
	row_major float4x4	cbWorld;
	row_major float4x4	cbBoneTransforms[MAX_BONE_COUNT];
	float4				cbLightColor;
	float4				cbLightDirection;
	float4				cbMaterialColor;
};

cbuffer MATERIAL_CONSTANT_BUFFER : register( b1 )
{
	float4	mtlAmbient;
	float4	mtlBump;
	float4	mtlDiffuse;
	float4	mtlEmissive;
	float4	mtlSpecular;
}