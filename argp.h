#include  <stdbool.h>
#include  <stdint.h>

//-----------------------------------------------------------------------------
#define  ARGP_DEBUG  0  // 1 = enable

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
// Program name & version
//
// 1.2.0  Add C Escape Codes
// 1.1.0  Increase size of PID integer to 22 bits (yes, twenty two)
// 1.0.2  Add formatting to --help
// 1.0.1  Add `make install`
//
#define TOOLNAME  "stuff"
#define VER_MAJ   "1"
#define VER_MIN   "2"
#define VER_SUB   "0"
#define VER_SVN   "0"


//-----------------------------------------------------------------------------
// CLI switches
//

typedef  uint32_t  uint22_t ;

typedef
	struct cli {
		char*     pre;     // tokenised contents of the CLI prefixor

		uint22_t  pid;     // --pid -p
		char*     tty;     // --tty -t

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
