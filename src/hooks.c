#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <hidsdi.h>
#include <detours/detours.h>
#include <string.h>

#include "logger.h"

BOOL WINAPI hookRegisterRawInputDevices( PCRAWINPUTDEVICE rawInputDevices, UINT numDevices, UINT cbSize );
HWND WINAPI hookCreateWindowExA( DWORD exStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param );
BOOLEAN WINAPI hookHidD_GetProductStringProc( HANDLE hidDevObj, PVOID buffer, ULONG bufferLen );
UINT WINAPI hookGetRawInputDeviceInfoW( HANDLE device, UINT command, LPVOID data, PUINT size );
UINT WINAPI hookGetRawInputData( HANDLE device, UINT command, LPVOID data, PUINT size, UINT headerSize );
BOOL WINAPI hookSetWindowTextA( HWND window, LPCSTR newText );
BOOL WINAPI hookSetWindowTextW( HWND window, LPCWSTR newText );
BOOL WINAPI hookShowWindow( HWND window, int cmdShow );

HWND( WINAPI* realCreateWindowExA ) ( DWORD exStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param ) = CreateWindowExA;
BOOL( WINAPI* realRegisterRawInputDevices ) ( PCRAWINPUTDEVICE rawInputDevices, UINT numDevices, UINT cbSize ) = RegisterRawInputDevices;

BOOLEAN( WINAPI* realHidD_GetProductString ) ( HANDLE hidDevObj, PVOID buffer, ULONG bufferLen ) = HidD_GetProductString;

UINT( WINAPI* realGetRawInputDeviceInfoW ) ( HANDLE device, UINT command, LPVOID data, PUINT size ) = GetRawInputDeviceInfoW;
UINT( WINAPI* realGetRawInputData ) ( HANDLE device, UINT command, LPVOID data, PUINT size, UINT headerSize ) = GetRawInputData;

BOOL( WINAPI* realSetWindowTextW ) ( HWND window, LPCWSTR newText ) = SetWindowTextW;
BOOL( WINAPI* realSetWindowTextA ) ( HWND window, LPCSTR newText ) = SetWindowTextA;

BOOL( WINAPI* realShowWindow ) ( HWND window, int cmdShow ) = ShowWindow;

extern void CALLBACK wmInputTimerHack( HWND param1, UINT param2, UINT_PTR param3, DWORD param4 );
extern void CALLBACK hookWndProcTimer( HWND param1, UINT param2, UINT_PTR param3, DWORD param4 );

extern HWND diabloWindow;

WNDPROC oldWndProc = NULL;

LRESULT CALLBACK hookWndProc( HWND window, UINT msg, LPARAM lparam, WPARAM wparam ) {
	switch ( msg ) {
		//case WM_TIMER:
		//case WM_INPUT:
		//case WM_MOUSEMOVE:
		//case WM_LBUTTONDOWN:
		//case WM_SETCURSOR:
		//case WM_NCHITTEST:
		//case WM_LBUTTONUP:
		//	break;
		case WM_WINDOWPOSCHANGED:
			//CallWindowProc( oldWndProc, diabloWindow, WM_INPUT_DEVICE_CHANGE, GIDC_REMOVAL, 0 );
			//CallWindowProc( oldWndProc, diabloWindow, WM_INPUT_DEVICE_CHANGE, GIDC_ARRIVAL, 0 );
			//PostMessage( diabloWindow, WM_INPUT_DEVICE_CHANGE, GIDC_REMOVAL, ( LPARAM ) 0x00D1AB10 );
			PostMessage( diabloWindow, WM_INPUT_DEVICE_CHANGE, GIDC_ARRIVAL, ( LPARAM ) 0x00D1AB10 );
		default:
			logPrintf( "hookWndProc( 0x%p, 0x%p, 0x%p, 0x%p )\n", window, msg, lparam, wparam );
			break;
	};

	return CallWindowProc( oldWndProc, window, msg, lparam, wparam );
}

BOOL WINAPI hookRegisterRawInputDevices( PCRAWINPUTDEVICE rawInputDevices, UINT numDevices, UINT cbSize ) {
	BOOL res = realRegisterRawInputDevices( rawInputDevices, numDevices, cbSize );

	logPrintf( "%d = RegisterRawInputDevices( %p, %u, %u )\n", res, rawInputDevices, numDevices, cbSize );

	if ( rawInputDevices && rawInputDevices[ 0 ].hwndTarget == diabloWindow ) {
		logPrintf( "Registering raw input devices to Diablo II window\n" );

		PostMessage( diabloWindow, WM_INPUT_DEVICE_CHANGE, GIDC_REMOVAL, ( LPARAM ) 0x00D1AB10 );
		PostMessage( diabloWindow, WM_INPUT_DEVICE_CHANGE, GIDC_ARRIVAL, ( LPARAM ) 0x00D1AB10 );

		SetTimer( diabloWindow, 0xD1AB, 16, wmInputTimerHack );
		SetTimer( diabloWindow, 0x2121, 250, hookWndProcTimer );
	}

	return res;
}

