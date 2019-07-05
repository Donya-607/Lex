struct VS_OUT
{
	float4 pos		: SV_POSITION;
	float4 color	: COLOR;
	float4 eyeVector: TEXCOORD0;
	float4 normal	: TEXCOORD1;
	float2 texCoord	: TEXCOORD2;
};

cbuffer CONSTANT_BUFFER : register( b0 )
{
	row_major float4x4	worldViewProjection;
	row_major float4x4	world;
	float4				eyePosition;
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