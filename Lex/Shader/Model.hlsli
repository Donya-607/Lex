struct VS_OUT
{
	float4		svPos		: SV_POSITION;
	float4		wsPos		: POSITION;
	float4		normal		: NORMAL;
	float2		texCoord	: TEXCOORD0;
};

struct DirectionalLight
{
	float4		color;
	float4		direction;
};
cbuffer CBPerScene : register( b0 )
{
	DirectionalLight cbDirLight;
	float4		cbEyePosition;
	row_major
	float4x4	cbViewProj;
};

cbuffer CBPerModel : register( b1 )
{
	float4		cbDrawColor;
	row_major
	float4x4	cbWorld;
};

cbuffer CBPerSubset : register( b3 )
{
	float4		cbAmbient;
	float4		cbDiffuse;
	float4		cbSpecular;
};
