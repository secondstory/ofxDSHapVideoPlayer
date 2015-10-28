
#pragma once

#include "DSShared.h"
#include <assert.h>
#include <streams.h>

DEFINE_GUID(CLSID_SampleGrabber, 0xb62f694e, 0x593, 0x4e60, 0xaa, 0x1c, 0x16, 0xaf, 0x64, 0x96, 0xac, 0x39);
DEFINE_GUID(IID_ISampleGrabber, 0xbe5b5e, 0xcca0, 0x4a9f, 0xb1, 0x98, 0xdf, 0x2f, 0xac, 0xe8, 0x2d, 0x57);

#define FILTERNAME L"SampleGrabberFilter"

typedef HRESULT(CALLBACK *MANAGEDCALLBACKPROC)(double Time, IMediaSample *pSample);
//typedef void (CALLBACK *MANAGEDCALLBACKPROC)(BYTE* pdata, long len);

// ISampleGrabber interface definition
#ifdef __cplusplus
extern "C" {
#endif 
	// {04951BFF-696A-4ade-828D-42A5F1EDB631}
	//DEFINE_GUID(IID_ISampleGrabber,
		//0x4951bff, 0x696a, 0x4ade, 0x82, 0x8d, 0x42, 0xa5, 0xf1, 0xed, 0xb6, 0x31);

	
	DECLARE_INTERFACE_(ISampleGrabber, IUnknown) {
		STDMETHOD(RegisterCallback)(MANAGEDCALLBACKPROC callback) PURE;
	};
	

#ifdef __cplusplus
}
#endif 


class DSUncompressedSampleGrabber : public CTransInPlaceFilter, public ISampleGrabber {

private:

	MANAGEDCALLBACKPROC callback;
	ISampleGrabberCB * pCallback;
	CMediaType * m_t;
	long m_Width;
	long m_Height;
	long m_SampleSize;
	long m_Stride;

public:

	// instantiation
	DSUncompressedSampleGrabber(IUnknown * pOuter, HRESULT * phr, BOOL ModifiesData);
	~DSUncompressedSampleGrabber();
	static CUnknown *WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	// IUnknown
	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	HRESULT CheckInputType(const CMediaType * pmt);
	HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType * pmt);
	HRESULT Transform(IMediaSample * pMediaSample);
	HRESULT DecideBufferSize(IMemAllocator * pAlloc, ALLOCATOR_PROPERTIES * pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType * pMediaType);
	HRESULT CheckTransform(const CMediaType * mtIn, const CMediaType * mtOut){
		return NOERROR;
	}

	HRESULT STDMETHODCALLTYPE SetOneShot(BOOL OneShot);
	//HRESULT STDMETHODCALLTYPE SetMediaType(const AM_MEDIA_TYPE *pType);
	HRESULT STDMETHODCALLTYPE GetConnectedMediaType(AM_MEDIA_TYPE *pType);
	HRESULT STDMETHODCALLTYPE SetBufferSamples(BOOL BufferThem);
	HRESULT STDMETHODCALLTYPE GetCurrentBuffer(long *pBufferSize, long *pBuffer);
	HRESULT STDMETHODCALLTYPE GetCurrentSample(IMediaSample **ppSample);
	HRESULT STDMETHODCALLTYPE SetCallback(ISampleGrabberCB *pCallback, long WhichMethodToCallback);

	STDMETHODIMP RegisterCallback(MANAGEDCALLBACKPROC mdelegate);
};