#include  <argp.h>     // argp support
#include  <stdbool.h>  // bool, true, false
#include  <stdlib.h>   // realloc, getenv
#include  <string.h>   // strdup, strspn, strcspn

#include  "argp.h"
#include  "error.h"

//-----------------------------------------------------------------------------
// argp parameters
//
cli_t  cli;

const char*  argp_program_version = 
	TOOLNAME " " VER_MAJ "." VER_MIN "." VER_SUB "(" VER_SVN ")";

const char*  argp_program_bug_address = BUGSTO;

char doc[] = 
	"\nThe contents of the environment variable " ENV_VAR 
		" will be added to the START of the command line arguments "
		"...Use -~ as the FIRST argument to disable this feature."
	"\v"
	"The following string formats are valid representations of a "
	"value/character in the range {0 .. 255}\n"
	"\tASCII Character   {' ' .. '~'}    / (0x20 .. 0x7E} (except 0x5C)\n"
	"\t\\\\                Backslash       / {0x5C}\n"
	"\t\\^<key>           {^@  .. ^_, ^?} / {0x00 .. 0x1F, 0x7F}\n"
	"\t\\d255             Three digit decimal number (000  .. 255 }\n"
	"\t\\xFF              Two digit hex number       {0x00 .. 0xFF}\n"
	"\t\\377              Three digit octal number   {000  .. 377 }\n"
	"\tC Escape Codes    \\r => 0x0D   \\t => 0x09   \\e => 0x1B  \\f => 0x0C\n"
	"\t                  \\n => 0x0A   \\b => 0x08   \\a => 0x07  \\v => 0x0B\n"


;

static char args_doc[] = "\"string\"  \"string\" ...";

// http://www.gnu.org/software/libc/manual/html_node/Argp-Option-Vectors.html
// If you can come up with a way to make this readable, you've solved a problem I couldn't!
static struct argp_option options[] = {

	{NULL,      '\0', "",     OPTION_DOC, "Target process:", 1},
	{"pid",      'p', "{0..65535}",    0, "PID of target program", 1},
	{"tty",      't', "<tty device>",  0, "eg. /dev/tty0", 1},
//	{"  Section 1 footer", '\0', "", OPTION_DOC, "", 1},

	{NULL,      '\0', "",     OPTION_DOC, "Common options:", 2},
	{"append",   'a', "<char list>",   0, "Append terminator to each string", 2},
	{"ban",      'b', "<char list>",   0, "Banned values", 2},
	{"delay",    'd', "<delay mS>",    0, "Delay between strings", 2},
	{"rat",      'r', "value",         OPTION_ARG_OPTIONAL, "RAT mode", 2},

	// Leave a blank line by incrementing the section number
	// isascii('\x81') == false - so no "short option" for 'doit' or 'secret'
//	{"doit", '\x81', NULL, 0, "Do a thing (default: don't do it)", 2},
//	{"secret", '\x80', NULL, OPTION_HIDDEN, "Secret hidden option", 2},

//	{NULL,     '\0', "",  OPTION_DOC, "ANOTHER SECTION HEADER:", 3},
//	{"format",  'f', "tPBpOiosdx", 0, "A lengthy description "
//	                                  "{target,Pkts,Bytes,port,Opt,in,out,src,dest,xtra}\n"
//	                                  "Note: Pkts, Bytes, and Opt use CAPS", 3},
//	{NULL, 'F', NULL, OPTION_ALIAS, NULL, 3},

	// Section #99, leaves gaps for future sections {4..98}
	{NULL,     '\0', "",  OPTION_DOC, "Output detail:", 99},
	{NULL,     '~',  NULL, 0, "Exclude environment variable |"ENV_VAR"|\n"
	                          "This MUST be the FIRST argument", 99},
	{"verbose", 'v', NULL, 0, "Display verbose commentary (-vvv)", 99},
	{"quiet",   'q', NULL, 0, "Do not display a commentary (-qq)", 99},
#if CLI_NEGATION
	{NULL,      '!', NULL, OPTION_HIDDEN, "Negate next argument", 99},
#endif

	// A header for the system options
	{NULL, '\0', "", OPTION_DOC, "System options:", -1},

	{NULL}
};

