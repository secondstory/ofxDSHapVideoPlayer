
#include "DSUncompressedSampleGrabber.h"
//#include <stdafx.h>
//#pragma comment(lib,"Strmiids.lib")
#include <ddraw.h>
#include <objbase.h>
#include <commctrl.h>
#include <initguid.h>

/*
ULONG CBaseFilter::NonDelegatingRelease(void)
{
	return 1;
}

*/
/////////////////////// instantiation //////////////////////////

DSUncompressedSampleGrabber::DSUncompressedSampleGrabber(IUnknown * pOuter, HRESULT * phr, BOOL ModifiesData)
	: CVideoTransformFilter(FILTERNAME, (IUnknown*)pOuter, CLSID_SampleGrabber) {
	callback = NULL;
	printf("DSUncompressedSampleGrabber()\n");
}

DSUncompressedSampleGrabber::~DSUncompressedSampleGrabber() {
	callback = NULL;
}

CUnknown *WINAPI DSUncompressedSampleGrabber::CreateInstance(LPUNKNOWN punk, HRESULT *phr) {
	printf("CreateInstance()\n");
	HRESULT hr;
	if (!phr)
		phr = &hr;
	DSUncompressedSampleGrabber *pNewObject = new DSUncompressedSampleGrabber(punk, phr, FALSE);
	if (pNewObject == NULL)
		*phr = E_OUTOFMEMORY;
	return pNewObject;
}

/////////////////////// IUnknown //////////////////////////
HRESULT DSUncompressedSampleGrabber::NonDelegatingQueryInterface(const IID &riid, void **ppv) {
	if (riid == IID_ISampleGrabber) {
		return GetInterface(static_cast<ISampleGrabber*>(this), ppv);
	}
	else
		return CVideoTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}

/////////////////////// CTransInPlaceFilter //////////////////////////
HRESULT DSUncompressedSampleGrabber::CheckInputType(const CMediaType *pmt)
{

	return S_OK;

	if (pmt->majortype == MEDIATYPE_Video &&
		(pmt->subtype == MEDIASUBTYPE_RGB24) &&
		pmt->formattype == FORMAT_VideoInfo)
		return S_OK;
	else
		return E_FAIL;
}

HRESULT STDMETHODCALLTYPE DSUncompressedSampleGrabber::SetMediaType(const AM_MEDIA_TYPE *pType){ 

	printf("SetMediaTypeXX()\n");

	return S_OK;
}

HRESULT DSUncompressedSampleGrabber::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{

	printf("SetMediaType()\n");
	printf("major type %i, subtype %i\n", pmt->majortype, pmt->subtype);
	printf("matching major type %i, matching subtype %i\n", (pmt->majortype == MEDIATYPE_Video), pmt->subtype == MEDIASUBTYPE_Avi);
	printf("formattype %i\n", pmt->formattype == FORMAT_VideoInfo);
	printf("cbformat %i, null %i\n", pmt->cbFormat >= sizeof(VIDEOINFOHEADER), pmt->pbFormat != NULL);

	HRESULT hr = S_OK;

	return hr;

	VIDEOINFOHEADER* vih = (VIDEOINFOHEADER *)pmt->pbFormat;

	if (pmt->majortype == MEDIATYPE_Video &&
		(pmt->subtype == MEDIASUBTYPE_Avi) &&
		pmt->formattype == FORMAT_VideoInfo &&
		pmt->cbFormat >= sizeof(VIDEOINFOHEADER) &&
		pmt->pbFormat != NULL)
	{

		// calculate the stride for RGB formats
		DWORD dwStride = (vih->bmiHeader.biWidth * (vih->bmiHeader.biBitCount / 8) + 3) & ~3;

		// set video parameters
		m_Width = vih->bmiHeader.biWidth;
		m_Height = vih->bmiHeader.biHeight;
		m_SampleSize = pmt->lSampleSize;
		m_Stride = (long)dwStride;
	}
	else
	{
		printf("failed to set media type\n");
		hr = E_FAIL;
	}

	return hr;
}

