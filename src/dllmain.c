#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <psapi.h>
#include <hidsdi.h>
#include <string.h>
#include <detours/detours.h>
#include "logger.h"
#include "xinputmod.h"
#include "hooks.h"

void CALLBACK wmInputTimerHack( HWND param1, UINT param2, UINT_PTR param3, DWORD param4 );

HWND diabloWindow = NULL;

void CALLBACK wmInputTimerHack( HWND param1, UINT param2, UINT_PTR param3, DWORD param4 ) {
	static RAWINPUT dummy = {
		.header = { 0 },
		.data = { 0 }
	};

	//logPrintf( "Sending WM_INPUT\n" );

	// HACKHACKHACK
	SetPropA( diabloWindow, "userData", NULL );

	PostMessage( diabloWindow, WM_INPUT, RIM_INPUT, ( LPARAM ) &dummy );
}

BOOL WINAPI DllMain( HINSTANCE dll, DWORD reason, LPVOID reserved ) {
	LONG err = 0;

	if ( DetourIsHelperProcess( ) )
		return TRUE;

	switch ( reason ) {
		case DLL_PROCESS_ATTACH:
			logOpen( );
			logPrintf( "Built %s at %d\n", __DATE__, __TIME__ );
			logPrintf( "DllMain::DLL_PROCESS_ATTACH\n" );

			loadHooks( );
			loadXInput( );
			
			break;
		case DLL_PROCESS_DETACH:
			unloadHooks( );

			logPrintf( "DllMain::DLL_PROCESS_DETACH\n" );
			logClose( );

			unloadXInput( );
			break;
		default:
			break;
	};

	return TRUE;
}
