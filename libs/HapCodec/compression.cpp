
#include "interface.h"
#include "hapcodec.h"
#include <float.h>
#include <omp.h>
#include <squish/squish.h>
#include <hap/hap.h>
#include <hap/YCoCgDXT.h>
#include <hap/YCoCg.h>

//#define USE_OPENMP_DXT
#define NUM_THREADS 1

// initalize the codec for compression
DWORD 
CodecInst::CompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
	if (_isStarted  == 0x1337)
	{
		CompressEnd();
	}
	_isStarted = 0;
	omp_set_num_threads(NUM_THREADS);

	if ( int error = CompressQuery(lpbiIn, lpbiOut) != ICERR_OK )
	{
		return error;
	}

	// Collect properties

	_width = lpbiIn->biWidth;
	_height = abs(lpbiIn->biHeight);
	_format = lpbiIn->biBitCount;
	_length = _width * _height * _format / 8;

	// Allocate temporary buffers
	_dxtFlags = GetDxtFlags();
	_dxtBufferSize = squish::GetStorageRequirements(_width, _height, _dxtFlags);
	_dxtBuffer = (void*)lag_aligned_malloc(_dxtBuffer, _dxtBufferSize, 16, "dxtBuffer");

	unsigned int bufferSize = align_round(_width, 16) * _height * 4 + 4096;
	
	_buffer = (unsigned char *)lag_aligned_malloc(_buffer, bufferSize, 16, "buffer");
	_buffer2 = (unsigned char *)lag_aligned_malloc(_buffer2, bufferSize, 16, "buffer2");
	_prevBuffer = (unsigned char *)lag_aligned_malloc(_prevBuffer, bufferSize, 16, "prevBuffer");

	if (!_buffer || !_buffer2 || !_prevBuffer || !_dxtBuffer)
	{
		lag_aligned_free(_dxtBuffer, "dxtBuffer");
		lag_aligned_free(_buffer, "buffer");
		lag_aligned_free(_buffer2, "buffer2");
		lag_aligned_free(_prevBuffer, "prevBuffer");
		return ICERR_MEMORY;
	}

	_isStarted = 0x1337;

	return ICERR_OK;
}

// get the maximum size a compressed frame will take;
// 105% of image size + 1KB should be plenty even for random static

DWORD
CodecInst::CompressGetSize(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
	return (DWORD)( align_round(lpbiIn->biWidth,16)*abs(lpbiIn->biHeight)*lpbiIn->biBitCount/8*1.05 + 1024);
}

// release resources when compression is done

DWORD
CodecInst::CompressEnd()
{
	if (_isStarted  == 0x1337)
	{
		lag_aligned_free(_dxtBuffer, "dxtBuffer");
		lag_aligned_free(_buffer, "buffer");
		lag_aligned_free(_buffer2, "buffer2");
		lag_aligned_free(_prevBuffer, "prevBuffer");
	}
	_isStarted = 0;
	return ICERR_OK;
}

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
#pragma omp parallel
	{
		int id = omp_get_thread_num();
		const unsigned char* aa = a + (numPixels * 4 / NUM_THREADS) * id;
		unsigned char* bb = b + (numPixels * 4 / NUM_THREADS) * id;
	#pragma omp for
		for (int i = 0; i < numPixels; i++)
		{
			bb[0] = aa[2];
			bb[1] = aa[1];
			bb[2] = aa[0];
			bb[3] = aa[3];

			aa += 4;
			bb += 4;
		}
	}
}

void
Convert24bppTo32bpp(int width, int height, bool swapRedBlue, const unsigned char* a, unsigned char* b)
{
	int numPixels = width * height;
#pragma omp parallel
	{
		int id = omp_get_thread_num();
		const unsigned char* aa = a + (numPixels * 3 / NUM_THREADS) * id;
		unsigned char* bb = b + (numPixels * 4 / NUM_THREADS) * id;
#pragma omp for
		for (int i = 0; i < numPixels; i++)
		{
			bb[0] = aa[0];
			bb[1] = aa[1];
			bb[2] = aa[2];
			bb[3] = 255;

			if (swapRedBlue)
			{
				unsigned char temp = bb[0];
				bb[0] = bb[2];
				bb[2] = temp;
			}

			aa += 3;
			bb += 4;
		}
	}
}