//+============================================================================
// Parse CLI options
//
static
error_t  cli_parse (int key,  char *arg,  struct argp_state *state)
{
#if CLI_NEGATION
	static int  neg = 0;  // ! (or -!) triggers a logical negation of the next arg
#endif

	// http://www.gnu.org/software/libc/manual/html_node/Argp-Parsing-State.html
	// state->input is a pointer to our CLI argument structure
	cli_t*  pCli = state->input;
	bool    special;

	// Process ARGP special cases
	special = true;  // assume special case
	switch (key) {
		// http://www.gnu.org/software/libc/manual/html_node/Argp-Special-Keys.html
		case ARGP_KEY_INIT:  // Inititialise CLI variables
			neg = 0;

			pCli->tty    = NULL;
			pCli->pid    = 0;

			pCli->app    = NULL;
			pCli->ban    = NULL;
			pCli->delay  = 0;

			pCli->rat    = RAT_OFF;

//			pCli->noise  = nWARN;  // set in main()

			pCli->pos    = NULL;
			pCli->posCnt = 0;
			break;

		case ARGP_KEY_FINI:  // Parser has completed
			break;

		case ARGP_KEY_ARG: // arg with no "key" (Eg. input file)
			DEBUG("# KeyArg[%d]=|%s|\n", state->arg_num, arg);
			pCli->posCnt = state->arg_num + 1;
			pCli->pos    = realloc(pCli->pos, pCli->posCnt * sizeof(*pCli->pos));
			pCli->pos[state->arg_num] = arg;
			break;

		case ARGP_KEY_NO_ARGS:  // No non-option arguments
			// We demand a string of char to push
			ERROR("Nothing to stuff.\n");
			argp_usage(state);   // This will exit()

		case ARGP_KEY_END:  // All arguments have been parsed
			break;

		case ARGP_KEY_SUCCESS:
			break;

		default:
			special = false;
			break;
	}
	if (special)  goto done ;

	// We have chosen to use only key values <= 0xFF
	key &= 0xFF;

	// Debug - track our progress
	DEBUG("arg%c: ", neg?'~':' ');
	if (isascii(key))  DEBUG("-%c", key) ;
	else               DEBUG("-\\x%02X", key) ;
	DEBUG(" => |%s| ... |%s|\n", arg, state->argv[state->next]);

	// Process program specific switches
	switch (key) {
		case '~':    // -~ Ignore environment settings
			FERROR(ERR_, "-~ MUST be the FIRST argument.\n");

#if CLI_NEGATION
		case '!':    // -~ Negate next option
			if (neg)  FERROR(ERR_, "Stacked negation\n") ;
			neg = 2 ;
			break;
#endif

		case 't':    // --tty -t  
			if (pCli->pid) {
				FERROR(ERR_, "--tty : PID already specified\n");
				exit(1);
			}
			pCli->tty = arg;
			break;

		case 'p':    // --pid -p
			if (pCli->tty)  FERROR(ERR_, "--pid : TTY already specified\n") ;
			pCli->pid = atoi(arg);
			break;

		case 'a':    // --append -a
			pCli->app = arg;
			break;

		case 'r':    // --rat -r
			pCli->rat = arg ? atoi(arg) : RAT_LNEXT ;
			if (pCli->rat > 255)  FERROR(ERR_, "--rat : range is {0..255}\n") ; 
			break;

		case 'b':    // --ban -b
			pCli->ban = arg;
			break;

		case 'd':    // --delay -d
			pCli->delay = atoi(arg);
			break;

//		// Bug in gcc refuses to recognise '\x81' !
//		case 0x81:  // --doit
//			pCli->doit = true;
//			break;


		case 'v':     // --verbose -v
			if (pCli->noise < 10)  pCli->noise++ ;
			break;

		case 'q':     // --quiet -q
			if (pCli->noise >  0)  pCli->noise-- ;
			break;

		default:
			return ARGP_ERR_UNKNOWN;
    }

done:
#if CLI_NEGATION
	if (neg)  neg-- ;
#endif
	return 0;
}

