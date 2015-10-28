#pragma once
#include "interface.h"
#include "hapcodec.h"
#include <commctrl.h>
#include <shellapi.h>
#include <Windowsx.h>
#include <intrin.h>

#include <hap/hap.h>
#define return_badformat() return (DWORD)ICERR_BADFORMAT;
//#define return_badformat() { char msg[256];sprintf(msg,"Returning error on line %d", __LINE__);MessageBox (HWND_DESKTOP, msg, "Error", MB_OK | MB_ICONEXCLAMATION); return (DWORD)ICERR_BADFORMAT; }

// Test for MMX, SSE, SSE2, and SSSE3 support
/*bool DetectFlags(){
	int CPUInfo[4];
	__cpuid(CPUInfo,1);
	//SSE3 = (CPUInfo[2]&(1<< 0))!=0;
	SSSE3= (CPUInfo[2]&(1<< 9))!=0;
#ifndef X64_BUILD
	SSE  = (CPUInfo[3]&(1<<25))!=0;
	SSE2 = (CPUInfo[3]&(1<<26))!=0;
	return (CPUInfo[3]&(1<<23))!=0;
#else
	return true;
#endif	
}*/

extern BOOL CALLBACK ConfigureDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern void StoreRegistrySettings(bool nullframes, bool useSnappy, int dxtQuality);
extern void LoadRegistrySettings(bool* nullFrames, bool* useSnappy, int* dxtQuality);

CodecInst* g_currentUICodec = NULL;
HMODULE hmoduleHap = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD, LPVOID) 
{
	hmoduleHap = (HMODULE) hinstDLL;
	return TRUE;
}

CodecInst::CodecInst(CodecInst::CodecType codecType)
{
#ifndef LAGARITH_RELEASE
	if (_isStarted == 0x1337)
	{
		char msg[128];
		sprintf_s(msg,128,"Attempting to instantiate a codec instance that has not been destroyed");
		MessageBox (HWND_DESKTOP, msg, "Error", MB_OK | MB_ICONEXCLAMATION);
	}
#endif

	_in = NULL;
	_out = NULL;

	_buffer = NULL;
	_buffer2 = NULL;
	_prevBuffer = NULL;
	_dxtBuffer = NULL;
	_length = 0;
	_dxtBufferSize = 0;
	_dxtFlags = 0;

	_width = _height = _format = 0;

	_codecType = codecType;
	_useAlpha = false;
	_useNullFrames = false;
	_useSnappy = true;

	_isStarted = 0;
	_supportUncompressedOutput = true;
}

BOOL CodecInst::QueryConfigure() 
{
	return TRUE; 
}

DWORD CodecInst::About(HWND hwnd) 
{
	g_currentUICodec = this;
	DialogBox(hmoduleHap, MAKEINTRESOURCE(IDD_DIALOG2), hwnd, (DLGPROC)ConfigureDialogProc);
	g_currentUICodec = NULL;
	return ICERR_OK;
}

DWORD CodecInst::Configure(HWND hwnd) 
{
	g_currentUICodec = this;
	DialogBox(hmoduleHap, MAKEINTRESOURCE(IDD_DIALOG1), hwnd, (DLGPROC)ConfigureDialogProc);
	g_currentUICodec = NULL;
	return ICERR_OK;
}

CodecInst* Open(ICOPEN* icinfo, CodecInst::CodecType codecType) 
{
	if (icinfo && icinfo->fccType != ICTYPE_VIDEO)
		return NULL;

	CodecInst* pinst = new CodecInst(codecType);

	if (icinfo) icinfo->dwError = pinst ? ICERR_OK : ICERR_MEMORY;

	return pinst;
}

CodecInst::~CodecInst()
{
	try 
	{
		if ( _isStarted == 0x1337 )
		{
			CompressEnd();
			//DecompressEnd();
		}
		_isStarted =0;
	} catch ( ... ) {};
}

DWORD Close(CodecInst* pinst) 
{
	try 
	{
		if ( pinst && !IsBadWritePtr(pinst,sizeof(CodecInst)) )
		{
			delete pinst;
		}
	} catch ( ... ){};
    return 1;
}

