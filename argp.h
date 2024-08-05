#ifndef  _ARGP_H_
#define  _ARGP_H_

#include  <stdbool.h>
#include  <stdint.h>

#include  "ver.h"

//-----------------------------------------------------------------------------
#define  ARGP_DEBUG   0  // 1 = enable

//-----------------------------------------------------------------------------
#define  CLI_NEGATION 1  // 1 = enable

//-----------------------------------------------------------------------------
// The content of an environment variable can be forced on the front of 
// the command line - by PREpending the environment variable, anything
// set in the variable can be overridden on the command line
#define ENV_VAR  TOOLNAME"_CLI"

//-----------------------------------------------------------------------------
#define STREQ(s1,s2)  (strcmp(s1,s2)==0)
#define FREE(p)       do{ free(p);  p = NULL; }while(0)

//-----------------------------------------------------------------------------
#define BUGSTO "<csbluechip@gmail.com>"

//-----------------------------------------------------------------------------
// CLI switches
//

typedef  uint32_t  uint22_t ;

typedef
	struct cli {
		char*     pre;     // tokenised contents of the CLI prefixor

		uint22_t  pid;     // --pid -p
		char*     tty;     // --tty -t

		int       human;   // --humanise -h
		int       colour;  // --colour -c

		char*     app;     // --append -a
		int       appLen; 

		char*     ban;     // --ban    -b
		int       banLen; 

		int       rat;     // --rat    -r
		int       delay;   // --delay  -d  (mS)

		int       noise;   // --verbose -v noise++ ; --quiet -q noise--
		char**    pos;     // positional args
		int       posCnt;  // positional cnt
	}
cli_t;

extern  cli_t  cli;

enum rat {
	RAT_OFF = -2,
	RAT_LNEXT = -1,
};

//-----------------------------------------------------------------------------
int  parseCLI (int argc,  char** argv,  char** envv) ;

#endif //_ARGP_H_