//+============================================================================
// Add an environment variable (if any) before argv, and update argc.
// Return the expanded environment variable to be freed later, or NULL
// if no options were added to argv.
//
// add_envopt(&argc, &argv, const char* env)
// On calling, env should be the name of the environment variable to be found
//
static
char*  cli_prefix (int* argcp,  char*** argvp,  const char* pre)
{
	char*   chp   = NULL;    // running pointer through prefix variable
	char**  oargv = NULL;    // runs through old argv array
	char**  nargv = NULL;    // runs through new argv array
	int     oargc = *argcp;  // old argc
	int     nargc = 0;       // number of arguments in prefix
	char*   copy  = NULL;    // copy of the env var
	char*   evsep = " \t";   // parameter separators

	// Sanity checks
	if (!pre || !*pre)  return NULL ;

	// Take a copy of the prefix
	if ( !(copy = strdup(pre)) )  return NULL ;

	// Break the copy in to parameters
	for (chp = copy;  *chp;  nargc++) {
		chp += strspn(chp, evsep);   // skip leading separators
		if (!*chp)  break ;          // end of var check

		chp += strcspn(chp, evsep);  // find end of pram
		if (*chp)  *chp++ = '\0' ;   // mark it
	}

	// No prams?
	if (nargc == 0) {
		FREE(copy);
		return NULL;
	}

	// increase argc
	*argcp += nargc;

	// Allocate the new argv array, with an extra element just in case
	//   the original arg list did not end with a NULL.
	if ( !(nargv = (char**)calloc(*argcp + 1, sizeof(*argvp))) )  return NULL ;
	oargv  = *argvp;
	*argvp = nargv;

	// Copy the program name first
	if (oargc-- < 0)  return NULL ;  // quit("ARGC<=0  eh?");
	*(nargv++) = *(oargv++);

	// Then insert the extra args
	for (chp = copy;  nargc > 0;  nargc--) {
		chp += strspn(chp, evsep);  // skip separators
		*(nargv++) = chp;           // store start
		while (*chp++) ;            // skip over word
	}

	// Finally copy the old args and add a NULL (usual convention)
	while (oargc--)  *(nargv++) = *(oargv++) ;
	*nargv = NULL;

	return copy;  // The caller will need to free() this
}

//+============================================================================
static
void  cli_dump (cli_t*  pCli)
{
	int  i;

	DEBUG("# CLI arguments:\n");
	DEBUG("  pid   =|%d|\n", cli.pid);
	DEBUG("  tty   =|%s|\n", cli.tty);

	DEBUG("  app   =|%s|\n", cli.app);
	DEBUG("  ban   =|%s|\n", cli.ban);

	DEBUG("  rat   =|0x%02X|\n", cli.rat);

	DEBUG("  noise =|%d|\n", cli.noise);
	for (i = 0;  i < cli.posCnt;  i++)
		DEBUG("  pos[%d]=|%s|[%lu]\n", i, cli.pos[i], strlen(cli.pos[i]));

	return;
}

//+============================================================================
// Replace all ! with -!
//
static
void  cli_negflg (int argc,  char** argv)
{
	int  i;

	for (i = 1;  i < argc;  i++)
		if (STREQ(argv[i], "!"))  argv[i] = "-!" ;

	return;
}

//+============================================================================
static
void  cli_cleanup (void)
{
	FREE(cli.pos);

	FREE(cli.pre);

	return;
}

//+============================================================================
int  parseCLI (int argc,  char** argv,  char** envv)
{
	int          i;

	struct argp  argp   = {options, cli_parse, args_doc, doc};
	int          badopt = 0;

	// http://www.gnu.org/software/libc/manual/html_node/Argp-Parsing-State.html
	unsigned     flags  = 0;

	// Destructor
	atexit(cli_cleanup);

#	if ARGP_DEBUG == 1
		cli.noise = nDEBUG;
#	else
		cli.noise = nINFO;
#	endif

	// CLI before environment padding
	DEBUG("# User Input: |%s", argv[0]);
	for (i = 1;  i < argc;  i++)  DEBUG("|%s", argv[i]) ;
	DEBUG("|\n");

	// Unless disabled, Add the env var prams IN FRONT OF the argv[] prams
	//   ...This allows command line override of env var settings
	// Remember to free(cli.pre) before exiting
	if ( (argc == 1) || (!STREQ(argv[1], "-~")) ) {  // Don't EXclude env var
		cli.pre = cli_prefix(&argc, &argv, getenv(ENV_VAR));
		DEBUG("# Enviroment: |%s|=|%s|\n",
		      ENV_VAR, cli.pre ? getenv(ENV_VAR) : " -not found- ");

	} else {
		// Strip off the (prefixing) -~
		for (i = 1;  i < argc;  i++)  argv[i] = argv[i+1] ;
		argc--;
	}

	// Preprocess for logical negations
	cli_negflg(argc, argv);

	// CLI after environment padding
	DEBUG("# Command arguments: |%s", argv[0]);
	for (i = 1;  i < argc;  i++)  DEBUG("|%s", argv[i]) ;
	DEBUG("|\n");

	// Parse CLI (&cli is not actually /required/ as it is global)
	DEBUG("-----PARSE-----\n");
	argp_parse(&argp, argc, argv, flags, &badopt, &cli);

	// Dump the results
	DEBUG("-----DUMP-----\n");
	cli_dump(&cli);

	return 0;
}
