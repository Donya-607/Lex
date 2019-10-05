#include "SkinnedMesh.hlsli"

static const float PI = 3.14159265359f;

// see http://www.project-asura.com/program/d3d11/d3d11_004.html

float3 NormalizedLambert( float3 diffuse, float3 normal, float3 lightDir )
{
	return diffuse * max( 0.0f, dot( normal, lightDir ) ) * ( 1.0f / PI );
}

Texture2D		diffuseMap		: register( t0 );
SamplerState	diffuseSampler	: register( s0 );

float4 main( VS_OUT pin ) : SV_TARGET
{
	float4 diffuseColor = diffuseMap.Sample( diffuseSampler, pin.texCoord );
	if ( diffuseColor.a <= 0.0f ) { discard; }
	// else

	float3 color = NormalizedLambert( mtlDiffuse.rgb, pin.normal.rgb, lightDir.rgb );
	color = ( color * 0.5f ) + 0.5f;

	float4 output;
	output.rgb	= color * diffuseColor.rgb;
	output.rgb	= saturate( output.rgb + mtlAmbient.rgb );
	output.a	= diffuseColor.a;

	return output * pin.color * lightColor;

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

	/*
	// make reference to http://marupeke296.com/DXPS_S_No4_DiffuseMap.html

	float  lightPower =
		saturate( dot( pin.normal, normalize( -lightDir ) ) );
	float4 surfaceColor =
		diffuseMap.Sample( diffuseSampler, pin.texCoord ) * lightColor * diffuse;

	float4 output = lightPower * surfaceColor;
	return output; // saturate( output + ambient );
	*/
}