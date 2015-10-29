
#include <windows.h>
#include <DirectShow/BaseClasses/streams.h>
#include <initguid.h>
#if (1100 > _MSC_VER)
#include <olectlid.h>
#else
#include <olectl.h>
#endif
#include <assert.h>
#include "HapTransform.h"
//#include "hapcodec.h"
#include <squish/squish.h>
#include <hap/YCoCgDXT.h>
#include <hap/YCoCg.h>

static DWORD FOURCC_HAPH = MAKEFOURCC('H','A','P','H');
#define FOURCC_DXTY  (MAKEFOURCC('D','X','T','Y'))
#define FCC(ch4)   ((((DWORD)(ch4) & 0xFF) << 24) |     \
					(((DWORD)(ch4) & 0xFF00) << 8) |    \
					(((DWORD)(ch4) & 0xFF0000) >> 8) |  \
					(((DWORD)(ch4) & 0xFF000000) >> 24))

#define RGB24	24
#define RGB32	32

void
FlipVerticallyInPlace(unsigned char* buffer, int stride, int height)
{
	unsigned char* row = new unsigned char[stride];
	const unsigned char* inRow = buffer;
	unsigned char* outRow = buffer + (stride * (height - 1));
	for (int i = 0; i < height/2; i++)
	{
		memcpy((void*)row, (const void*)inRow, stride);
		memcpy((void*)inRow, (const void*)outRow, stride);
		memcpy((void*)outRow, (const void*)row, stride);
		inRow += stride;
		outRow -= stride;
	}
	delete [] row;
}

void
ConvertBGRAtoRGBA(int width, int height, const unsigned char* a, unsigned char* b)
{
	int numPixels = width * height;
	for (int i = 0; i < numPixels; i++)
	{
		b[0] = a[2];
		b[1] = a[1];
		b[2] = a[0];
		b[3] = a[3];

		a += 4;
		b += 4;
	}
}

void
Convert32bppTo24bpp(int width, int height, bool swapRedBlue, const unsigned char* a, unsigned char* b)
{
	int numPixels = width * height;
	for (int i = 0; i < numPixels; i++)
	{
		b[0] = a[0];
		b[1] = a[1];
		b[2] = a[2];

		if (swapRedBlue)
		{
			unsigned char temp = b[0];
			b[0] = b[2];
			b[2] = temp;
		}

		a += 4;
		b += 3;
	}
}

HapPixelFormatTransformFilter::HapPixelFormatTransformFilter(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr)
	: CTransformFilter(tszName, punk, CLSID_HapPixelFormatTransformFilter)
{ 
	_width = _height = 0;
	_format = 0;
	_bufferSize = 0;
	_outputSize = 0;
	_bufferSize = 0;
	_buffer = NULL;
	_buffer2 = NULL;
	_isOutputTopDown = true;
}

HapPixelFormatTransformFilter::~HapPixelFormatTransformFilter()
{
	FreeBuffers();
}

// Check whether an input format is supported
HRESULT 
HapPixelFormatTransformFilter::CheckInputType(const CMediaType *mtIn)
{
	if ((mtIn->majortype != MEDIATYPE_Video) ||
		//(mtIn->subtype != FOURCC_DXT1 && mtIn->subtype != FOURCC_DXT5 && mtIn->subtype != FOURCC_DXTY) ||
		(mtIn->formattype != FORMAT_VideoInfo) || 
		(mtIn->cbFormat < sizeof(VIDEOINFOHEADER)))
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	VIDEOINFOHEADER *pVih = reinterpret_cast<VIDEOINFOHEADER*>(mtIn->pbFormat);
	if ((pVih->bmiHeader.biBitCount != 24) &&
		(pVih->bmiHeader.biBitCount != 32))
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	if ((pVih->bmiHeader.biCompression != FOURCC_DXT1) &&
		(pVih->bmiHeader.biCompression != FOURCC_DXT5) &&
		(pVih->bmiHeader.biCompression != FOURCC_DXTY))
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	return S_OK;
}

// Specify supported output formats
HRESULT
HapPixelFormatTransformFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	ASSERT(m_pInput->IsConnected());
	if (iPosition < 0)
	{
		return E_INVALIDARG;
	}
	if (iPosition == 0)
	{
		HRESULT hr = m_pInput->ConnectionMediaType(pMediaType);
		if (FAILED(hr))
		{
			return hr;
		}
		pMediaType->subtype = MEDIASUBTYPE_RGB32;
		pMediaType->SetTemporalCompression(FALSE);

		ASSERT(pMediaType->formattype == FORMAT_VideoInfo);
		VIDEOINFOHEADER *pVih = reinterpret_cast<VIDEOINFOHEADER*>(pMediaType->pbFormat);
		pVih->bmiHeader.biBitCount = 32;
		pVih->bmiHeader.biCompression = BI_RGB;
		pVih->bmiHeader.biSizeImage = DIBSIZE(pVih->bmiHeader); 
		return S_OK;
	}
	else if (iPosition == 1)
	{
		HRESULT hr = m_pInput->ConnectionMediaType(pMediaType);
		if (FAILED(hr))
		{
			return hr;
		}
		pMediaType->subtype = MEDIASUBTYPE_RGB24;
		pMediaType->SetTemporalCompression(FALSE);

		ASSERT(pMediaType->formattype == FORMAT_VideoInfo);
		VIDEOINFOHEADER *pVih = reinterpret_cast<VIDEOINFOHEADER*>(pMediaType->pbFormat);
		pVih->bmiHeader.biBitCount = 24;
		pVih->bmiHeader.biCompression = BI_RGB;
		pVih->bmiHeader.biSizeImage = DIBSIZE(pVih->bmiHeader); 
		return S_OK;
	}
	// else
	return VFW_S_NO_MORE_ITEMS;
}

