#ifndef _INCLUDED_DONYA_H_
#define _INCLUDED_DONYA_H_

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace Donya
{
	/// <summary>
	/// <para></para>
	/// This Function has not completely implemented yet.<para></para>
	/// <para></para>
	/// Doing CoInitializeEx( COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE ).<para></para>
	/// If initialize failed, returns false.<para></para>
	/// In release mode, the isAppendFPS flag fixed to false.
	/// </summary>
	bool Init( const char *windowCaption, bool isAppendFPS = true );

	/// <summary>
	/// <para></para>
	/// This Function has not implemented yet.<para></para>
	/// <para></para>
	/// Please call in game-loop.<para></para>
	/// If I received WM_QUIT, returns false.
	/// </summary>
	bool MessageLoop();

	/// <para></para>
	/// This Function has not implemented yet.<para></para>
	/// <para></para>
	void Uninit();

	ID3D11Device		*GetDevice();
	ID3D11DeviceContext	*GetImmediateContext();
}

#endif // !_INCLUDED_DONYA_H_
