#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Xinput.h>
#include <time.h>
#include <stdlib.h>
#include "xinputmod.h"
#include "logger.h"

typedef DWORD( WINAPI* XInputGetStateProc ) ( _In_ DWORD userIndex, _Out_ XINPUT_STATE* state );
typedef DWORD( WINAPI* XInputSetStateProc ) ( _In_ DWORD userIndex, _In_ XINPUT_VIBRATION* vibe );

static XInputGetStateProc origXInputGetState = NULL;
static XInputSetStateProc origXInputSetState = NULL;

static HMODULE realXInputDLL = NULL;

DWORD WINAPI XInputGetState( _In_ DWORD userIndex, _Out_ XINPUT_STATE* state ) WIN_NOEXCEPT {
	int res = 0;

	//logPrintf( "%s: %lu returning ", __FUNCTION__, userIndex );

	if ( origXInputGetState ) {
		res = origXInputGetState( userIndex, state );

		logPrintf( "%d = XInputGetState( %d ) {\n", res, ( int ) userIndex );
			logPrintf( "\tlstick: { %d, %d }\n", state->Gamepad.sThumbLX, state->Gamepad.sThumbLY );
		logPrintf( "}\n" );

		//logPrintf( "%d\n", res );

		return res;
	}

	return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputSetState( _In_ DWORD userIndex, _In_ XINPUT_VIBRATION* vibe ) WIN_NOEXCEPT {
	//logPrintf( "%s: %lu\n", __FUNCTION__, userIndex );

	if ( origXInputSetState )
		return origXInputSetState( userIndex, vibe );

	return ERROR_DEVICE_NOT_CONNECTED;
}

BOOL loadXInput( void ) {
	logPrintf( "Attempting to load real xinput1_4.dll... " );
		realXInputDLL = LoadLibrary( "C:\\Windows\\System32\\xinput1_4.dll" );
	logPrintf( "%s\n", ( realXInputDLL == NULL ) ? "failed!" : "loaded!" );

	if ( realXInputDLL ) {
		origXInputGetState = ( XInputGetStateProc ) GetProcAddress( realXInputDLL, "XInputGetState" );
		origXInputSetState = ( XInputSetStateProc ) GetProcAddress( realXInputDLL, "XInputSetState" );

		logPrintf( "XInputGetState addr: %p\n", origXInputGetState );
		logPrintf( "XInputSetState addr: %p\n", origXInputSetState );

		return TRUE;
	}

	return FALSE;
}

void unloadXInput( void ) {
	if ( realXInputDLL )
		FreeLibrary( realXInputDLL );
}