// Ignore attempts to set/get the codec state but return successful.
// We do not want Lagarith settings to be application specific, but
// some programs assume that the codec is not configurable if GetState
// and SetState are not supported.
DWORD CodecInst::GetState(LPVOID pv, DWORD dwSize)
{
	if ( pv == NULL )
	{
		return 1;
	} 
	else if ( dwSize < 1 )
	{
		return ICERR_BADSIZE;
	}
	memset(pv,0,1);
	return 1;
}

// See GetState comment
DWORD CodecInst::SetState(LPVOID pv, DWORD dwSize)
{
	if ( pv )
	{
		return ICERR_OK;
	} 
	else 
	{
		return 1;
	}
}

// check if the codec can compress the given format to the desired format
DWORD CodecInst::CompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
	// check for valid format
	if (lpbiIn->biCompression != 0)
	{
		return_badformat()
	}

	// check for valid bitdepth
	if (_codecType == Hap)
	{
		if (lpbiIn->biBitCount != 32 && lpbiIn->biBitCount != 24)
		{
			return_badformat()
		}
	}
	if (_codecType == HapAlpha)
	{
		if (lpbiIn->biBitCount != 32)
		{
			return_badformat()
		}
	}
	if (_codecType == HapQ)
	{
		if (lpbiIn->biBitCount != 32 && lpbiIn->biBitCount != 24)
		{
			return_badformat()
		}
	}


	// Make sure width and height is multiple of 4
	if ( lpbiIn->biWidth%4 != 0 ||
		abs(lpbiIn->biHeight)%4 != 0)
	{
		return_badformat()
	}

	LoadRegistrySettings(&_useNullFrames, &_useSnappy, &_dxtQuality);

	// See if the output format is acceptable if need be
	if ( lpbiOut )
	{
		if ( lpbiOut->biSize < sizeof(BITMAPINFOHEADER) )
			return_badformat()

		if ( lpbiOut->biBitCount != 24 && lpbiOut->biBitCount != 32)
			return_badformat();
		if ( abs(lpbiIn->biHeight) != abs(lpbiOut->biHeight) )
			return_badformat();
		if ( lpbiIn->biWidth != lpbiOut->biWidth )
			return_badformat();
		if ( _codecType == Hap && lpbiOut->biCompression != FOURCC_HAP)
			return_badformat();
		if ( _codecType == HapAlpha && lpbiOut->biCompression != FOURCC_HAPA)
			return_badformat();
		if ( _codecType == HapQ && lpbiOut->biCompression != FOURCC_HAPQ)
			return_badformat();
		if ( _useAlpha && lpbiIn->biBitCount == 32 && lpbiIn->biBitCount != lpbiOut->biBitCount )
			return_badformat();
	}

	/*if ( !DetectFlags() ){
		return_badformat()
	}*/

	return (DWORD)ICERR_OK;
}

// return the intended compress format for the given input format
DWORD CodecInst::CompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
	if ( !lpbiOut)
	{
		return sizeof(BITMAPINFOHEADER)+sizeof(UINT32);	
	}

	// make sure the input is an acceptable format
	if ( CompressQuery(lpbiIn, NULL) == ICERR_BADFORMAT )
	{
		return_badformat()
	}

	LoadRegistrySettings(&_useNullFrames, &_useSnappy, &_dxtQuality);

	if (_codecType == HapAlpha && lpbiIn->biBitCount != 32)
	{
		return_badformat()
	}

	*lpbiOut = *lpbiIn;
	lpbiOut->biSize = sizeof(BITMAPINFOHEADER)+sizeof(UINT32);
	lpbiOut->biPlanes = 1;
	switch (_codecType)
	{
	case Hap:
		lpbiOut->biBitCount = 24;
		lpbiOut->biCompression = FOURCC_HAP;
		break;
	case HapAlpha:
		lpbiOut->biBitCount = 32;
		lpbiOut->biCompression = FOURCC_HAPA;
		break;
	case HapQ:
		lpbiOut->biBitCount = 24;
		lpbiOut->biCompression = FOURCC_HAPQ;
		break;
	}
	
	if ( lpbiOut->biBitCount != 24 )
	{
		lpbiOut->biSizeImage = lpbiIn->biWidth * abs(lpbiIn->biHeight) * lpbiIn->biBitCount/8;
	} 
	else 
	{
		lpbiOut->biSizeImage = align_round(lpbiIn->biWidth * lpbiIn->biBitCount/8,4)* abs(lpbiIn->biHeight);
	}
	
	return (DWORD)ICERR_OK;
}

