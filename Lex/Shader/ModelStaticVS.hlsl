#include "Model.hlsli"
#include "Techniques.hlsli"

struct VS_IN
{
	float4 pos		: POSITION;
	float4 normal	: NORMAL;
	float4 tangent	: TANGENT;
	float2 texCoord	: TEXCOORD0;
};

cbuffer CBPerMesh : register( b2 )
{
	row_major
	float4x4	cbAdjustMatrix;
};

VS_OUT main( VS_IN vin )
{
	vin.pos.w		= 1.0f;
	vin.normal.w	= 0.0f;
	vin.tangent.w	= 0.0f;

	float4x4 W		= mul( cbAdjustMatrix, cbWorld );
	float4x4 WV		= mul( W, cbView );
	float4x4 WVP	= mul( W, cbViewProj );

	VS_OUT vout		= ( VS_OUT )( 0 );
	vout.wsPos		= mul( vin.pos, W );
	vout.svPos		= mul( vin.pos, WVP );
	
	float3 vsNormal	= normalize( mul( vin.normal,  WV ).xyz );
	float3 vsTangent= normalize( mul( vin.tangent, WV ).xyz );
	float4x4 VT		= mul( cbView, MakeMatrixToTangentSpace( vsTangent, vsNormal ) );
	vout.tsLightVec	= normalize( mul( -cbDirLight.direction, VT ) );
	vout.tsEyeVec	= normalize( mul( cbEyePosition - vout.wsPos, VT ) );
	
	vout.texCoord	= vin.texCoord;
	return vout;
}
