#ifndef _MAIN_HEADER
#define _MAIN_HEADER

#include <process.h>
#include <malloc.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vfw.h>
#include <malloc.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

//unsigned __int64 GetTime();


//#define X64_BUILD // this will be set automaticly if using the release 64 configuration

//#define LOSSLESS_CHECK			// enables decompressing encoded frames and comparing to original

#define LAGARITH_RELEASE		// if this is a version to release, disables all debugging info 

inline void * lag_aligned_malloc( void *ptr, int size, int align, char *str ) {
	if ( ptr ){
		try {
			#ifndef LAGARITH_RELEASE
			char msg[128];
			sprintf_s(msg,128,"Buffer '%s' is not null, attempting to free it...",str);
			MessageBox (HWND_DESKTOP, msg, "Error", MB_OK | MB_ICONEXCLAMATION);
			#endif
			_aligned_free(ptr);
		} catch ( ... ){
			#ifndef LAGARITH_RELEASE
			char msg[256];
			sprintf_s(msg,128,"An exception occurred when attempting to free non-null buffer '%s' in lag_aligned_malloc",str);
			MessageBox (HWND_DESKTOP, msg, "Error", MB_OK | MB_ICONEXCLAMATION);
			#endif
		}
	}
	return _aligned_malloc(size,align);
}

#ifdef LAGARITH_RELEASE
#define lag_aligned_free(ptr, str) { \
	if ( ptr ){ \
		try {\
			_aligned_free((void*)ptr);\
		} catch ( ... ){ } \
	} \
	ptr=NULL;\
}
#else 
#define lag_aligned_free(ptr, str) { \
	if ( ptr ){ \
		try { _aligned_free(ptr); } catch ( ... ){\
			char err_msg[256];\
			sprintf_s(err_msg,256,"Error when attempting to free pointer %s, value = 0x%X - file %s line %d",str,ptr,__FILE__, __LINE__);\
			MessageBox (HWND_DESKTOP, err_msg, "Error", MB_OK | MB_ICONEXCLAMATION);\
		} \
	} \
	ptr=NULL;\
}
#endif

// y must be 2^n
#define align_round(x,y) ((((unsigned int)(x))+(y-1))&(~(y-1)))

#include "resource.h"
//#include "compact.h"
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
	((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
	((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))


#define FOURCC_DXT1  (MAKEFOURCC('D','X','T','1'))
#define FOURCC_dxt1  (MAKEFOURCC('d','x','t','1'))
#define FOURCC_DXT5  (MAKEFOURCC('D','X','T','5'))
#define FOURCC_dxt5  (MAKEFOURCC('d','x','t','5'))
#define FOURCC_DXTY  (MAKEFOURCC('D','X','T','Y'))
#define FOURCC_dxty  (MAKEFOURCC('d','x','t','y'))

static DWORD FOURCC_HAP = MAKEFOURCC('H','a','p','1');
static DWORD FOURCC_HAPA = MAKEFOURCC('H','a','p','5');
static DWORD FOURCC_HAPQ = MAKEFOURCC('H','a','p','Y');

static DWORD FOURCC_hap = MAKEFOURCC('h','a','p','1');
static DWORD FOURCC_hapa = MAKEFOURCC('h','a','p','5');
static DWORD FOURCC_hapq = MAKEFOURCC('h','a','p','y');





// possible colorspaces
#define RGB24	24
#define RGB32	32

class CodecInst 
{
public:

	enum CodecType
	{
		Hap,
		HapAlpha,
		HapQ,
	};

	int						_isStarted;			//if the codec has been properly initalized yet
	unsigned char*			_buffer;
	unsigned char*			_buffer2;
	unsigned char*			_prevBuffer;
	const unsigned char*	_in;
	unsigned char*			_out;

	unsigned int			_dxtBufferSize;
	void*					_dxtBuffer;
	int						_dxtFlags;

	unsigned int			_length;
	unsigned int			_width;
	unsigned int			_height;
	unsigned int			_format;	//input format for compressing, output format for decompression. Also the bitdepth.
	
	CodecType				_codecType;
	int						_dxtQuality;
	bool					_useAlpha;
	bool					_useNullFrames;
	bool					_useSnappy;
	unsigned int compressed_size;

	CodecInst(CodecType codecType);
	~CodecInst();

	DWORD GetState(LPVOID pv, DWORD dwSize);
	DWORD SetState(LPVOID pv, DWORD dwSize);
	DWORD About(HWND hwnd);
	DWORD Configure(HWND hwnd);
	DWORD GetInfo(ICINFO* icinfo, DWORD dwSize);

	DWORD CompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD CompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD CompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD CompressGetSize(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD Compress(ICCOMPRESS* icinfo, DWORD dwSize);
	DWORD CompressEnd();

	DWORD DecompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD DecompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD DecompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD Decompress(ICDECOMPRESS* icinfo, DWORD dwSize);
	DWORD DecompressGetPalette(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
	DWORD DecompressEnd();

	BOOL QueryConfigure();


private:
	int				GetDxtFlags() const;
	unsigned long	CompressHap(const unsigned char* inputBuffer, void* outputBuffer, unsigned long outputBufferSize, unsigned int compressorOptions);
	unsigned long	CompressHapAlpha(const unsigned char* inputBuffer, void* outputBuffer, unsigned long outputBufferSize, unsigned int compressorOptions);
	unsigned long	CompressHapQ(const unsigned char* inputBuffer, void* outputBuffer, unsigned long outputBufferSize, unsigned int compressorOptions);

	bool _supportUncompressedOutput;
};

CodecInst* Open(ICOPEN* icinfom, CodecInst::CodecType codecType);
DWORD Close(CodecInst* pinst);

#endif