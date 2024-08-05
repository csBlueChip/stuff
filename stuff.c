#include  <stdlib.h>    // exit
#include  <stdio.h>     // printf
#include  <stdint.h>    // uint
#include  <string.h>    // memcpy
#include  <sys/stat.h>  // stat
#include  <unistd.h>    // readlink
#include  <ctype.h>     // toupper
#include  <unistd.h>    // getpid

#include  "argp.h"
#include  "error.h"
#include  "tty.h"
#include  "humanise.h"

//+============================================================================ ========================================
static
err_t  pid2tty (void)
{
	struct stat  sb;        // stat block
	char         fn[32];    // Filename eg. "/proc/<pid>/..."

	static char  link[32];  // readlink result
	int          len;       // length of link

	// no tty, no pid, but we are humanising (not stuffing)
	if ((cli.pid == 0) && cli.human) {
		// if we are humanising, and requested RAT (LNext) consideration, use local tty settings
		if (cli.rat > RAT_OFF)
			WARN("! Using local terminal signal settings!  pid=%d\n", (cli.pid = getpid()));
		else
			return ERR_OK;
	}

	// Find /proc/<pid>
	snprintf(fn, sizeof(fn), "/proc/%d",  cli.pid);
	if ( (stat(fn, &sb) != 0) || (S_ISDIR(sb.st_mode) == 0) ) {
		ERROR("PID not found: %d\n", cli.pid);
		return ERR_;
	}

	// Find tty attached to stdin
	snprintf(fn, sizeof(fn), "/proc/%d/fd/0",  cli.pid);
	if ((len = readlink(fn, link, sizeof(link)-1)) == -1) {
		ERROR("Cannot locate stdin for PID: %d\n", cli.pid);
		return ERR_;
	}
	link[len] = '\0';  // sanity

	// Put tty name in cli.tty
	cli.tty = link;
	INFO("# PID %d, TTY is: |%s|\n", cli.pid, cli.tty);

	return ERR_OK;
}

//+============================================================================ ========================================
// Check if value is in the Banned list
//
int  isban (char ch)
{
	if (!cli.banLen)  return 0 ;  // no banned list

	char*  cp = cli.ban;

	for (int i = 0;  i < cli.banLen;  i++, cp++)
		if (*cp == ch)  return 1 ;

	return 0;
}

//+============================================================================ ========================================
#define isodigit(c) ((c >= '0') && (c <= '7'))
#define isbdigit(c) ((c >= '0') && (c <= '1'))

static
int  xlat (char* id,  char* s,  int* len,  int ban)
{
	char*  cp  = NULL;
	int    pos = 0;

	*len = 0;
	if (!s)  return ERR_OK ;
	if (!strlen(s))  goto parsed ;

	for (int pass = 1;  pass <= 2;  pass++) {
		for (cp = s;  *cp;  cp++) {
			int  val = 0;
			pos = cp-s+1;
			if (*cp == '\\') {
				switch (tolower(cp[1])) {
					case '\\' :  val = '\\';  cp += 1;  break ;  // backslash
					case 'a'  :  val = '\a';  cp += 1;  break ;  // 0x07 - BEL - alarm
					case 'b'  :  val = '\b';  cp += 1;  break ;  // 0x08 - BS  - backspace
					case 't'  :  val = '\t';  cp += 1;  break ;  // 0x09 - TAB - <tab>
					case 'n'  :  val = '\n';  cp += 1;  break ;  // 0x0A - LF  - Line Feed
					case 'v'  :  val = '\v';  cp += 1;  break ;  // 0x0B - VT  - Vertical Tab
					case 'f'  :  val = '\f';  cp += 1;  break ;  // 0x0C - FF  - Form Feed
					case 'r'  :  val = '\r';  cp += 1;  break ;  // 0x0D - CR  - Carriage Return
					case 'e'  :  val = '\e';  cp += 1;  break ;  // 0x1B - ESC - Escape

					case '^'  :  // ^?
						char ch = toupper(cp[2]);
						if ((ch < '?') || (ch > '_'))  goto error ;
						val = (ch -0x40) &0x7F;
						cp += 2;
						break;

					case 'd'  :  // \d255
						for (int i = 2;  i <= 4;  i++)
							if (!isdigit(cp[i]))  goto error ;
							else  val = (val *10) +(cp[i] -'0') ;
						if (val > 255)  goto error ;
						cp += 4;
						break ;

					case 'x'  :  // \xFF
						for (int i = 2;  i <= 3;  i++)
							if (!isxdigit(cp[i]))  goto error ;
							else  val = (val *16) +( (cp[i] > '9') ? (toupper(cp[i]) -'A' +10) : (cp[i] -'0') ) ;
						cp += 3;
						break;

					default   :  // \377
						for (int i = 1;  i <= 3;  i++)
							if (!isodigit(cp[i]))  goto error ;
							else  val = (val *8) +(cp[i] -'0') ;
						if (val > 0377)  goto error ;
						cp += 3;
						break;
				}

			} else  val = *cp ;

			if (!cli.human && ban && isban(val))  goto banned ;

			if (pass == 2) {
				s[*len] = val & 0xFF;
				*len = *len +1;
			}
		}
	}

parsed:
	if (cli.noise >= nINFO) {
		INFO("# %s: ", id);

		if (!*len) {
			INFO("-none-\n");

		} else {
			INFO("{");
			for (int i = 0;  i < *len;  i++)  INFO("%02X, ", (uint8_t)s[i]) ;
			INFO("\b\b}[%d]\n", *len);
		}
	}

	return ERR_OK;

error:
	ERROR("%s : Invalid char @%ld\n  \"%s\"\n  %*.s^--here\n", id, pos, s, pos,"");
	return ERR_;

banned:
	ERROR("%s : Banned char @%ld\n  \"%s\"\n  %*.s^--here\n", id, pos, s, pos,"");
	return ERR_;

}