// return information about the codec
DWORD CodecInst::GetInfo(ICINFO* icinfo, DWORD dwSize)
{
	if (icinfo == NULL)
		return sizeof(ICINFO);

	if (dwSize < sizeof(ICINFO))
		return 0;

	icinfo->dwSize          = sizeof(ICINFO);
	icinfo->fccType         = ICTYPE_VIDEO;
	icinfo->dwFlags			= VIDCF_FASTTEMPORALC | VIDCF_FASTTEMPORALD;
	icinfo->dwVersion		= 0x00010000;
	icinfo->dwVersionICM	= ICVERSION;

	if (_codecType == Hap)
	{
		icinfo->fccHandler		= FOURCC_HAP;
		memcpy(icinfo->szName,L"Hap",sizeof(L"Hap"));
		memcpy(icinfo->szDescription,L"Hap",sizeof(L"Hap"));
	}
	else if (_codecType == HapAlpha)
	{
		icinfo->fccHandler		= FOURCC_HAPA;
		memcpy(icinfo->szName,L"Hap Alpha",sizeof(L"Hap Alpha"));
		memcpy(icinfo->szDescription,L"Hap Alpha",sizeof(L"Hap Alpha"));
	}
	else if (_codecType == HapQ)
	{
		icinfo->fccHandler		= FOURCC_HAPQ;
		memcpy(icinfo->szName,L"Hap Q",sizeof(L"Hap Q"));
		memcpy(icinfo->szDescription,L"Hap Q",sizeof(L"Hap Q"));
	}

	return sizeof(ICINFO);
}

// check if the codec can decompress the given format to the desired format
DWORD CodecInst::DecompressQuery(const LPBITMAPINFOHEADER lpbiIn, const LPBITMAPINFOHEADER lpbiOut)
{
#if 1
	if (_codecType != Hap &&
		_codecType != HapAlpha &&
		_codecType != HapQ)
	{
		return_badformat();
	}

	if (_codecType == Hap)
	{
		if (lpbiIn->biCompression != FOURCC_HAP && lpbiIn->biCompression != FOURCC_hap)
		{
			return_badformat();
		}
		if (lpbiIn->biBitCount != 24)
		{
			return_badformat();
		}
	}

	if (_codecType == HapAlpha)
	{
		if (lpbiIn->biCompression != FOURCC_HAPA && lpbiIn->biCompression != FOURCC_hapa)
		{
			return_badformat();
		}
		if (lpbiIn->biBitCount != 32)
		{
			return_badformat();
		}
	}

	if (_codecType == HapQ)
	{
		if (lpbiIn->biCompression != FOURCC_HAPQ && lpbiIn->biCompression != FOURCC_hapq)
		{
			return_badformat();
		}
		if (lpbiIn->biBitCount != 24)
		{
			return_badformat();
		}
	}

	if (0 != (lpbiIn->biWidth%4) || 
		0 != (abs(lpbiIn->biHeight)%4))
	{
		return_badformat();
	}

	if (!lpbiOut)
	{
		return (DWORD)ICERR_OK;
	}

	if (!_supportUncompressedOutput)
	{
		if (lpbiOut->biCompression == BI_RGB)
		{
			return_badformat();
		}
	}

	if (lpbiOut->biCompression != FOURCC_DXT1 &&
		lpbiOut->biCompression != FOURCC_DXT5 &&
		lpbiOut->biCompression != FOURCC_DXTY &&
		lpbiOut->biCompression != BI_RGB &&
		lpbiOut->biCompression != FOURCC_dxt1 &&
		lpbiOut->biCompression != FOURCC_dxt5 &&
		lpbiOut->biCompression != FOURCC_dxty)
	{
		return_badformat();
	}

	// Don't support output to bottom-up
	if (lpbiOut->biCompression == BI_RGB && lpbiOut->biHeight < 0)
	{
		return_badformat();
	}

	if (lpbiOut->biCompression == BI_RGB && lpbiOut->biBitCount != 24 && lpbiOut->biBitCount != 32)
	{
		return_badformat();
	}

	if ((lpbiOut->biCompression == FOURCC_DXT1 || lpbiOut->biCompression == FOURCC_dxt1) && lpbiOut->biBitCount != 24)
	{
		return_badformat();
	}
	if ((lpbiOut->biCompression == FOURCC_DXT5 || lpbiOut->biCompression == FOURCC_dxt5) && lpbiOut->biBitCount != 32)
	{
		return_badformat();
	}
	if ((lpbiOut->biCompression == FOURCC_DXTY || lpbiOut->biCompression == FOURCC_dxty) && lpbiOut->biBitCount != 24)
	{
		return_badformat();
	}

	if ( abs(lpbiIn->biHeight) != abs(lpbiOut->biHeight) )
		return_badformat();

	if ( lpbiIn->biWidth != lpbiOut->biWidth )
		return_badformat();
#endif
	return (DWORD)ICERR_OK;
}

