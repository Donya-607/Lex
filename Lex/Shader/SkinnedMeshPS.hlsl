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

Texture2D		diffuseMap		: register( t0 );
SamplerState	diffuseSampler	: register( s0 );

float4 main( VS_OUT pin ) : SV_TARGET
{
	/*
	float4 nNorm		= normalize( pin.normal );
	float4 nLightDir	= normalize( lightDirection );
	float4 nViewDir		= normalize( pin.eyeVector );
	float4 NL			= saturate( dot( nNorm, nLightDir ) );

	float4 nReflect		= normalize( ( 2 * NL * nNorm ) - nLightDir );
	float4 varSpecular	= pow( saturate( dot( nReflect, nViewDir ) ), 2 ) * 2;

	float4 phongShade	= diffuse * NL * varSpecular * specular;
	*/

	return diffuseMap.Sample( diffuseSampler, pin.texCoord ) 
		* pin.color
		// * materialColor
		// * phongShade
		;
}