int
CodecInst::GetDxtFlags() const
{
	int dxtFlags = 0;
	switch (_dxtQuality)
	{
	case 0:
		dxtFlags |= squish::kColourIterativeClusterFit;
		break;
	default:
	case 1:
		dxtFlags |= squish::kColourClusterFit;
		break;
	case 2:
		dxtFlags |= squish::kColourRangeFit;
		break;
	}

	switch (_codecType)
	{
	case Hap:
		dxtFlags |= squish::kDxt1;
		break;
	case HapAlpha:
	case HapQ:
		dxtFlags |= squish::kDxt5;
		break;
	}

	return dxtFlags;
}

struct DDS_PIXELFORMAT {
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwFourCC;
	DWORD dwRGBBitCount;
	DWORD dwRBitMask;
	DWORD dwGBitMask;
	DWORD dwBBitMask;
	DWORD dwABitMask;
};

typedef struct {
	DWORD           dwSize;
	DWORD           dwFlags;
	DWORD           dwHeight;
	DWORD           dwWidth;
	DWORD           dwPitchOrLinearSize;
	DWORD           dwDepth;
	DWORD           dwMipMapCount;
	DWORD           dwReserved1[11];
	DDS_PIXELFORMAT ddspf;
	DWORD           dwCaps;
	DWORD           dwCaps2;
	DWORD           dwCaps3;
	DWORD           dwCaps4;
	DWORD           dwReserved2;
} DDS_HEADER;

void
SaveDDS(int width, int height, void* data, unsigned long numBytes)
{
	FILE* fh = fopen("frame.bin", "wb");
	if (fh == NULL)
		return;

	unsigned int ddsMagic = 0x20534444;
	fwrite(&ddsMagic, 1, sizeof(ddsMagic), fh);

	DDS_HEADER header;
	memset(&header, 0, 124);
	header.dwSize = 124;
	header.dwFlags = 0x1 | 0x2 | 0x4 | 0x1000;
	header.dwHeight = height;
	header.dwWidth = width;
	int blockSize = 8;
	header.dwPitchOrLinearSize = max( 1, ((width+3)/4) ) * blockSize;
	header.ddspf.dwSize = 32;
	header.ddspf.dwFlags = 0x4;
	header.ddspf.dwFourCC = FOURCC_DXT1;
	header.dwCaps = 0x1000;
	fwrite(&header, 1, header.dwSize, fh);
	fwrite(data, 1, numBytes, fh);

	fclose(fh);
}

void
SaveDDS5(int width, int height, void* data, unsigned long numBytes)
{
	FILE* fh = fopen("frame5.bin", "wb");
	if (fh == NULL)
		return;

	unsigned int ddsMagic = 0x20534444;
	fwrite(&ddsMagic, 1, sizeof(ddsMagic), fh);

	DDS_HEADER header;
	memset(&header, 0, 124);
	header.dwSize = 124;
	header.dwFlags = 0x1 | 0x2 | 0x4 | 0x1000;
	header.dwHeight = height;
	header.dwWidth = width;
	int blockSize = 16;
	header.dwPitchOrLinearSize = max( 1, ((width+3)/4) ) * blockSize;
	header.ddspf.dwSize = 32;
	header.ddspf.dwFlags = 0x1 | 0x4;
	header.ddspf.dwFourCC = FOURCC_DXT5;
	header.dwCaps = 0x1000;
	fwrite(&header, 1, header.dwSize, fh);
	fwrite(data, 1, numBytes, fh);

	fclose(fh);
}

