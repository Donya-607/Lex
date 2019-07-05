#include "SkinnedMesh.hlsli"

Texture2D		diffuseMap		: register( t0 );
SamplerState	diffuseSampler	: register( s0 );

float4 main( VS_OUT pin ) : SV_TARGET
{
	return diffuseMap.Sample( diffuseSampler, pin.texCoord ) * pin.color;
}