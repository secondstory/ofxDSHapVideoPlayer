#ifndef HAPTRANSFORM_H
#define HAPTRANSFORM_H

// {110A6A7E-1318-439a-94F4-C332207B4917}
DEFINE_GUID(CLSID_HapPixelFormatTransformFilter, 
			0x110a6a7e, 0x1318, 0x439a, 0x94, 0xf4, 0xc3, 0x32, 0x20, 0x7b, 0x49, 0x17);

static WCHAR g_wszName[] = L"Hap Pixel Format Transform Filter";

class HapPixelFormatTransformFilter : public CTransformFilter
{
public:

	// COM
	DECLARE_IUNKNOWN;
	static CUnknown* WINAPI		CreateInstance(IUnknown *pUnk, HRESULT *pHr);

	 // Overridden from CTransformFilter base class
	HRESULT			CheckInputType(const CMediaType *mtIn);
	HRESULT			GetMediaType(int iPosition,CMediaType * pMediaType);
	HRESULT			CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT			Transform(IMediaSample *pIn, IMediaSample *pOut);
	HRESULT			DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop);

private:
	HapPixelFormatTransformFilter(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	virtual ~HapPixelFormatTransformFilter();

	void			SetFormats(BITMAPINFOHEADER* formatIn, BITMAPINFOHEADER* formatOut);
	void			CreateBuffers();
	void			FreeBuffers();

	unsigned char*			_buffer;
	unsigned char*			_buffer2;
	unsigned int			_bufferSize;
	bool					_isOutputTopDown;
	unsigned int			_outputSize;
	int						_dxtFlags;
	unsigned int			_width;
	unsigned int			_height;
	unsigned int			_format;
	unsigned int			_formatOut;
};

#endif