#pragma once
#include "interface.h"
#include "hapcodec.h"
#include <commctrl.h>
#include <shellapi.h>
#include <Windowsx.h>
#include <intrin.h>

extern CodecInst* g_currentUICodec;

const char *dxtQuality_options[] = {"Very High", "High - default", "Low"};

extern void StoreRegistrySettings(bool nullframes, bool useSnappy, int dxtQuality);
extern void LoadRegistrySettings(bool* nullFrames, bool* useSnappy, int* dxtQuality);

HWND CreateTooltip(HWND hwnd)
{
    // initialize common controls
	INITCOMMONCONTROLSEX	iccex;		// struct specifying control classes to register
    iccex.dwICC		= ICC_WIN95_CLASSES;
    iccex.dwSize	= sizeof(INITCOMMONCONTROLSEX);
    InitCommonControlsEx(&iccex);

#ifdef X64_BUILD
	HINSTANCE	ghThisInstance=(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
#else 
	HINSTANCE	ghThisInstance=(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);
#endif
    HWND		hwndTT;					// handle to the tooltip control

    // create a tooltip window
	hwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
							CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
							hwnd, NULL, ghThisInstance, NULL);
	
	SetWindowPos(hwndTT, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	// set some timeouts so tooltips appear fast and stay long (32767 seems to be a limit here)
	SendMessage(hwndTT, TTM_SETDELAYTIME, (WPARAM)(DWORD)TTDT_INITIAL, (LPARAM)500);
	SendMessage(hwndTT, TTM_SETDELAYTIME, (WPARAM)(DWORD)TTDT_AUTOPOP, (LPARAM)30*1000);

	return hwndTT;
}

struct { UINT item; UINT tip; } item2tip[] = 
{
	{ IDC_NULLFRAMES,	IDS_TIP_NULLFRAMES	},
	{ IDC_SNAPPY,		IDS_TIP_SNAPPY		},
	{ IDC_MODE_OPTIONS,	IDS_TIP_MODE_OPTION},
	{ 0,0 }
};

int AddTooltip(HWND tooltip, HWND client, UINT stringid)
{
#ifdef X64_BUILD
	HINSTANCE ghThisInstance=(HINSTANCE)GetWindowLongPtr(client,GWLP_HINSTANCE);
#else
	HINSTANCE ghThisInstance=(HINSTANCE)GetWindowLong(client, GWL_HINSTANCE);
#endif

	TOOLINFO				ti;			// struct specifying info about tool in tooltip control
    static unsigned int		uid	= 0;	// for ti initialization
	RECT					rect;		// for client area coordinates
	TCHAR					buf[2000];	// a static buffer is sufficent, TTM_ADDTOOL seems to copy it

	// load the string manually, passing the id directly to TTM_ADDTOOL truncates the message :(
	if ( !LoadString(ghThisInstance, stringid, buf, 2000) ) return -1;

	// get coordinates of the main client area
	GetClientRect(client, &rect);
	
    // initialize members of the toolinfo structure
	ti.cbSize		= sizeof(TOOLINFO);
	ti.uFlags		= TTF_SUBCLASS;
	ti.hwnd			= client;
	ti.hinst		= ghThisInstance;		// not necessary if lpszText is not a resource id
	ti.uId			= uid;
	ti.lpszText		= buf;

	// Tooltip control will cover the whole window
	ti.rect.left	= rect.left;    
	ti.rect.top		= rect.top;
	ti.rect.right	= rect.right;
	ti.rect.bottom	= rect.bottom;
	
	// send a addtool message to the tooltip control window
	SendMessage(tooltip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);	
	return uid++;
}

BOOL CALLBACK AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	if (uMsg == WM_INITDIALOG)
	{
	}
	else if (uMsg == WM_COMMAND) 
	{
		if (LOWORD(wParam)==IDC_OK)
		{
			EndDialog(hwndDlg, 0);
		}
		else if ( LOWORD(wParam)==IDC_CANCEL )
		{
			EndDialog(hwndDlg, 0);
		}
	}
	else if ( uMsg == WM_CLOSE )
	{
		EndDialog(hwndDlg, 0);
	}
	return 0;
}

