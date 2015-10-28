
#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// DirectShow includes and helper methods 
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <DShow.h>
#pragma include_alias( "dxtrans.h", "qedit.h" )
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__
#include <uuids.h>
#include <Aviriff.h>
#include <Windows.h>

//for threading
#include <process.h>

// Due to a missing qedit.h in recent Platform SDKs, we've replicated the relevant contents here
// #include <qedit.h>
MIDL_INTERFACE("0579154A-2B53-4994-B0D0-E773148EFF85")
ISampleGrabberCB : public IUnknown
{
public:
	/*
	virtual HRESULT STDMETHODCALLTYPE SampleCB(
		double SampleTime,
		IMediaSample *pSample) = 0;*/

	virtual HRESULT STDMETHODCALLTYPE SampleCB(
		long SampleTime,
		IMediaSample *pSample) = 0;

	virtual HRESULT STDMETHODCALLTYPE BufferCB(
		double SampleTime,
		BYTE *pBuffer,
		long BufferLen) = 0;

};


MIDL_INTERFACE("6B652FFF-11FE-4fce-92AD-0266B5D7C78F")
ISampleGrabber : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE SetOneShot(
		BOOL OneShot) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetMediaType(
		const AM_MEDIA_TYPE *pType) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(
		AM_MEDIA_TYPE *pType) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(
		BOOL BufferThem) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(
		long *pBufferSize,
		long *pBuffer) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(
		IMediaSample **ppSample) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetCallback(
		ISampleGrabberCB *pCallback,
		long WhichMethodToCallback) = 0;

};