// Check whether an input format is compatible with an output format
HRESULT
HapPixelFormatTransformFilter::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	// Check the major type.
	if (mtOut->majortype != MEDIATYPE_Video)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	// Check the subtype and format type.
	if (mtOut->subtype != MEDIASUBTYPE_RGB24 && 
		mtOut->subtype != MEDIASUBTYPE_RGB32)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	if ((mtOut->formattype != FORMAT_VideoInfo) || 
		(mtOut->cbFormat < sizeof(VIDEOINFOHEADER)))
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}



	// Compare the bitmap information against the input type.
	ASSERT(mtIn->formattype == FORMAT_VideoInfo);
	BITMAPINFOHEADER *pBmiOut = HEADER(mtOut->pbFormat);
	BITMAPINFOHEADER *pBmiIn = HEADER(mtIn->pbFormat);

	/*
	if (pBmiIn->biCompression == FOURCC_DXT1)
	{
		if (pBmiOut->biBitCount != 24 ||
			pBmiOut->biCompression != BI_RGB ||
			mtOut->subtype != MEDIASUBTYPE_RGB24)
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}
	if (pBmiIn->biCompression == FOURCC_DXT5)
	{
		if (pBmiOut->biBitCount != 32 ||
			pBmiOut->biCompression != BI_RGB ||
			mtOut->subtype != MEDIASUBTYPE_RGB32)
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}
	if (pBmiIn->biCompression == FOURCC_DXTY)
	{
		if (pBmiOut->biBitCount != 24 ||
			pBmiOut->biCompression != BI_RGB ||
			mtOut->subtype != MEDIASUBTYPE_RGB24)
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}*/

	if ((pBmiOut->biPlanes != 1) ||
		(pBmiOut->biBitCount != 32 && pBmiOut->biBitCount != 24) ||
		(pBmiOut->biCompression != BI_RGB) ||
		(pBmiOut->biWidth != pBmiIn->biWidth) ||
		(pBmiOut->biHeight != pBmiIn->biHeight))
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	// Compare source and target rectangles.
	RECT rcImg;
	SetRect(&rcImg, 0, 0, pBmiIn->biWidth, pBmiIn->biHeight);
	RECT *prcSrc = &((VIDEOINFOHEADER*)(mtIn->pbFormat))->rcSource;
	RECT *prcTarget = &((VIDEOINFOHEADER*)(mtOut->pbFormat))->rcTarget;
	if (!IsRectEmpty(prcSrc) && !EqualRect(prcSrc, &rcImg))
	{
		return VFW_E_INVALIDMEDIATYPE;
	}
	if (!IsRectEmpty(prcTarget) && !EqualRect(prcTarget, &rcImg))
	{
		return VFW_E_INVALIDMEDIATYPE;
	}

	// Everything is good.
	return S_OK;
}

HRESULT
HapPixelFormatTransformFilter::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp)
{
	AM_MEDIA_TYPE mt;
	HRESULT hr = m_pOutput->ConnectionMediaType(&mt);
	if (FAILED(hr))
	{
		return hr;
	}

	AM_MEDIA_TYPE mtin;
	hr = m_pInput->ConnectionMediaType(&mtin);
	if (FAILED(hr))
	{
		return hr;
	}

	ASSERT(mt.formattype == FORMAT_VideoInfo);
	ASSERT(mtin.formattype == FORMAT_VideoInfo);
	BITMAPINFOHEADER *pbmi = HEADER(mt.pbFormat);
	BITMAPINFOHEADER *pbmin = HEADER(mtin.pbFormat);

	SetFormats(pbmin, pbmi);

	pProp->cbBuffer = DIBSIZE(*pbmi) * 2; 
	if (pProp->cbAlign == 0)
	{
		pProp->cbAlign = 1;
	}
	if (pProp->cBuffers == 0)
	{
		pProp->cBuffers = 1;
	}
	// Release the format block.
	FreeMediaType(mt);

	// Set allocator properties.
	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProp, &Actual);
	if (FAILED(hr)) 
	{
		return hr;
	}
	// Even when it succeeds, check the actual result.
	if (pProp->cbBuffer > Actual.cbBuffer) 
	{
		return E_FAIL;
	}
	return S_OK;
}

