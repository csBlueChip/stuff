#ifndef  _ERROR_H_
#define  _ERROR_H_

#include  <stdio.h>
#include  <stdarg.h>

// List of error codes
typedef
	enum err {
		ERR_OK     = 0,
		ERR_BADPRM = 1,
		ERR_ = 255,
	}
err_t;

// output noise level
typedef
	enum noise {
		nERROR = 1,
		nWARN  = 2,
		nINFO  = 3,
		nEXTRA = 4,
		nDEBUG = 5,
	}
noise_t;

void  msg (int level,  int kill,  FILE* fh,  const char* fmt,  ...) ;

#define ERROR(fmt,...)        msg(nERROR, -1,  stderr, fmt, ##__VA_ARGS__)
#define FERROR(err, fmt,...)  msg(nERROR, err, stderr, fmt, ##__VA_ARGS__)
#define WARN(fmt,...)         msg(nWARN,  -1,  stdout, fmt, ##__VA_ARGS__)
#define INFO(fmt,...)         msg(nINFO,  -1,  stdout, fmt, ##__VA_ARGS__)
#define EXTRA(fmt,...)        msg(nEXTRA, -1,  stdout, fmt, ##__VA_ARGS__)
#define DEBUG(fmt,...)        msg(nDEBUG, -1,  stdout, fmt, ##__VA_ARGS__)

#endif //_ERROR_H_
