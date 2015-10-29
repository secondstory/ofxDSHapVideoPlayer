// Based off of Ben Rudiak-Gould's huffyuv source code


#include "hapcodec.h"


/***************************************************************************
 * DriverProc  -  The entry point for an installable driver.
 *
 * PARAMETERS
 * dwDriverId:  For most messages, <dwDriverId> is the DWORD
 *     value that the driver returns in response to a <DRV_OPEN> message.
 *     Each time that the driver is opened, through the <DrvOpen> API,
 *     the driver receives a <DRV_OPEN> message and can return an
 *     arbitrary, non-zero value. The installable driver interface
 *     saves this value and returns a unique driver handle to the
 *     application. Whenever the application sends a message to the
 *     driver using the driver handle, the interface routes the message
 *     to this entry point and passes the corresponding <dwDriverId>.
 *     This mechanism allows the driver to use the same or different
 *     identifiers for multiple opens but ensures that driver handles
 *     are unique at the application interface layer.
 *
 *     The following messages are not related to a particular open
 *     instance of the driver. For these messages, the dwDriverId
 *     will always be zero.
 *
 *         DRV_LOAD, DRV_FREE, DRV_ENABLE, DRV_DISABLE, DRV_OPEN
 *
 * hDriver: This is the handle returned to the application by the
 *    driver interface.
 *
 * uiMessage: The requested action to be performed. Message
 *     values below <DRV_RESERVED> are used for globally defined messages.
 *     Message values from <DRV_RESERVED> to <DRV_USER> are used for
 *     defined driver protocols. Messages above <DRV_USER> are used
 *     for driver specific messages.
 *
 * lParam1: Data for this message.  Defined separately for
 *     each message
 *
 * lParam2: Data for this message.  Defined separately for
 *     each message
 *
 * RETURNS
 *   Defined separately for each message.
 *
 ***************************************************************************/


