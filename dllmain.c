#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <psapi.h>
#include <string.h>
#include "logger.h"
#include "xinputmod.h"

LRESULT CALLBACK hookedWndProc( HWND window, UINT msg, WPARAM wparam, LPARAM lparam );
DWORD WINAPI messageThread( LPVOID param );
BOOL CALLBACK findDiabloWindowCB( HWND window, LPARAM lparam );
HWND findDiabloWindow( void );

HANDLE messageThreadHandle = INVALID_HANDLE_VALUE;
DWORD messageThreadId = 0;

volatile BOOL messageThreadRun = TRUE;

WNDPROC oldWndProc = NULL;
HWND diabloWindow = NULL;

LRESULT CALLBACK hookedWndProc( HWND window, UINT msg, WPARAM wparam, LPARAM lparam ) {
	//switch ( msg ) {
	//	case WM_INPUT:
	//		logPrintf( "%s: WM_INPUT\n", __FUNCTION__ );
	//		break;
	//	case WM_INPUT_DEVICE_CHANGE:
	//		logPrintf( "%s: WM_INPUT_DEVICE_CHANGE\n", __FUNCTION__ );
	//		break;
	//	default:
	//		break;
	//};

	return oldWndProc( window, msg, wparam, lparam );
}

DWORD WINAPI messageThread( LPVOID param ) {
	logPrintf( "Entering message thread\n" );

	logPrintf( "Waiting for Diablo II...\n" );
		do {
			Sleep( 250 );
		} while ( ! findDiabloWindow( ) );
	logPrintf( "Found Diablo II!\n" );

	logPrintf( "Attempting to hook WndProc... " );
		oldWndProc = ( WNDPROC ) SetWindowLongPtr( diabloWindow, GWLP_WNDPROC, ( LONG ) hookedWndProc );
	logPrintf( "%s\n", ( oldWndProc == NULL ) ? "failed!" : "hooked!" );

	if ( oldWndProc ) {
		Sleep( 250 );

		logPrintf( "Sending WM_INPUT_DEVICE_CHANGE" );
		CallWindowProc( oldWndProc, diabloWindow, WM_INPUT_DEVICE_CHANGE, GIDC_ARRIVAL, 0 );
		logPrintf( "Sent!\n" );

		while ( messageThreadRun ) {
			CallWindowProc( oldWndProc, diabloWindow, WM_INPUT, 0, 0 );
			Sleep( 16 );
		}
	}

	logPrintf( "Leaving message thread\n" );
	return 0;
}

BOOL WINAPI DllMain( HINSTANCE dll, DWORD reason, LPVOID reserved ) {
	switch ( reason ) {
		case DLL_PROCESS_ATTACH:
			logOpen( );
			logPrintf( "DllMain::DLL_PROCESS_ATTACH\n" );

			if ( loadXInput( ) == TRUE ) {
				messageThreadHandle = CreateThread( NULL, 0, messageThread, NULL, 0, &messageThreadId );
			}

			break;
		case DLL_PROCESS_DETACH:
			logPrintf( "DllMain::DLL_PROCESS_DETACH\n" );
			logClose( );

			if ( messageThreadHandle != INVALID_HANDLE_VALUE ) {
				messageThreadRun = FALSE;

				WaitForSingleObject( messageThreadHandle, 1000 );
				CloseHandle( messageThreadHandle );
			}

			unloadXInput( );
			break;
		default:
			break;
	};

	return TRUE;
}

BOOL CALLBACK findDiabloWindowCB( HWND window, LPARAM lparam ) {
	CHAR windowText[ 1024 ];
	CHAR windowClass[ 1024 ];

	if ( IsWindow( window ) && IsWindowEnabled( window ) && IsWindowVisible( window ) ) {
		memset( windowText, 0, sizeof( windowText ) );
		memset( windowClass, 0, sizeof( windowClass ) );

		GetWindowText( window, windowText, sizeof( windowText ) );
		RealGetWindowClass( window, windowClass, sizeof( windowClass ) );

		logPrintf( "%s: Found window { Text: %s, Class: %s }\n", __FUNCTION__, windowText, windowClass );

		if ( strcmp( windowClass, "Diablo II" ) == 0 ) {
			logPrintf( "%s: Found Diablo II; stopping window enumeration\n", __FUNCTION__ );

			diabloWindow = window;
			return FALSE;
		}
	}
		
	return TRUE;
}

HWND findDiabloWindow( void ) {
	EnumWindows( findDiabloWindowCB, 0 );
	return diabloWindow;
}