// return the default decompress format for the given input format 
DWORD CodecInst::DecompressGetFormat(const LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
#if 1
	//_supportUncompressedOutput = false;

	if (DecompressQuery(lpbiIn, NULL) != ICERR_OK)
	{
		return_badformat();
	}

	if ( !lpbiOut)
		return sizeof(BITMAPINFOHEADER);

	lpbiOut->biSize = sizeof(BITMAPINFOHEADER);
	lpbiOut->biWidth = lpbiIn->biWidth;
	lpbiOut->biHeight = lpbiIn->biHeight;
	lpbiOut->biPlanes = 1;
	lpbiOut->biCompression = BI_RGB;
	lpbiOut->biBitCount = 32;
	lpbiOut->biSizeImage = lpbiOut->biWidth * abs(lpbiOut->biHeight) * 4;
	lpbiOut->biXPelsPerMeter = 0;
	lpbiOut->biYPelsPerMeter = 0;
	lpbiOut->biClrUsed = 0;
	lpbiOut->biClrImportant = 0;

	//lpbiOut->biCompression = FOURCC_DXT1;
	//lpbiOut->biBitCount = 24;
	//lpbiOut->biSizeImage = lpbiIn->biWidth * abs(lpbiIn->biHeight);

	//unsigned int outputBufferTextureFormat = 0;
	/*if (0 != HapGetTextureFormat(lpbiIn->biSizeImage, &outputBufferTextureFormat))
	{
		return_badformat();
	}*/

	//int encodingFormat = *(UINT32*)(&lpbiIn[1]);
	//outputBufferTextureFormat = encodingFormat;

	
	switch (_codecType)
	{
	case Hap:
		lpbiOut->biCompression = FOURCC_DXT1;
		lpbiOut->biBitCount = 24;
		lpbiOut->biSizeImage = lpbiIn->biWidth * abs(lpbiIn->biHeight);
		break;
	case HapAlpha:
		lpbiOut->biCompression = FOURCC_DXT5;
		lpbiOut->biBitCount = 32;
		lpbiOut->biSizeImage = lpbiIn->biWidth * abs(lpbiIn->biHeight) * 2;
		break;
	case HapQ:
		lpbiOut->biCompression = FOURCC_DXTY;
		lpbiOut->biBitCount = 24;
		lpbiOut->biSizeImage = lpbiIn->biWidth * abs(lpbiIn->biHeight) * 2;
		break;
	default:
		return_badformat();
		break;
	}

#endif
	return (DWORD)ICERR_OK;
}

DWORD CodecInst::DecompressGetPalette(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
	return_badformat();
}