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

HWND( WINAPI* realCreateWindowExA ) ( DWORD exStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param ) = CreateWindowExA;
BOOL( WINAPI* realRegisterRawInputDevices ) ( PCRAWINPUTDEVICE rawInputDevices, UINT numDevices, UINT cbSize ) = RegisterRawInputDevices;

BOOLEAN( WINAPI* realHidD_GetProductString ) ( HANDLE hidDevObj, PVOID buffer, ULONG bufferLen ) = HidD_GetProductString;

UINT( WINAPI* realGetRawInputDeviceInfoW ) ( HANDLE device, UINT command, LPVOID data, PUINT size ) = GetRawInputDeviceInfoW;
UINT( WINAPI* realGetRawInputData ) ( HANDLE device, UINT command, LPVOID data, PUINT size, UINT headerSize ) = GetRawInputData;

BOOL( WINAPI* realSetWindowTextW ) ( HWND window, LPCWSTR newText ) = SetWindowTextW;
BOOL( WINAPI* realSetWindowTextA ) ( HWND window, LPCSTR newText ) = SetWindowTextA;

extern void CALLBACK wmInputTimerHack( HWND param1, UINT param2, UINT_PTR param3, DWORD param4 );
extern HWND diabloWindow;

BOOL WINAPI hookRegisterRawInputDevices( PCRAWINPUTDEVICE rawInputDevices, UINT numDevices, UINT cbSize ) {
	BOOL res = realRegisterRawInputDevices( rawInputDevices, numDevices, cbSize );

	logPrintf( "%d = RegisterRawInputDevices( %p, %u, %u )\n", res, rawInputDevices, numDevices, cbSize );

	if ( rawInputDevices && rawInputDevices[ 0 ].hwndTarget == diabloWindow ) {
		logPrintf( "Registering raw input devices to Diablo II window\n" );

		PostMessage( diabloWindow, WM_INPUT_DEVICE_CHANGE, GIDC_ARRIVAL, ( LPARAM ) 0x00D1AB10 );
		SetTimer( diabloWindow, 0xD1AB, 16, wmInputTimerHack );
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
		logPrintf( "Found Diablo II window (0x%p)", diabloWindow );
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
	DetourTransactionCommit( );
}