HWND WINAPI hookCreateWindowExA( DWORD exStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param ) {
	HWND res = realCreateWindowExA( exStyle, className, windowName, style, x, y, width, height, parent, menu, instance, param );

	logPrintf(
		"0x%p = CreateWindowExA( exStyle = 0x%08X, className = %s, windowName = %s, style = 0x%08X, x = %d, y = %d, width = %d, height = %d, parent = 0x%p\n",
		res,
		exStyle,
		className,
		windowName,
		style,
		x,
		y,
		width,
		height,
		parent
	);

	if ( strcmp( className, "Diablo II" ) == 0 ) {
		diabloWindow = res;
		logPrintf( "Found Diablo II window (0x%p)\n", diabloWindow );
		logPrintf( "Subclassing WndProc\n" );
	}

	return res;
}

BOOLEAN WINAPI hookHidD_GetProductStringProc( HANDLE hidDevObj, PVOID buffer, ULONG bufferLen ) {
	BOOLEAN res = realHidD_GetProductString( hidDevObj, buffer, bufferLen );

	logPrintf( "%d = HidD_GetProductString( %p, %ls, %u )\n", res, hidDevObj, buffer, bufferLen );
	return res;
}

UINT WINAPI hookGetRawInputDeviceInfoW( HANDLE device, UINT command, LPVOID data, PUINT size ) {
	UINT res = realGetRawInputDeviceInfoW( device, command, data, size );

	if ( command == RIDI_DEVICENAME )
		logPrintf( "%d = GetRawInputDeviceInfoW( %p, 0x%08X, %ls, %u )\n", res, device, command, data, *size );
	else
		logPrintf( "%d = GetRawInputDeviceInfoW( %p, 0x%08X, 0x%p, %u )\n", res, device, command, data, *size );

	return res;
}

UINT WINAPI hookGetRawInputData( HANDLE device, UINT command, LPVOID data, PUINT size, UINT headerSize ) {
	UINT res = realGetRawInputData( device, command, data, size, headerSize );

	logPrintf( "%d = GetRawInputData( %p, 0x%08X, 0x%p, 0x%u, 0x%u )\n", res, device, command, data, *size, headerSize );
	return res;
}

BOOL WINAPI hookSetWindowTextA( HWND window, LPCSTR newText ) {
	BOOL res = realSetWindowTextA( window, newText );

	logPrintf( "%d = SetWindowTextA( 0x%p, %s )\n", res, window, newText );
	return res;
}

BOOL WINAPI hookSetWindowTextW( HWND window, LPCWSTR newText ) {
	BOOL res = realSetWindowTextW( window, newText );

	logPrintf( "%d = SetWindowTextW( 0x%p, %ls )\n", res, window, newText );
	return res;
}

BOOL WINAPI hookShowWindow( HWND window, int cmdShow ) {
	BOOL res = realShowWindow( window, cmdShow );

	logPrintf( "%d = ShowWindow( 0x%p, %d )\n", window, cmdShow );
	return res;
}

BOOL loadHooks( void ) {
	LONG err = 0;

	logPrintf( "Loading detours... " );
		DetourRestoreAfterWith( );

		DetourTransactionBegin( );
		DetourUpdateThread( GetCurrentThread( ) );
		DetourAttach( ( PVOID* ) &realRegisterRawInputDevices, hookRegisterRawInputDevices );
		DetourAttach( ( PVOID* ) &realCreateWindowExA, hookCreateWindowExA );

		DetourAttach( ( PVOID* ) &realHidD_GetProductString, hookHidD_GetProductStringProc );

		DetourAttach( ( PVOID* ) &realGetRawInputDeviceInfoW, hookGetRawInputDeviceInfoW );
		DetourAttach( ( PVOID* ) &realGetRawInputData, hookGetRawInputData );

		DetourAttach( ( PVOID* ) &realSetWindowTextA, hookSetWindowTextA );
		DetourAttach( ( PVOID* ) &realSetWindowTextW, hookSetWindowTextW );

		DetourAttach( ( PVOID* ) &realShowWindow, hookShowWindow );
		err = DetourTransactionCommit( );
	logPrintf( "detour result = %d\n", err );

	return ( err == 0 ) ? TRUE : FALSE;
}

void unloadHooks( void ) {
	DetourTransactionBegin( );
	DetourUpdateThread( GetCurrentThread( ) );
	DetourDetach( ( PVOID* ) &realRegisterRawInputDevices, hookRegisterRawInputDevices );
	DetourDetach( ( PVOID* ) &realCreateWindowExA, hookCreateWindowExA );

	DetourDetach( ( PVOID* ) &realHidD_GetProductString, hookHidD_GetProductStringProc );

	DetourDetach( ( PVOID* ) &realGetRawInputDeviceInfoW, hookGetRawInputDeviceInfoW );
	DetourDetach( ( PVOID* ) &realGetRawInputData, hookGetRawInputData );

	DetourDetach( ( PVOID* ) &realSetWindowTextA, hookSetWindowTextA );
	DetourDetach( ( PVOID* ) &realSetWindowTextW, hookSetWindowTextW );

	DetourDetach( ( PVOID* ) &realShowWindow, hookShowWindow );
	DetourTransactionCommit( );
}
