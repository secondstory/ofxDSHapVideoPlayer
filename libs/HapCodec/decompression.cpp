
#include "hapcodec.h"
#include "interface.h"
#include <float.h>
#include <squish/squish.h>
#include <hap/hap.h>
#include <hap/YCoCgDXT.h>
#include <hap/YCoCg.h>

// initialize the codec for decompression
DWORD
CodecInst::DecompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{	
	if (_isStarted == 0x1337)
	{
		DecompressEnd();
	}
	_isStarted = 0;

	if ( int error = DecompressQuery(lpbiIn,lpbiOut) != ICERR_OK )
	{
		return error;
	}

	_dxtFlags = 0;
	_dxtBufferSize = 0;
	_buffer = _buffer2 = NULL;
	_dxtBuffer = NULL;

	// Collect properties
	_width = lpbiIn->biWidth;
	_height = lpbiIn->biHeight;

	if (lpbiOut->biCompression == 0)
	{
		_format = lpbiOut->biBitCount;

		_dxtFlags = GetDxtFlags();
		_dxtBufferSize = squish::GetStorageRequirements(_width, _height, _dxtFlags);
		// Allocate temporary buffers
		_length = _width * _height * _format / 8;
		int bufferSize = _length + 2048;
		bufferSize=align_round(_width * 4, 8) * _height + 2048;

		_dxtBuffer = (void*)lag_aligned_malloc(_dxtBuffer, _dxtBufferSize, 16, "dxtBuffer");
		_buffer = (unsigned char *)lag_aligned_malloc( _buffer, bufferSize, 16, "buffer");
		_buffer2 = (unsigned char *)lag_aligned_malloc( _buffer2, bufferSize, 16, "buffer2");

		if (!_buffer || !_buffer2 || !_dxtBuffer)
		{
			return ICERR_MEMORY;
		}
	}
	else
	{
		_format = lpbiOut->biCompression;
	}

	_isStarted = 0x1337;

	return ICERR_OK;
}

// release resources when decompression is done
DWORD CodecInst::DecompressEnd()
{
	if ( _isStarted == 0x1337 )
	{
		lag_aligned_free(_buffer, "buffer");
		lag_aligned_free(_buffer2, "buffer2");
		lag_aligned_free(_dxtBuffer, "dxtBuffer");
	}
	_isStarted = 0;
	return ICERR_OK;
}

extern void ConvertBGRAtoRGBA(int width, int height, const unsigned char* a, unsigned char* b);
extern void FlipVerticallyInPlace(unsigned char* buffer, int stride, int height);

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

// Called to decompress a frame, the actual decompression will be
// handed off to other functions based on the frame type.
DWORD CodecInst::Decompress(ICDECOMPRESS* icinfo, DWORD dwSize) 
{
//	try {

	DWORD return_code = ICERR_OK;
	if (_isStarted != 0x1337)
	{
		DecompressBegin(icinfo->lpbiInput, icinfo->lpbiOutput);
	}

	_out = (unsigned char *)icinfo->lpOutput;
	_in  = (const unsigned char *)icinfo->lpInput;
	//icinfo->lpbiOutput->biSizeImage = 0;

	int width = (int)icinfo->lpbiInput->biWidth;
	int height = (int)icinfo->lpbiInput->biHeight;
	compressed_size = icinfo->lpbiInput->biSizeImage;
	assert(width == _width);
	assert(height == _height);

	// according to the avi specs, the calling application is responsible for handling null frames.
	if ( compressed_size == 0 )
	{
		#ifndef LAGARITH_RELEASE
		MessageBox (HWND_DESKTOP, "Received request to decode a null frame", "Error", MB_OK | MB_ICONEXCLAMATION);
		#endif
		return ICERR_OK;
	}

	unsigned int outputBufferTextureFormat;
	unsigned long outputBufferBytesUsed;

	if (_format == RGB24)
	{
		if (HapResult_No_Error == HapDecode(_in, icinfo->lpbiInput->biSizeImage, _dxtBuffer, _dxtBufferSize, &outputBufferBytesUsed, &outputBufferTextureFormat))
		{
			if (outputBufferTextureFormat == HapTextureFormat_YCoCg_DXT5)
			{
				int outputSize = DeCompressYCoCgDXT5((const byte*)_dxtBuffer, _buffer, _width, _height, _width * 4);
				assert(outputSize == _width * _height * 4);

				ConvertCoCg_Y8888ToBGR_(_buffer, _buffer2, _width, _height, _width * 4, _width * 4, 0);

				// Convert 32-bit to 24-bit
				Convert32bppTo24bpp(_width, _height, false, _buffer2, _out);
			}
			else
			{
				// Convert DXT to RGBA
				squish::DecompressImage(_buffer, _width, _height, _dxtBuffer, _dxtFlags);

				// Convert 32-bit to 24-bit and swap red-blue channels
				Convert32bppTo24bpp(_width, _height, true, _buffer, _out);
			}

			icinfo->lpbiOutput->biSizeImage = _length;

			// if the output is bottom-up we need to flip our output
			if (icinfo->lpbiOutput->biHeight > 0)
			{
				FlipVerticallyInPlace(_out, _width * 3, _height);
			}
		}
	}
	else if (_format == RGB32)
	{
		if (HapResult_No_Error == HapDecode(_in, icinfo->lpbiInput->biSizeImage, _dxtBuffer, _dxtBufferSize, &outputBufferBytesUsed, &outputBufferTextureFormat))
		{
			if (outputBufferTextureFormat == HapTextureFormat_YCoCg_DXT5)
			{
				int outputSize = DeCompressYCoCgDXT5((const byte*)_dxtBuffer, _buffer, _width, _height, _width * 4);
				assert(outputSize == _width * _height * 4);

				ConvertCoCg_Y8888ToBGR_(_buffer, _out, _width, _height, _width * 4, _width * 4, 0);
			}
			else
			{
				// Convert DXT to RGBA
				squish::DecompressImage(_buffer, _width, _height, _dxtBuffer, _dxtFlags);

				// Swap red-blue channels
				ConvertBGRAtoRGBA(_width, _height, _buffer, _out);
			}

			icinfo->lpbiOutput->biSizeImage = _length;

			// if the output is bottom-up we need to flip our output
			if (icinfo->lpbiOutput->biHeight > 0)
			{
				FlipVerticallyInPlace(_out, _width * 4, _height);
			}
		}
	}
	else
	{
		// Decompress to DXT compressed formats
		if (HapResult_No_Error == HapDecode(_in, icinfo->lpbiInput->biSizeImage, _out, icinfo->lpbiOutput->biSizeImage, &outputBufferBytesUsed, &outputBufferTextureFormat))
		{
			icinfo->lpbiOutput->biSizeImage = outputBufferBytesUsed;
		}
	}

	return return_code;
}