BOOL CALLBACK ConfigureDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_INITDIALOG)
	{
		bool nullframes=false;
		bool snappy=true;
		int dxtQuality = 1;
		LoadRegistrySettings(&nullframes, &snappy, &dxtQuality);

		HWND hwndItem;
		/*= GetDlgItem(hwndDlg, IDC_MODE_OPTIONS);
		for(int i=0; i<3; i++)
			SendMessage(hwndItem, LB_ADDSTRING, 0, (LPARAM)mode_options[i]);
		SendMessage(hwndItem, LB_SETCURSEL, mode, 1);*/

		hwndItem = GetDlgItem(hwndDlg, IDC_DXTQUALITY_OPTIONS);
		EnableWindow(hwndItem, true);
		for(int i=0; i<3; i++)
			SendMessage(hwndItem, LB_ADDSTRING, 0, (LPARAM)dxtQuality_options[i]);
		SendMessage(hwndItem, LB_SETCURSEL, dxtQuality, 1);

		CheckDlgButton(hwndDlg, IDC_NULLFRAMES, nullframes);
		CheckDlgButton(hwndDlg, IDC_SNAPPY, snappy);

		// Disable the quality dialog for HAPQ
		if (g_currentUICodec != NULL)
		{
			if (g_currentUICodec->_codecType == CodecInst::HapQ)
			{
				EnableWindow(hwndItem, false);
			}
		}

#ifndef _DEBUG
		{
			HWND snappyHandle = GetDlgItem(hwndDlg, IDC_SNAPPY);
			EnableWindow(snappyHandle, false);
			//CheckDlgButton(hwndDlg, IDC_SNAPPY, true);
		}
#endif

		//HWND suggest = GetDlgItem(hwndDlg, IDC_SUGGEST);
		//Button_Enable(suggest,!noupsample);

		HWND hwndTip = CreateTooltip(hwndDlg);
		for (int l=0; item2tip[l].item; l++ )
			AddTooltip(hwndTip, GetDlgItem(hwndDlg, item2tip[l].item),	item2tip[l].tip);
		SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT)350);
	}
	else if (uMsg == WM_COMMAND)
	{
		//HWND suggest = GetDlgItem(hwndDlg, IDC_SUGGEST);
		//Button_Enable(suggest,IsDlgButtonChecked(hwndDlg, IDC_NOUPSAMPLE) != BST_CHECKED);
		if (LOWORD(wParam)==IDC_OK)
		{
			bool nullframes=(IsDlgButtonChecked(hwndDlg, IDC_NULLFRAMES) == BST_CHECKED);
			bool snappy=(IsDlgButtonChecked(hwndDlg, IDC_SNAPPY) == BST_CHECKED);

			int dxtQuality = (int)SendDlgItemMessage(hwndDlg, IDC_DXTQUALITY_OPTIONS, LB_GETCURSEL, 0, 0);
			if ( dxtQuality <0 || dxtQuality >=3 )
				dxtQuality=1;

			StoreRegistrySettings(nullframes, snappy, dxtQuality);
		
			EndDialog(hwndDlg, 0);
		}
		else if ( LOWORD(wParam)==IDC_CANCEL )
		{
			EndDialog(hwndDlg, 0);
		}
		//else if ( LOWORD(wParam)==IDC_HOMEPAGE )
		//{
		//ShellExecute(NULL, "open", "http://lags.leetcode.net/codec.html", NULL, NULL, SW_SHOW);
		//}
	} 
	else if ( uMsg == WM_CLOSE )
	{
		EndDialog(hwndDlg, 0);
	}
	return 0;
}