// called to compress a frame; the actual compression will be
// handed off to other functions depending on the color space and settings
DWORD
CodecInst::Compress(ICCOMPRESS* icinfo, DWORD dwSize)
{
	_out = (unsigned char *)icinfo->lpOutput;
	_in  = (const unsigned char *)icinfo->lpInput;

	if (icinfo->lFrameNum == 0)
	{
		if (_isStarted != 0x1337)
		{
			if (int error = CompressBegin(icinfo->lpbiInput,icinfo->lpbiOutput) != ICERR_OK)
			{
				return error;
			}
		}
	}
	else if (_useNullFrames)
	{
		// compare in two parts, video is probably more likely to change in middle than at bottom
		unsigned int pos = _length / 2 + 15;
		pos &= (~15);
		if (!memcmp(_in+pos, _prevBuffer+pos, _length-pos) && !memcmp(_in, _prevBuffer,pos) )
		{
			// Set the frame to NULL (duplicate of previous)
			icinfo->lpbiOutput->biSizeImage =0;
			*icinfo->lpdwFlags = 0;
			return ICERR_OK;
		}
	}

	if (icinfo->lpckid)
	{
		*icinfo->lpckid = 'cd';
	}
	
	int ret_val = ICERR_OK;

	int width = (int)icinfo->lpbiInput->biWidth;
	int height = abs((int)icinfo->lpbiInput->biHeight);
	assert(width == _width);
	assert(height == _height);

	// Keep a copy of this frame to compare against
	if (_useNullFrames)
	{
		memcpy(_prevBuffer, _in, _length);
	}

	unsigned char* input = (unsigned char*)_in;

	// Convert 24-bit to 32-bit as squish only supports 32-bit
	if (_format == RGB24)
	{
		Convert24bppTo32bpp(width, height, true, input, _buffer);
		input = _buffer;
	}
	else
	{
		ConvertBGRAtoRGBA(width, height, input, _buffer);
		input = _buffer;
	}

	const bool isSourceTopDown = (icinfo->lpbiInput->biHeight < 0);
	if (!isSourceTopDown)
	{
		FlipVerticallyInPlace(input, width * 4, height);
	}

	unsigned int compressorOptions = HapCompressorNone;
	if (_useSnappy)
		compressorOptions |= HapCompressorSnappy;

	unsigned long maxOutputSize = icinfo->dwFrameSize;
	if (icinfo->lpbiOutput->biSizeImage > maxOutputSize)
		maxOutputSize = icinfo->lpbiOutput->biSizeImage;

	unsigned long outputBufferBytesUsed = 0;
	switch (_codecType)
	{
	case Hap:
		outputBufferBytesUsed = CompressHap(input, icinfo->lpOutput, maxOutputSize, compressorOptions);
		break;
	case HapAlpha:
		outputBufferBytesUsed = CompressHapAlpha(input, icinfo->lpOutput, maxOutputSize, compressorOptions);
		break;
	case HapQ:
		outputBufferBytesUsed = CompressHapQ(input, icinfo->lpOutput, maxOutputSize, compressorOptions);
		break;
	}


	if (outputBufferBytesUsed == 0)
	{
		ret_val = ICERR_ERROR;
	}

	// set keyframe
	if (ret_val == ICERR_OK )
	{
		*icinfo->lpdwFlags = AVIIF_KEYFRAME;
		icinfo->lpbiOutput->biSizeImage = (DWORD)outputBufferBytesUsed;
	}

	return (DWORD)ret_val;
}
unsigned long
CodecInst::CompressHap(const unsigned char* inputBuffer, void* outputBuffer, unsigned long outputBufferBytes, unsigned int compressorOptions)
{
	unsigned long outputBufferBytesUsed = 0;

	// convert to DXT format
#ifdef USE_OPENMP_DXT
#pragma omp parallel
	{
		int numChunks = 4;
		int chunkHeight = _height / numChunks;
		int threadChunkBytes = _width * chunkHeight * 4;
		int threadDxtChunkBytes = threadChunkBytes / 8;
		int id = omp_get_thread_num();
		int offset1 = id*threadChunkBytes;
		int offset2 = id*threadDxtChunkBytes;
#pragma omp for 
		for (int i = 0; i < numChunks; i++)
		{
			squish::CompressImage(inputBuffer + offset1, _width, chunkHeight, (unsigned char*)_dxtBuffer + offset2, _dxtFlags, NULL);
		}
	}
#else
	squish::CompressImage(inputBuffer, _width, _height, _dxtBuffer, _dxtFlags, NULL);
#endif


	//SaveDDS(_width, _height, _dxtBuffer, _dxtBufferSize);

	// encode and compress as HAP frame

	if (HapResult_No_Error == HapEncode((unsigned char*)_dxtBuffer, _dxtBufferSize, HapTextureFormat_RGB_DXT1, compressorOptions, outputBuffer, outputBufferBytes, &outputBufferBytesUsed))
	{
		outputBufferBytesUsed = outputBufferBytesUsed;
	}


	/*
	// test decode

	void* buffer = new char[1024*1024*4];
	unsigned long bufferSize = 1024*1024*4;

	unsigned int outputBufferTextureFormat;
	if (HapResult_No_Error == HapDecode(outputBuffer, outputBufferBytesUsed, buffer, bufferSize, &outputBufferBytesUsed, &outputBufferTextureFormat))
	{
		outputBufferBytesUsed = outputBufferBytesUsed;
	}*/

	return outputBufferBytesUsed;
}

