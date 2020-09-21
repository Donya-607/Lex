struct VS_OUT
{
	float4		svPos		: SV_POSITION;
	float4		wsPos		: POSITION;
	float4		tsLightVec	: NORMAL0;	// (vertex->light) vector in tangent space
	float4		tsEyeVec	: NORMAL1;	// (vertex->camera) vector in tangent space
	float2		texCoord	: TEXCOORD0;
};

struct DirectionalLight
{
	float4		color;
	float4		direction;
};
cbuffer CBPerScene : register( b0 )
{
	DirectionalLight cbDirLight;	// World space
	float4		cbEyePosition;		// World space
	row_major
	float4x4	cbView;
	row_major
	float4x4	cbViewProj;
};

cbuffer CBPerModel : register( b1 )
{
	float4		cbDrawColor;
	row_major
	float4x4	cbWorld;
};

