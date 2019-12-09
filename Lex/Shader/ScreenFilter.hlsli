// These function returns value may be out of range of [0.0f ~ 1.0f].

float3 AddBrightEffect( float3 input, float brightness )
{
	return input + brightness;
}
float3 AddContrastEffect( float3 input, float contrast )
{
	return ( ( input - 0.5f ) * contrast ) + 0.5f;
}
float3 AddSaturateEffect( float3 input, float saturate )
{
	float average = ( input.r + input.g + input.b ) / 3.0f;
	return ( ( input - average ) * saturate ) + average;
}
float3 AddColorToneEffect( float3 input, float3 colorTone )
{
	return input * colorTone;
}
float3 AddScreenFilter( float3 input, float brightness, float contrast, float saturate, float3 colorTone )
{
	input = AddBrightEffect   ( input, brightness );
	input = AddContrastEffect ( input, contrast   );
	input = AddSaturateEffect ( input, saturate   );
	input = AddColorToneEffect( input, colorTone  );
	return input;
}