#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "logger.h"

#ifdef HOOKLOG
static FILE* logFile = NULL;
#endif

void logOpen( void ) {
#ifdef HOOKLOG
	logFile = fopen( "hooklog.txt", "wt+" );
#endif
}

void logClose( void ) {
#ifdef HOOKLOG
	if ( logFile ) {
		fclose( logFile );
	}
#endif
}

void logPrintf( const char* fmt, ... ) {
#ifdef HOOKLOG
	char buf[ 1024 ];
	int len = 0;
	va_list argp;

	va_start( argp, fmt );
		len = _vsnprintf( buf, sizeof( buf ), fmt, argp );
	va_end( argp );

	fwrite( buf, 1, len, logFile );
	fflush( logFile );
#endif
}