HRESULT DSUncompressedSampleGrabber::Transform(IMediaSample * pIn, IMediaSample *pOut){

	//return S_OK;

	//printf("DSUncompressedSampleGrabber::Transform %p\n", this);

	long latestBufferLength = pIn->GetSize();
	//printf("Buffer length: %i samples\n", (int)latestBufferLength);

	BYTE * ptrBuffer = NULL;
	pIn->GetPointer(&ptrBuffer); 
	//printf("Buffer pointer: %p\n", ptrBuffer);

	REFERENCE_TIME sampleTime;
	REFERENCE_TIME totalDuration;
	HRESULT hr = pIn->GetTime(&sampleTime, &totalDuration);

	
	if (hr == S_OK){

		//printf("Buffer time: %i\n", (int)sampleTime/1000);
		//printf("File duration: %i\n", (int)totalDuration / 1000);
	}
	
	/*
	AM_MEDIA_TYPE * info;
	hr = pIn->GetMediaType(&info);

	if ( hr != S_OK ){

		printf("not okay!!\n");
	}
	else {

		if (info->formattype == FORMAT_VideoInfo){

			printf("videoinfo!!\n");
		}
	}
	*/

	// invoke managed delegate
	if (pCallback != NULL){

		// send the input buffer thru 
		pCallback->SampleCB(long(sampleTime), pIn);
	}

	return S_OK;
}

HRESULT DSUncompressedSampleGrabber::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut){

	printf("DSUncompressedSampleGrabber::CheckTransform\n");

	if (mtIn->formattype == FORMAT_VideoInfo){

		printf("FORMAT_VideoInfo\n");

		//VIDEOINFOHEADER2 * vih2 = (VIDEOINFOHEADER*)mtIn->Format();

		BITMAPINFOHEADER * pbih = &((VIDEOINFOHEADER*)mtIn->Format())->bmiHeader;

		printf("Dim %i %i\n", (int)pbih->biWidth, (int)pbih->biHeight);


	}

	return S_OK;
}

HRESULT DSUncompressedSampleGrabber::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
	if (m_pInput->IsConnected() == FALSE) {
		return E_UNEXPECTED;
	}

	ASSERT(pAlloc);
	ASSERT(pProperties);
	HRESULT hr = NOERROR;

	printf("sample size %i\n", m_pInput->CurrentMediaType().GetSampleSize());

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 1; // 1920 * 1080 * 4;
	//pProperties->cbBuffer = m_pInput->CurrentMediaType().GetSampleSize();
	ASSERT(pProperties->cbBuffer);

	// Ask the allocator to reserve us some sample memory, NOTE the function
	// can succeed (that is return NOERROR) but still not have allocated the
	// memory that we requested, so we must check we got whatever we wanted
	ALLOCATOR_PROPERTIES Actual;

	hr = pAlloc->SetProperties(pProperties, &Actual);
	if (FAILED(hr)) {
		printf("Failed to allocate memory\n");
		return hr;
	}

	ASSERT(Actual.cBuffers >= 1);

	if (pProperties->cBuffers > Actual.cBuffers ||
		pProperties->cbBuffer > Actual.cbBuffer) {
		return E_FAIL;
	}
	return NOERROR;
}

HRESULT DSUncompressedSampleGrabber::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	// Is the input pin connected
	if (m_pInput->IsConnected() == FALSE) {
		printf("warning: pin not connected\n");
		return E_UNEXPECTED;
	}

	// This should never happen
	if (iPosition < 0) {
		return E_INVALIDARG;
	}

	// Do we have more items to offer
	if (iPosition > 0) {
		return VFW_S_NO_MORE_ITEMS;
	}

	*pMediaType = m_pInput->CurrentMediaType();
	return NOERROR;
}

/////////////////////// ISampleGrabber //////////////////////////

STDMETHODIMP DSUncompressedSampleGrabber::RegisterCallback(MANAGEDCALLBACKPROC mdelegate)
{
	// Set pointer to managed delegate
	callback = mdelegate;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE DSUncompressedSampleGrabber::SetOneShot(BOOL OneShot)
{
	printf("SetOneShot()\n");
	return S_OK;
}

/*
HRESULT STDMETHODCALLTYPE DSUncompressedSampleGrabber::SetMediaType(const AM_MEDIA_TYPE *pType)
{
	printf("SetMediaType()\n");


	return S_OK;
}
*/

HRESULT STDMETHODCALLTYPE DSUncompressedSampleGrabber::GetConnectedMediaType(AM_MEDIA_TYPE *pType)
{
	printf("GetConnectedMediaType()\n");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE DSUncompressedSampleGrabber::SetBufferSamples(BOOL BufferThem)
{
	printf("SetBufferSamples()\n");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE DSUncompressedSampleGrabber::GetCurrentBuffer(long *pBufferSize, long *pBuffer)
{
	printf("GetCurrentBuffer()\n");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE DSUncompressedSampleGrabber::GetCurrentSample(IMediaSample **ppSample)
{
	printf("GetCurrentSample()\n");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE DSUncompressedSampleGrabber::SetCallback(ISampleGrabberCB *pCallback, long WhichMethodToCallback)
{
	printf("SetCallback()\n");
	this->pCallback = pCallback;

	return S_OK;
}