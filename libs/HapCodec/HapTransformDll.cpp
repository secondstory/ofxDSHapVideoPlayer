
#include <windows.h>
#include <DirectShow/BaseClasses/streams.h>
#include <initguid.h>
#if (1100 > _MSC_VER)
#include <olectlid.h>
#else
#include <olectl.h>
#endif

#include "hapcodec.h"
#include "HapTransform.h"

DEFINE_GUID(MEDIASUBTYPE_DXT1, MAKEFOURCC('D','X','T','1'), 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(MEDIASUBTYPE_DXT5, MAKEFOURCC('D','X','T','5'), 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(MEDIASUBTYPE_DXTY, MAKEFOURCC('D','X','T','Y'), 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

// Setup information

const AMOVIESETUP_MEDIATYPE sudPinInputTypes[] =
{
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DXT1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DXT5 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DXTY },
};

 const AMOVIESETUP_MEDIATYPE sudPinOutputTypes[] =
{
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RGB32 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RGB24 },
};

const AMOVIESETUP_PIN sudpPins[] =
{
	{ 
		L"Input",             // Pins string name
		FALSE,                // Is it rendered
		FALSE,                // Is it an output
		FALSE,                // Are we allowed none
		FALSE,                // And allowed many
		&CLSID_NULL,          // Connects to filter
		NULL,                 // Connects to pin
		3,                    // Number of types
		sudPinInputTypes          // Pin information
	},
	{
		L"Output",            // Pins string name
		FALSE,                // Is it rendered
		TRUE,                 // Is it an output
		FALSE,                // Are we allowed none
		FALSE,                // And allowed many
		&CLSID_NULL,          // Connects to filter
		NULL,                 // Connects to pin
		2,                    // Number of types
		sudPinOutputTypes          // Pin information
	}
};

const AMOVIESETUP_FILTER sudFilter =
{
	&CLSID_HapPixelFormatTransformFilter,         // Filter CLSID
	g_wszName,			// String name
	MERIT_NORMAL,       // Filter merit
	2,                      // Number of pins
	sudpPins                // Pin information
};


CFactoryTemplate g_Templates[] = 
{
	{ 
		g_wszName,
			&CLSID_HapPixelFormatTransformFilter,
			HapPixelFormatTransformFilter::CreateInstance,
			NULL,
			&sudFilter
	}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);

}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);
BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}