//+============================================================================ ========================================
int*  posLen = NULL;

static
void  cleanup (void)
{
	FREE(posLen);
	tty_close();
}

//+============================================================================ ========================================
// Parse all character strings to hex
//
static
err_t  hexify (void)
{
	err_t  err;

	if ( (err = xlat("Append", cli.app, &cli.appLen, 0)) != ERR_OK)  return err ;

	if ( (err = xlat("Ban   ", cli.ban, &cli.banLen, 0)) != ERR_OK)  return err ;

	// Some memory in which to store an array of all the string [Positional Arugment] lengths
	if ( !(posLen = calloc(cli.posCnt, sizeof(int))) )  {
		ERROR("! malloc() fail\n");
		return ERR_;
	}

	for (int i = 0;  i < cli.posCnt;  i++) {
		char  id[24];
		snprintf(id, sizeof(id), "string[%d]", i);
		if ( (err = xlat(id, cli.pos[i], &posLen[i], 1)) != ERR_OK)  return err ;
	}

	return ERR_OK;
}

//++=========================================================================== ========================================
int  main (int argc,  char* argv[],  char* envp[])
{
	err_t  err;

	atexit(cleanup);

	parseCLI(argc, argv, envp);

	WARN("# %s", TOOLNAME " v" VER_MAJ "." VER_MIN "." VER_SUB "(" VER_SVN ")");
	WARN(" ... (C) copyright csBlueChip, 2024\n");

	// PID -> TTY
	if (!cli.tty && ((err = pid2tty()) != ERR_OK))  return err ;

	// If we are humansing and *not* RAT'ing, we can skip this bit
	if (cli.tty) {	
		// Open TTY
		if ( (err = tty_open(cli.tty)) != ERR_OK)  return err ;

		// RAT mode will sniff out the TTY key bindings are prefix them with LNEXT
		if (cli.rat > RAT_OFF) {
			if ( (err = tty_getSig()     ) != ERR_OK)  return err ;
		if ( (err = tty_setLnext()   ) != ERR_OK)  return err ;
			if ( (err = tty_showLit()    ) != ERR_OK)  return err ;
		} else {
			EXTRA("# RAT mode DISabled\n");
		}

//		if ( (err = tty_setSpeed()   ) != ERR_OK)  return err ;
//		if ( (err = tty_setCanon()   ) != ERR_OK)  return err ;
	}

	// Parse all character strings to hex
	if ( (err = hexify()) != ERR_OK)  return err ;

	// Send the strings
	for (int i = 0;  i < cli.posCnt;  i++) {
		if (cli.human) {
			WARN("string[%d] : ", i);
			if ( (err = humanise(cli.pos[i], posLen[i])) != ERR_OK)  return err ;
			printf("\n");
		} else { // Stuff it!
			if ( (err = tty_stuff(cli.pos[i], posLen[i], cli.rat)) != ERR_OK)  return err ;
			if (cli.app)
				if ( (err = tty_stuff(cli.app, cli.appLen, -2)) != ERR_OK)  return err ;
			usleep(cli.delay *1000);
		}
	}

	return ERR_OK;
}























