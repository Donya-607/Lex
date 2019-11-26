#ifndef INCLUDED_DONYA_BLEND_H_
#define INCLUDED_DONYA_BLEND_H_

struct ID3D11Device;
struct ID3D11DeviceContext;
struct D3D11_BLEND_DESC;

namespace Donya
{
	namespace Blend
	{
		/// <summary>
		/// If you use "AlphaToCoverage" mode(not have "NO_ATC" postfix), you should use Donya::Color::FilteringAlpha() at specify alpha.
		/// </summary>
		enum class Mode : int
		{
			/*
			NO_BLEND	: ( RGB : dest * ( 1 - srcA ) + ( src * srcA ),	A : destA * ( 1 - srcA ) + srcA )
			ALPHA		: ( RGB : dest * ( 1 - srcA ) + ( src * srcA ),	A : destA * ( 1 - srcA ) + srcA )
			ADD			: ( RGB : dest				  + ( src * srcA ),	A : destA )
			SUB			: ( RGB : src * dest,							A : srcA * destA )
			MUL			: ( RGB : src * dest,							A : srcA * destA )
			SCREEN		: ( RGB : dest * ( 1 - src  ) + ( src * srcA ),	A : destA * ( 1 - srcA ) + srcA )
			LIGHTEN		: ( RGB : max( src, dest ),						A : max( srcA, destA ) )
			DARKEN		: ( RGB : min( src, dest ),						A : min( srcA, destA ) )
			*/

			NO_BLEND,
			ALPHA,
			ADD,
			SUB,
			MUL,
			SCREEN,
			LIGHTEN,
			DARKEN,

			ALPHA_NO_ATC,	// Disable "AlphaToCoverage" version.
			ADD_NO_ATC,		// Disable "AlphaToCoverage" version.
			SUB_NO_ATC,		// Disable "AlphaToCoverage" version.
			MUL_NO_ATC,		// Disable "AlphaToCoverage" version.
			SCREEN_NO_ATC,	// Disable "AlphaToCoverage" version.
			LIGHTEN_NO_ATC,	// Disable "AlphaToCoverage" version.
			DARKEN_NO_ATC,	// Disable "AlphaToCoverage" version.

			BLEND_MODE_COUNT
		};

		/// <summary>
		/// Create all library's blend state. The old states will be discard.
		/// If you set nullptr to "pDevice", use default(library's) device.<para></para>
		/// Returns false when failed a creation.
		/// </summary>
		bool Init( ID3D11Device *pDevice = nullptr );

		/// <summary>
		/// Create a user definition blend state.<para></para>
		/// Returns false when failed the creation.
		/// </summary>
		bool CreateExternalBlendState( const D3D11_BLEND_DESC &useDefinitionBlendDesc, int identifier, bool enableAlphaToCoverage, ID3D11Device *pDevice = nullptr );

		/// <summary>
		/// Activate the library's blend state.<para></para>
		/// If you set nullptr to "pImmediateContext", use default(library's) device.
		/// </summary>
		void Activate( Mode blendMode, ID3D11DeviceContext *pImmediateContext = nullptr );

		/// <summary>
		/// Activate a user definition blend state.<para></para>
		/// "identifier" is the identifier of used in CreateExternalBlendState().<para></para>
		/// If you set nullptr to "pImmediateContext", use default(library's) device.<para></para>
		/// Returns false when the 'identifier" was not found.
		/// </summary>
		bool ActivateExternal( int identifier, ID3D11DeviceContext *pImmediateContext = nullptr );

		/// <summary>
		/// Returns current blend mode. But if now settings user definition blend state, returns Mode::BLEND_MODE_COUNT.
		/// </summary>
		Mode CurrentMode();

		/// <summary>
		/// Returns true if when enabled "AlphaToCoverage".
		/// </summary>
		bool IsEnabledATC();
	}
}

#endif // INCLUDED_DONYA_BLEND_H_