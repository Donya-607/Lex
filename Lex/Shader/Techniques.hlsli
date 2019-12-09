static const float PI = 3.14159265359f;

// Calculate diffuse reflection.
// Argument.nwsNormal : The normal of normalized world space.
// Argument.nwsToLightVec : The light vector of normalized world space. this vector is "position -> light".
// Returns : Diffuse factor of normalized in 0.0f ~ 1.0f.
float Lambert( float3 nwsNormal, float3 nwsToLightVec )
{
	return max( 0.0f, dot( nwsNormal, nwsToLightVec ) );
}
// Calculate half-lambert.
// Argument.nwsNormal : The normal of normalized world space.
// Argument.nwsToLightVec : The light vector of normalized world space. this vector is "position -> light".
// Returns : Diffuse factor. 0.5f ~ 1.0f.
float HalfLambert( float3 nwsNormal, float3 nwsToLightVec )
{
	float lambert = dot( nwsNormal, nwsToLightVec );
	return ( lambert * 0.5f ) + 0.5f;
}
// see http://www.project-asura.com/program/d3d11/d3d11_004.html
// Argument.diffuseColor : The diffuse color.
// Argument.nwsNormal : The normal of normalized world space.
// Argument.nwsToLightVec : The light vector of normalized world space. this vector is "position -> light".
// Returns : Diffuse color of according to the energy conservation law.
float3 NormalizedLambert( float3 diffuseColor, float3 nwsNormal, float3 nwsToLightVec )
{
	return diffuseColor * Lambert( nwsNormal, nwsToLightVec ) * ( 1.0f / PI );
}
// NormalizedLambert() of using HalfLambert(). see http://www.project-asura.com/program/d3d11/d3d11_004.html
// Argument.diffuseColor : The diffuse color.
// Argument.nwsNormal : The normal of normalized world space.
// Argument.nwsToLightVec : The light vector of normalized world space. this vector is "position -> light".
// Returns : Diffuse color of according to the energy conservation law.
float3 NormalizedHalfLambert( float3 diffuseColor, float3 nwsNormal, float3 nwsToLightVec )
{
	return diffuseColor * HalfLambert( nwsNormal, nwsToLightVec ) * ( 1.0f / PI );
}

// Calculate specular.
// Argument.nwsNormal : The normal of normalized world space.
// Argument.nwsToLightVec : The light vector of normalized world space. this vector is "position -> light".
// Argument.nwsToEyeVec : The eye vector of normalized world space. this vector is "position -> eye".
// Argument.specularPower : The specular's factor. will be power to return value.
// Returns : Specular factor. 0.0f ~ 1.0f.
float Phong( float3 nwsNormal, float3 nwsToLightVec, float3 nwsToEyeVec, float specularPower )
{
	// float3 nwsProjection = ( dot( nwsNormal, nwsToLightVec ) * nwsNormal );
	// float3 nwsReflection = nwsToLightVec + ( nwsToLightVec - ( nwsProjection * 2.0f ) );
	float3 nwsReflection  = normalize( reflect( -nwsToLightVec, nwsNormal ) );
	float  specularFactor = max( 0.0f, dot( nwsToEyeVec, nwsReflection ) );
	return pow( specularFactor, specularPower );
}
// Calculate specular by half vector.
// Argument.nwsNormal : The normal of normalized world space.
// Argument.nwsToLightVec : The light vector of normalized world space. this vector is "position -> light".
// Argument.nwsToEyeVec : The eye vector of normalized world space. this vector is "position -> eye".
// Argument.specularPower : The specular's factor. will be power to return value.
// Returns : Specular factor. 0.0f ~ 1.0f.
float BlinnPhong( float3 nwsNormal, float3 nwsToLightVec, float3 nwsToEyeVec, float specularPower )
{
	float3 nwsHalfVec = normalize( nwsToEyeVec + nwsToLightVec );
	return pow( dot( nwsHalfVec, nwsNormal ), specularPower );
}

// Calculate fog effect.
// Argument.affectColor : The color.
// Argument.wsEyePos : The position of eye(like camera) in world space.
// Argument.wsPos : The position of including "affectColor".
// Argument.wsNear : The near distance in world space. if the distance is less-equal than the near, the fog does not affecting.
// Argument.wsFar : The far distance in world space. if the distance is greater-equal than the far, the returns color will affected completely.
// Argument.fogColor : The color of fog. the affect color.
// Returns : Affected color by fog.
float3 AffectFog( float3 affectColor, float3 wsEyePos, float3 wsPos, float wsNear, float wsFar, float3 fogColor = { 1.0f, 1.0f, 1.0f } )
{
	float distance	= length( wsPos - wsEyePos );
	float alpha		= saturate( ( distance - wsNear ) / ( wsFar - wsNear ) );
	return ( affectColor * ( 1.0f - alpha ) ) + ( fogColor * alpha );
}
