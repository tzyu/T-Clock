/*-----------------------------------------------------
// main.c : API, hook procedure -> KAZUBON 1997-2001
//-------------------------------------------------------*/
// Modified by Stoic Joker: Tuesday, March 2 2010 - 10:42:42
#include "tcdll.h"

void InitClock(HWND hwnd);

//========================================================================================
//-----------------------------+++--> Entry Point of This (SystemTray Clock ShellHook) DLL:
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)   //---+++-->
{
	(void)lpvReserved;
	api.hInstance = hinstDLL;
	switch(fdwReason) {
	case DLL_PROCESS_ATTACH:
		#if !defined(_MSC_VER) || !defined(_DLL) // if not static MSVC build
		DisableThreadLibraryCalls(hinstDLL);
		#endif
		break;
	case DLL_PROCESS_DETACH:
		break;
	default: break;
	}
	return TRUE;
}
//========================================================================================
//---------------------------------------------+++--> SystemTray Clock ShellHook Procedure:
LRESULT CALLBACK Hook_CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)   //-----+++-->
{/// @todo : rewrite hooking code (use wrapper exe to hook it)
	CWPSTRUCT* pcwps = (CWPSTRUCT*)lParam;
	if(nCode >= 0 && pcwps && pcwps->hwnd) { // if this message is sent to the clock
		char classname[80];
		if(!gs_hwndClock && GetClassNameA(pcwps->hwnd, classname, 80) && !strcmp(classname, "TrayClockWClass")) {
			InitClock(pcwps->hwnd); // initialize  cf. wndproc.c
			PostMessage(gs_hwndTClockMain,MAINM_CLOCKINIT,0,(LPARAM)pcwps->hwnd);
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}
