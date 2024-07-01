#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "error.h"
#include "argp.h"

void  msg (int level,  int kill,  FILE* fh,  const char* fmt,  ...)
{
	if (level > cli.noise) {
		if (kill >= 0)  exit(kill) ;
		else            return ;
	}

	va_list  ap;

	if (level == nERROR)  fprintf(fh, "! Error %d : ", kill) ;

	va_start(ap, fmt);
	vfprintf(fh, fmt, ap);
	va_end(ap);

	fflush(fh);

	if (kill >= 0)  exit(kill) ;
}