void
HapPixelFormatTransformFilter::SetFormats(BITMAPINFOHEADER* formatIn, BITMAPINFOHEADER* formatOut)
{
	_width = formatOut->biWidth;
	_height = abs(formatOut->biHeight);
	_format = formatIn->biCompression;
	_formatOut = formatOut->biBitCount;
	_isOutputTopDown = (formatOut->biHeight < 0);
	_bufferSize = _width * _height * 4;
	_outputSize = _width * _height * formatOut->biBitCount / 8;

	_dxtFlags = squish::kDxt5;
	if (_format == FOURCC_DXT1)
		_dxtFlags = squish::kDxt1;

	FreeBuffers();
	CreateBuffers();
}

void
HapPixelFormatTransformFilter::CreateBuffers()
{
	_buffer = new unsigned char[_bufferSize];
	_buffer2 = new unsigned char[_bufferSize];
}

void
HapPixelFormatTransformFilter::FreeBuffers()
{
	if (_buffer != NULL)
	{
		delete [] _buffer;
		_buffer = NULL;
	}
	if (_buffer2 != NULL)
	{
		delete [] _buffer2;
		_buffer2 = NULL;
	}
}

HRESULT
HapPixelFormatTransformFilter::Transform(IMediaSample *pSource, IMediaSample *pDest)
{
	HRESULT hr = S_FALSE;

	// Get pointers to the underlying buffers.
	BYTE *pBufferIn, *pBufferOut;
	hr = pSource->GetPointer(&pBufferIn);
	if (FAILED(hr))
	{
		return hr;
	}
	hr = pDest->GetPointer(&pBufferOut);
	if (FAILED(hr))
	{
		return hr;
	}

	if (_format == FOURCC_DXT1 || _format == FOURCC_DXT5)
	{
		// Convert DXT to RGBA
		squish::DecompressImage(_buffer, _width, _height, pBufferIn, _dxtFlags);

		if (_formatOut == RGB32)
		{
			// Swap red-blue channels
			ConvertBGRAtoRGBA(_width, _height, _buffer, pBufferOut);
		}
		else if (_formatOut == RGB24)
		{
			// Convert 32-bit to 24-bit
			Convert32bppTo24bpp(_width, _height, true, _buffer, pBufferOut);
		}
	}
	else if (_format == FOURCC_DXTY)
	{
		int outputSize = DeCompressYCoCgDXT5((const byte*)pBufferIn, _buffer, _width, _height, _width * 4);
		assert(outputSize == _width * _height * 4);

		if (_formatOut == RGB32)
		{
			ConvertCoCg_Y8888ToBGR_(_buffer, pBufferOut, _width, _height, _width * 4, _width * 4, 0);
		}
		else if (_formatOut == RGB24)
		{
			ConvertCoCg_Y8888ToBGR_(_buffer, _buffer2, _width, _height, _width * 4, _width * 4, 0);

			// Convert 32-bit to 24-bit and swap red-blue channels
			Convert32bppTo24bpp(_width, _height, true, _buffer2, pBufferOut);
		}
	}

	// if the output is bottom-up we need to flip our output
	if (!_isOutputTopDown)
	{
		if (_formatOut == RGB32)
		{
			FlipVerticallyInPlace(pBufferOut, _width * 4, _height);
		}
		else if (_formatOut == RGB24)
		{
			FlipVerticallyInPlace(pBufferOut, _width * 3, _height);
		}
	}

	pDest->SetActualDataLength(_outputSize);

	REFERENCE_TIME timeStart, timeEnd;
	hr = pSource->GetTime(&timeStart, &timeEnd);

	if (FAILED(hr))
	{
		hr = pDest->SetTime(0, 0);
		assert(SUCCEEDED(hr));
	}
	else if (hr == S_OK)
	{
		hr = pDest->SetTime(&timeStart, &timeEnd);
		assert(SUCCEEDED(hr));
	}
	else
	{
		hr = 	pDest->SetTime(&timeStart, 0);
		assert(SUCCEEDED(hr));
	}

	hr = pDest->SetMediaTime(0, 0);

	hr = pDest->SetSyncPoint(TRUE);
	assert(SUCCEEDED(hr));

	hr = pDest->SetPreroll(FALSE);
	assert(SUCCEEDED(hr));

	hr = pSource->IsDiscontinuity();
	hr = pDest->SetDiscontinuity(hr == S_OK);

	return S_OK;
}

// COM function for creating instant of this class
CUnknown * WINAPI
HapPixelFormatTransformFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr)
{
	HapPixelFormatTransformFilter *pFilter = new HapPixelFormatTransformFilter(g_wszName, pUnk, pHr);
	if (pFilter== NULL) 
	{
		*pHr = E_OUTOFMEMORY;
	}
	return pFilter;
}
