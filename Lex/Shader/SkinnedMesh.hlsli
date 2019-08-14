struct VS_OUT
{
	float4 pos		: SV_POSITION;
	float4 color	: COLOR;
	float4 normal	: NORMAL;
	float2 texCoord	: TEXCOORD1;
};

cbuffer CONSTANT_BUFFER : register( b0 )
{
	row_major float4x4	worldViewProjection;
	row_major float4x4	world;
	float4				lightColor;
	float4				lightDir;
};

cbuffer MATERIAL_CONSTANT_BUFFER : register( b1 )
{
	float4	mtlAmbient;
	float4	mtlBump;
	float4	mtlDiffuse;
	float4	mtlEmissive;
	float4	mtlSpecular;
}