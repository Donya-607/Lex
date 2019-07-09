#include "SkinnedMesh.hlsli"

Texture2D		diffuseMap		: register( t0 );
SamplerState	diffuseSampler	: register( s0 );

float4 main( VS_OUT pin ) : SV_TARGET
{
	/*
	float4 nNorm		= normalize( pin.normal );
	float4 nLightDir	= normalize( lightDir );
	float4 nViewDir		= normalize( pin.eyeVector );
	float4 NL			= saturate( dot( nNorm, nLightDir ) );

	float4 nReflect		= normalize( ( 2 * NL * nNorm ) - nLightDir );
	float4 varSpecular	= pow( saturate( dot( nReflect, nViewDir ) ), 2 ) * 2;

	float4 phongShade	= diffuse * NL * varSpecular * specular;

	float4 color		= diffuseMap.Sample( diffuseSampler, pin.texCoord );
	color *= pin.color * materialColor;
	return color;
	*/

	// make reference to http://marupeke296.com/DXPS_S_No4_DiffuseMap.html

	float	lightPower =
		saturate( dot( pin.normal, normalize( lightDir ) ) );
	float4	surfaceColor =
		diffuseMap.Sample( diffuseSampler, pin.texCoord ) * lightColor;

	return lightPower * surfaceColor * diffuse;
}