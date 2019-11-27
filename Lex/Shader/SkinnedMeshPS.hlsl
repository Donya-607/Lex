#include "SkinnedMesh.hlsli"
#include "Techniques.hlsli"

Texture2D		diffuseMap			: register( t0 );
SamplerState	diffuseMapSampler	: register( s0 );

float4 main( VS_OUT pin ) : SV_TARGET
{
	float3	nLightVec		= normalize( -cbLightDirection.rgb ); // "position -> light" vector.
	float4	nEyeVector		= pin.wsPos - cbEyePosition;

	float	diffuseFactor	= HalfLambert( pin.normal.rgb, nLightVec );
	//		diffuseFactor	= pow( diffuseFactor, 2.0f ); // If needed.
	float4	diffuseColor	= cbDiffuse * diffuseFactor;

	float	specularFactor	= Phong( pin.normal.rgb, nLightVec, -nEyeVector.rgb, cbSpecular.w );
	// float	specularFactor	= BlinnPhong( pin.normal.rgb, nLightDir, -nEyeVector.rgb, specular.w );
	float4	specularColor	= cbSpecular * specularFactor * cbLightColor;

	float4	sampleColor		= diffuseMap.Sample( diffuseMapSampler, pin.texCoord );
		
	float3	shadedColor		= sampleColor.rgb * diffuseColor.rgb;
			shadedColor		= saturate( shadedColor + cbAmbient.rgb + specularColor.rgb );
			// shadedColor		= saturate( shadedColor + cbAmbient.rgb );

	float3	lightColor		= cbLightColor.rgb * cbLightColor.w;
	float3	lightedColor	= shadedColor * lightColor;

	// float3	foggedColor		= AffectFog( lightedColor, eyePosition.rgb, pin.wsPos.rgb, fogNear, fogFar, fogColor.rgb );

	// float3	outputColor		= foggedColor;
	float3	outputColor		= lightedColor;
	
	return	float4( outputColor, sampleColor.a ) * cbMaterialColor;

	/*
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
	*/

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