#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "logger.h"

static FILE* logFile = NULL;

void logOpen( void ) {
	logFile = fopen( "hooklog.txt", "wt+" );
}

void logClose( void ) {
	if ( logFile ) {
		fclose( logFile );
	}
}

void logPrintf( const char* fmt, ... ) {
	char buf[ 1024 ];
	int len = 0;
	va_list argp;

	va_start( argp, fmt );
		len = _vsnprintf( buf, sizeof( buf ), fmt, argp );
	va_end( argp );

	fwrite( buf, 1, len, logFile );
	fflush( logFile );
}
