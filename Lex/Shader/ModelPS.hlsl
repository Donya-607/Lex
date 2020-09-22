#include "Model.hlsli"
#include "Techniques.hlsli"

cbuffer CBPerSubset : register( b3 )
{
	float4	cbAmbient;
	float4	cbDiffuse;
	float4	cbSpecular;
	float4	cbEmissive;
};

float3 CalcLightInfluence( float4 lightColor, float3 nwsPixelToLightVec, float3 nwsPixelNormal, float3 nwsEyeVector )
{
	float3	ambientColor	= cbAmbient.rgb;
	
	float	diffuseFactor	= HalfLambert( nwsPixelNormal, nwsPixelToLightVec );
	//		diffuseFactor	= pow( diffuseFactor, 2.0f );
	float3	diffuseColor	= cbDiffuse.rgb * diffuseFactor;
	
	float	specularFactor	= max( 0.0f, Phong( nwsPixelNormal, nwsPixelToLightVec, nwsEyeVector, cbSpecular.w ) );
	float3	specularColor	= cbSpecular.rgb * specularFactor;

	float3	Ka				= ambientColor;
	float3	Kd				= diffuseColor;
	float3	Ks				= specularColor;
	float3	light			= lightColor.rgb * lightColor.w;
	return	Ka + ( ( Kd + Ks ) * light );
}

Texture2D		diffuseMap			: register( t0 );
Texture2D		normalMap			: register( t1 );
SamplerState	diffuseMapSampler	: register( s0 );
SamplerState	normalMapSampler	: register( s1 );

float4 main( VS_OUT pin ) : SV_TARGET
{
			pin.tsLightVec	= normalize( pin.tsLightVec	);
			pin.tsEyeVec	= normalize( pin.tsEyeVec	);
	
	float4	normalMapColor	= normalMap.Sample( normalMapSampler, pin.texCoord );
			normalMapColor	= SRGBToLinear( normalMapColor );
	float4	tsNormal		= float4( normalize( SampledToNormal( normalMapColor.xyz ) ), 0.0f );

	float4	diffuseMapColor	= diffuseMap.Sample( diffuseMapSampler, pin.texCoord );
			diffuseMapColor	= SRGBToLinear( diffuseMapColor );
	float	diffuseMapAlpha	= diffuseMapColor.a;

	float3	totalLight		= CalcLightInfluence( cbDirLight.color, pin.tsLightVec.xyz, tsNormal.xyz, pin.tsEyeVec.xyz );

	float3	resultColor		= diffuseMapColor.rgb * totalLight;
	float4	outputColor		= float4( resultColor, diffuseMapAlpha * cbDiffuse.a );
			outputColor		= outputColor * cbDrawColor;
	clip (	outputColor.a );
	return	LinearToSRGB( outputColor );
}
