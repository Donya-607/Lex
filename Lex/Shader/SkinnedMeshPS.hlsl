#include "SkinnedMesh.hlsli"

float4 main( VS_OUT pin ) : SV_TARGET
{
	if ( pin.color.a <= 0 ) { discard; }
	return pin.color;
}