LRESULT PASCAL DriverProc_Common(DWORD_PTR dwDriverID, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2) 
{
	CodecInst* pi = (CodecInst*)dwDriverID;
	switch (uiMessage) 
	{
		case DRV_FREE:
		case DRV_LOAD:
			return (LRESULT)1L;

		//case DRV_OPEN:
		//	return (LRESULT)Open((ICOPEN*) lParam2, CodecInst::Hap);

		case DRV_CLOSE:
			if (pi) Close(pi);
			return (LRESULT)1L;

		/*********************************************************************

		  state messages

		*********************************************************************/

    // cwk
    case DRV_QUERYCONFIGURE:    // configuration from drivers applet
      return (LRESULT)1L;

    case DRV_CONFIGURE:
      pi->Configure((HWND)lParam1);
      return DRV_OK;

    case ICM_CONFIGURE:
      //
      //  return ICERR_OK if you will do a configure box, error otherwise
      //
	  if (lParam1 == -1)
        return ICERR_OK;
      else
        return pi->Configure((HWND)lParam1);

    case ICM_ABOUT:
	  if (lParam1 == -1)
        return ICERR_OK;
      else
		return pi->About((HWND)lParam1);
        //return ICERR_UNSUPPORTED;

    case ICM_GETSTATE:
      return pi->GetState((LPVOID)lParam1, (DWORD)lParam2);

    case ICM_SETSTATE:
      return pi->SetState((LPVOID)lParam1, (DWORD)lParam2);

    case ICM_GETINFO:
      return pi->GetInfo((ICINFO*)lParam1, (DWORD)lParam2);

    case ICM_GETDEFAULTQUALITY:
      if (lParam1) {
        *((LPDWORD)lParam1) = 10000;
        return ICERR_OK;
      }
      break;

    /*********************************************************************

      compression messages

    *********************************************************************/

    case ICM_COMPRESS_QUERY:
      return pi->CompressQuery((LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2);

    case ICM_COMPRESS_BEGIN:
      return pi->CompressBegin((LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2);

    case ICM_COMPRESS_GET_FORMAT:
      return pi->CompressGetFormat((LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2);

    case ICM_COMPRESS_GET_SIZE:
      return pi->CompressGetSize((LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2);

    case ICM_COMPRESS:
      return pi->Compress((ICCOMPRESS*)lParam1, (DWORD)lParam2);

    case ICM_COMPRESS_END:
      return pi->CompressEnd();

    /*********************************************************************

      decompress messages

    *********************************************************************/

    case ICM_DECOMPRESS_QUERY:
		
      return pi->DecompressQuery((LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2);

    case ICM_DECOMPRESS_BEGIN:
      return  pi->DecompressBegin((LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2);

    case ICM_DECOMPRESS_GET_FORMAT:
      return pi->DecompressGetFormat((LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2);

    case ICM_DECOMPRESS_GET_PALETTE:
      return pi->DecompressGetPalette((LPBITMAPINFOHEADER)lParam1, (LPBITMAPINFOHEADER)lParam2);

    case ICM_DECOMPRESS:
      return pi->Decompress((ICDECOMPRESS*)lParam1, (DWORD)lParam2);

    case ICM_DECOMPRESS_END:
      return pi->DecompressEnd();

    /*********************************************************************

      standard driver messages

    *********************************************************************/

    case DRV_DISABLE:
    case DRV_ENABLE:
      return (LRESULT)1L;

    case DRV_INSTALL:
    case DRV_REMOVE:
      return (LRESULT)DRV_OK;
  }
  
  if (uiMessage < DRV_USER)
    return DefDriverProc(dwDriverID, hDriver, uiMessage, lParam1, lParam2);
  return ICERR_UNSUPPORTED;
}


LRESULT PASCAL DriverProc_Hap(DWORD_PTR dwDriverID, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2) 
{
	switch (uiMessage) 
	{
	case DRV_OPEN:
		{
			ICINFO *icinfo = (ICINFO *)lParam2;

			if (icinfo->fccHandler != FOURCC_HAP &&
				icinfo->fccHandler != FOURCC_hap)
			{
				return 0;
			}
			
			return (LRESULT)Open((ICOPEN*) lParam2, CodecInst::Hap);
		}
	default:
		return DriverProc_Common(dwDriverID, hDriver, uiMessage, lParam1, lParam2);
	}

	if (uiMessage < DRV_USER)
		return DefDriverProc(dwDriverID, hDriver, uiMessage, lParam1, lParam2);

	return ICERR_UNSUPPORTED;
}

LRESULT PASCAL DriverProc_HapAlpha(DWORD_PTR dwDriverID, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2) 
{
	switch (uiMessage) 
	{
	case DRV_OPEN:
		{
			ICINFO *icinfo = (ICINFO *)lParam2;

			if (icinfo->fccHandler != FOURCC_HAPA &&
				icinfo->fccHandler != FOURCC_hapa)
			{
				return 0;
			}

			return (LRESULT)Open((ICOPEN*) lParam2, CodecInst::HapAlpha);
		}
	default:
		return DriverProc_Common(dwDriverID, hDriver, uiMessage, lParam1, lParam2);
	}

	if (uiMessage < DRV_USER)
		return DefDriverProc(dwDriverID, hDriver, uiMessage, lParam1, lParam2);

	return ICERR_UNSUPPORTED;
}

LRESULT PASCAL DriverProc_HapQ(DWORD_PTR dwDriverID, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2) 
{
	switch (uiMessage) 
	{
	case DRV_OPEN:
		{
			ICINFO *icinfo = (ICINFO *)lParam2;

			if (icinfo->fccHandler != FOURCC_HAPQ &&
				icinfo->fccHandler != FOURCC_hapq)
			{
				return 0;
			}

			return (LRESULT)Open((ICOPEN*) lParam2, CodecInst::HapQ);
		}
	default:
		return DriverProc_Common(dwDriverID, hDriver, uiMessage, lParam1, lParam2);
	}

	if (uiMessage < DRV_USER)
		return DefDriverProc(dwDriverID, hDriver, uiMessage, lParam1, lParam2);

	return ICERR_UNSUPPORTED;
}

//
//  if you want to have multiple compressors in a single module add them
//  to this list, for example you can have a video capture module and a
//  video codec in the same module.
//
DRIVERPROC  DriverProcs[] = 
{
	DriverProc_Hap,
	DriverProc_HapAlpha,
	DriverProc_HapQ,

	NULL              // FENCE: must be last
};

#define BOGUS_DRIVER_ID		1

// All outside calls go through here.
LRESULT PASCAL DriverProc(DWORD_PTR dwDriverID, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2) 
{
	int i = -1;
	DRIVERPROC dp;

	switch(uiMessage)  
	{
	case DRV_LOAD:
		while( dp = DriverProcs[++i])
		{
			if( !dp( dwDriverID, hDriver, uiMessage, lParam1, lParam2))
				return 0;
		}
		return 1;

	case DRV_FREE:
		while( dp = DriverProcs[++i])
		{
			dp( dwDriverID, hDriver, uiMessage, lParam1, lParam2);
		}
		return 1;

	case DRV_OPEN:
		{
			// Open a driver instance.
			// drivID = 0L.
			// x is a far pointer to a zero-terminated string
			// containing the name used to open the driver.
			// y is passed through from the drvOpen call. It is
			// NULL if this open is from the Drivers Applet in control.exe
			// It is LPVIDEO_OPEN_PARMS otherwise.
			// Return 0L to fail the open.

			//	if we were opened without an open structure then just
			//	return a phony (non zero) id so the OpenDriver() will work.

			if( (LPVOID)lParam2 == NULL)
				return BOGUS_DRIVER_ID;

			ICINFO *icinfo = (ICINFO *)lParam2;

			if (icinfo && icinfo->fccType != ICTYPE_VIDEO)
				return 0;

			// use first DriverProc that accepts the input type
			while( dp = DriverProcs[++i])  
			{
				const LRESULT dw = dp( dwDriverID, hDriver, uiMessage, lParam1, lParam2);
				if(dw)
					return dw;
			}
			return 0;	// no takers
		}

	case DRV_QUERYCONFIGURE:
		return (LRESULT)1L;

    /*********************************************************************

      standard driver messages

    *********************************************************************/

	case DRV_DISABLE:
	case DRV_ENABLE:
		while( dp = DriverProcs[++i])
		{
			dp(  dwDriverID, hDriver, uiMessage, lParam1, lParam2);
		}
		return 1;

	case DRV_INSTALL:
	case DRV_REMOVE:
		return (LRESULT)DRV_OK;
	default:  
		{
			return DriverProc_Common(dwDriverID, hDriver, uiMessage, lParam1, lParam2);
			//return ((CodecInst *)dwDriverID)->DriverProc( dwDriverID, hDriver, uiMessage, lParam1, lParam2);
		}
	}
}