//EXTERN_C const CLSID CLSID_SampleGrabber;
//EXTERN_C const IID IID_ISampleGrabber;
EXTERN_C const CLSID CLSID_NullRenderer;

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
	((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
	((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))


#define FOURCC_DXT1  (MAKEFOURCC('D','X','T','1'))
#define FOURCC_dxt1  (MAKEFOURCC('d','x','t','1'))
#define FOURCC_DXT5  (MAKEFOURCC('D','X','T','5'))
#define FOURCC_dxt5  (MAKEFOURCC('d','x','t','5'))
#define FOURCC_DXTY  (MAKEFOURCC('D','X','T','Y'))
#define FOURCC_dxty  (MAKEFOURCC('d','x','t','y'))

static DWORD FOURCC_HAP = MAKEFOURCC('H', 'a', 'p', '1');
static DWORD FOURCC_HAPA = MAKEFOURCC('H', 'a', 'p', '5');
static DWORD FOURCC_HAPQ = MAKEFOURCC('H', 'a', 'p', 'Y');

static DWORD FOURCC_hap = MAKEFOURCC('h', 'a', 'p', '1');
static DWORD FOURCC_hapa = MAKEFOURCC('h', 'a', 'p', '5');
static DWORD FOURCC_hapq = MAKEFOURCC('h', 'a', 'p', 'y');




/*
// GetUnconnectedPin   
//    Finds an unconnected pin on a filter in the desired direction   
HRESULT GetUnconnectedPin(
	IBaseFilter *pFilter,   // Pointer to the filter.   
	PIN_DIRECTION PinDir,   // Direction of the pin to find.   
	IPin **ppPin)           // Receives a pointer to the pin.   
{
	*ppPin = 0;
	IEnumPins *pEnum = 0;
	IPin *pPin = 0;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		return hr;
	}
	while (pEnum->Next(1, &pPin, NULL) == S_OK)
	{
		PIN_DIRECTION ThisPinDir;
		pPin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PinDir)
		{
			IPin *pTmp = 0;
			hr = pPin->ConnectedTo(&pTmp);
			if (SUCCEEDED(hr))  // Already connected, not the pin we want.   
			{
				pTmp->Release();
			}
			else  // Unconnected, this is the pin we want.   
			{
				pEnum->Release();
				*ppPin = pPin;
				return S_OK;
			}
		}
		pPin->Release();
	}
	pEnum->Release();
	// Did not find a matching pin.   
	return E_FAIL;
}



// Disconnect any connections to the filter.   
HRESULT DisconnectPins(IBaseFilter *pFilter)
{
	IEnumPins *pEnum = 0;
	IPin *pPin = 0;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		return hr;
	}

	while (pEnum->Next(1, &pPin, NULL) == S_OK)
	{
		pPin->Disconnect();
		pPin->Release();
	}
	pEnum->Release();

	// Did not find a matching pin.   
	return S_OK;
}

// ConnectFilters   
//    Connects a pin of an upstream filter to the pDest downstream filter   
HRESULT ConnectFilters(
	IGraphBuilder *pGraph, // Filter Graph Manager.   
	IPin *pOut,            // Output pin on the upstream filter.   
	IBaseFilter *pDest)    // Downstream filter.   
{
	if ((pGraph == NULL) || (pOut == NULL) || (pDest == NULL))
	{
		return E_POINTER;
	}
#ifdef debug   
	PIN_DIRECTION PinDir;
	pOut->QueryDirection(&PinDir);
	_ASSERTE(PinDir == PINDIR_OUTPUT);
#endif   

	// Find an input pin on the downstream filter.   
	IPin *pIn = 0;
	HRESULT hr = GetUnconnectedPin(pDest, PINDIR_INPUT, &pIn);
	if (FAILED(hr))
	{
		return hr;
	}
	// Try to connect them.   
	hr = pGraph->Connect(pOut, pIn);
	pIn->Release();
	return hr;
}



// ConnectFilters   
//    Connects two filters   
HRESULT ConnectFilters(
	IGraphBuilder *pGraph,
	IBaseFilter *pSrc,
	IBaseFilter *pDest)
{
	if ((pGraph == NULL) || (pSrc == NULL) || (pDest == NULL))
	{
		return E_POINTER;
	}

	// Find an output pin on the first filter.   
	IPin *pOut = 0;
	HRESULT hr = GetUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
	if (FAILED(hr))
	{
		return hr;
	}
	hr = ConnectFilters(pGraph, pOut, pDest);
	pOut->Release();
	return hr;
}

// LocalFreeMediaType   
//    Free the format buffer in the media type   
void LocalFreeMediaType(AM_MEDIA_TYPE& mt)
{
	if (mt.cbFormat != 0)
	{
		CoTaskMemFree((PVOID)mt.pbFormat);
		mt.cbFormat = 0;
		mt.pbFormat = NULL;
	}
	if (mt.pUnk != NULL)
	{
		// Unecessary because pUnk should not be used, but safest.   
		mt.pUnk->Release();
		mt.pUnk = NULL;
	}
}

// LocalDeleteMediaType   
//    Free the format buffer in the media type,    
//    then delete the MediaType ptr itself   
void LocalDeleteMediaType(AM_MEDIA_TYPE *pmt)
{
	if (pmt != NULL)
	{
		LocalFreeMediaType(*pmt); // See FreeMediaType for the implementation.   
		CoTaskMemFree(pmt);
	}
}


HRESULT SaveGraphFile(IGraphBuilder *pGraph, WCHAR *wszPath)
{
	const WCHAR wszStreamName[] = L"ActiveMovieGraph";
	HRESULT hr;

	IStorage *pStorage = NULL;
	hr = StgCreateDocfile(
		wszPath,
		STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
		0, &pStorage);
	if (FAILED(hr))
	{
		return hr;
	}

	IStream *pStream;
	hr = pStorage->CreateStream(
		wszStreamName,
		STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
		0, 0, &pStream);
	if (FAILED(hr))
	{
		pStorage->Release();
		return hr;
	}

	IPersistStream *pPersist = NULL;
	pGraph->QueryInterface(IID_IPersistStream, (void**)&pPersist);
	hr = pPersist->Save(pStream, TRUE);
	pStream->Release();
	pPersist->Release();
	if (SUCCEEDED(hr))
	{
		hr = pStorage->Commit(STGC_DEFAULT);
	}
	pStorage->Release();
	return hr;
}

*/