unsigned long
CodecInst::CompressHapAlpha(const unsigned char* inputBuffer, void* outputBuffer, unsigned long outputBufferBytes, unsigned int compressorOptions)
{
	unsigned long outputBufferBytesUsed = 0;

	// convert to DXT format
#ifdef USE_OPENMP_DXT
#pragma omp parallel
	{
		int numChunks = 4;
		int chunkHeight = _height / numChunks;
		int threadChunkBytes = _width * chunkHeight * 4;
		int threadDxtChunkBytes = threadChunkBytes / 4;
		int id = omp_get_thread_num();
		int offset1 = id*threadChunkBytes;
		int offset2 = id*threadDxtChunkBytes;
#pragma omp for 
		for (int i = 0; i < numChunks; i++)
		{
			squish::CompressImage(inputBuffer + offset1, _width, chunkHeight, (unsigned char*)_dxtBuffer + offset2, _dxtFlags, NULL);
		}
	}
#else
	squish::CompressImage(inputBuffer, _width, _height, _dxtBuffer, _dxtFlags, NULL);
#endif


	//SaveDDS5(_width, _height, _dxtBuffer, _dxtBufferSize);
	// encode and compress as HAP frame

	HapEncode((unsigned char*)_dxtBuffer, _dxtBufferSize, HapTextureFormat_RGBA_DXT5, compressorOptions, outputBuffer, outputBufferBytes, &outputBufferBytesUsed);

	return outputBufferBytesUsed;
}

unsigned long
CodecInst::CompressHapQ(const unsigned char* inputBuffer, void* outputBuffer, unsigned long outputBufferBytes, unsigned int compressorOptions)
{
	unsigned long outputBufferBytesUsed = 0;

	// Convert to YCoCg
	ConvertRGB_ToCoCg_Y8888((unsigned char*)inputBuffer, (unsigned char*)_buffer2, (unsigned long)_width, (unsigned long)_height, (size_t)_width*4, (size_t)_width*4, 0);

	// TODO: no DXT quality is set...

	// convert to DXT format
	int outputSize = CompressYCoCgDXT5((const unsigned char*)_buffer2, (unsigned char*)_dxtBuffer, _width, _height, _width * 4);
	assert(outputSize <= _dxtBufferSize);

	HapEncode((unsigned char*)_dxtBuffer, _dxtBufferSize, HapTextureFormat_YCoCg_DXT5, compressorOptions, outputBuffer, outputBufferBytes, &outputBufferBytesUsed);

	return outputBufferBytesUsed;
}