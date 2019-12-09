#include "SkinnedMesh.hlsli"
#include "Techniques.hlsli"

Texture2D		diffuseMap			: register( t0 );
SamplerState	diffuseMapSampler	: register( s0 );

float4 main( VS_OUT pin ) : SV_TARGET
{
	float4	diffuseMapColor	= diffuseMap.Sample( diffuseMapSampler, pin.texCoord );
	float	diffuseMapAlpha = diffuseMapColor.a;
	
	float3	nLightVec		= normalize( -cbLightDirection.rgb );	// "position -> light" vector.
	float4	nEyeVector		= cbEyePosition - pin.wsPos;			// "position -> eye" vector.
	
	float4	ambientColor	= cbAmbient; // TODO : Will be apply an ambient-map, anything else.

	float	diffuseFactor	= HalfLambert( pin.normal.rgb, nLightVec );
	//		diffuseFactor	= pow( diffuseFactor, 2.0f ); // If needed.
	float4	diffuseColor	= cbDiffuse * diffuseFactor;

	float	specularFactor	= Phong( pin.normal.rgb, nLightVec, nEyeVector.rgb, cbSpecular.w );
	// float	specularFactor	= BlinnPhong( pin.normal.rgb, nLightVec, nEyeVector.rgb, cbSpecular.w );
	float4	specularColor	= cbSpecular * specularFactor;

	float3	Ka				= ambientColor.rgb;
	float3	Kd				= diffuseMapColor.rgb * diffuseColor.rgb;
	float3	Ks				= specularColor.rgb;
	float3	lightColor		= cbLightColor.rgb * cbLightColor.w;
	
	float3	outputColor		= Ka + ( ( Kd + Ks ) * lightColor );
	//		outputColor		= AffectFog( outputColor, eyePosition.rgb, pin.wsPos.rgb, fogNear, fogFar, fogColor.rgb );

	return	float4( outputColor, diffuseMapAlpha ) * cbMaterialColor;
}