#pragma once
#include "interface.h"
#include "hapcodec.h"
#include <commctrl.h>
#include <shellapi.h>
#include <Windowsx.h>
#include <intrin.h>

const char* kRootRegistryKey = "Software\\HapCodec";

void StoreRegistrySettings(bool nullframes, bool useSnappy, int dxtQuality)
{
	DWORD dp;
	HKEY regkey;
	const char * DxtQualityStrings[3] = {"HIGH", "GOOD", "LOW "};
	if ( RegCreateKeyEx(HKEY_CURRENT_USER,kRootRegistryKey,0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&regkey,&dp) == ERROR_SUCCESS)
	{
		DWORD data = 0;
		if (nullframes) data = 1;
		RegSetValueEx(regkey,"NullFrames",0,REG_DWORD,(unsigned char*)&data,4);
		if (useSnappy) data = 1; else data = 0;
		RegSetValueEx(regkey,"UseSnappy",0,REG_DWORD,(unsigned char*)&data,4);
		RegSetValueEx(regkey,"DXTQuality",0,REG_SZ,(unsigned char*)DxtQualityStrings[dxtQuality],4);
		RegCloseKey(regkey);
	}
}

void LoadRegistrySettings(bool* nullFrames, bool* useSnappy, int* dxtQuality)
{
	HKEY regkey;
	unsigned char data[]={0,0,0,0,0,0,0,0};
	DWORD size=sizeof(data);
	if ( RegOpenKeyEx(HKEY_CURRENT_USER,kRootRegistryKey,0,KEY_READ,&regkey) == ERROR_SUCCESS)
	{
		if ( nullFrames )
		{
			RegQueryValueEx(regkey,"NullFrames",0,NULL,data,&size);
			*nullFrames = (data[0]>0);
			size=sizeof(data);
			*(int*)data=0;
		}
		if ( useSnappy)
		{
			RegQueryValueEx(regkey,"UseSnappy",0,NULL,data,&size);
			*useSnappy = (data[0]>0);
			size=sizeof(data);
			*(int*)data=0;
		}
		if ( dxtQuality )
		{
			RegQueryValueEx(regkey,"DxtQuality",0,NULL,data,&size);

			int cmp = *(int*)data;
			if ( cmp == 'HGIH' ){
				*dxtQuality=0;
			} else if ( cmp == 'DOOG'){
				*dxtQuality=1;
			} else if ( cmp == ' WOL'){
				*dxtQuality=2;
			} else {
				*dxtQuality=1;
			}
		}
		RegCloseKey(regkey);
	}
	else 
	{
		bool nf = GetPrivateProfileInt("settings", "NullFrames", false, "hapcodec.ini")>0;
		bool snappy = GetPrivateProfileInt("settings", "UseSnappy", false, "hapcodec.ini")>0;
		int dxtQ = GetPrivateProfileInt("settings", "DxtQuality", 1, "hapcodec.ini");
		if (dxtQ < 0 || dxtQ >= 3)
		{
			dxtQ = 1;
		} 
		StoreRegistrySettings(nf, snappy, dxtQ);
		if ( nullFrames ) *nullFrames = nf;
		if ( useSnappy ) *useSnappy = snappy;
		if (dxtQuality) *dxtQuality = dxtQ;
	}
}