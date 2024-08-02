#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <ctype.h>
#include <unistd.h>

#include "error.h"
#include "argp.h"

//----------------------------------------------------------------------------- ----------------------------------------
// A "literal" is a character code which is intercepted by the TTY and interpreted as a signal. 
// Any "literal" may be prefixed with "Literal Next" (LNext) signal to signal that 
//   the following byte should NOT be interpreted as a signal, but as a literal value
//
#ifdef CTRL
#	undef   CTRL
#	define  CTRL(x)  ( (x - 0x40) & 0x7F )
#endif

typedef
	struct sig {
		char*  name;  // signal name
		int    idx;   // signal index
		char   dflt;  // current value
	}
sig_t;

sig_t  sig[] = {
	{ "intr"   , VINTR   , CTRL('C' )},  //  0 : ^C : intr    : Interrupt
	{ "quit"   , VQUIT   , CTRL('\\')},  //  1 : ^\ : quit    :
	{ "erase"  , VERASE  , CTRL('?' )},  //  2 : ^? : erase   : Erase last word
	{ "kill"   , VKILL   , CTRL('U' )},  //  3 : ^U : kill    : Kill process / Erase Line
	{ "eof"    , VEOF    , CTRL('D' )},  //  4 : ^D : eof     : Hang up
	{ "start"  , VSTART  , CTRL('Q' )},  //  8 : ^Q : start   : X-On
	{ "stop"   , VSTOP   , CTRL('S' )},  //  9 : ^S : stop    : X-Off
	{ "susp"   , VSUSP   , CTRL('Z' )},  // 10 : ^Z : susp    : Susupend process
	{ "eol"    , VEOL    , 0         },  // 11 :    : eol     :
	{ "rprnt"  , VREPRINT, CTRL('R' )},  // 12 : ^R : rprnt   : Reprint since last NL
	{ "discard", VDISCARD, CTRL('O' )},  // 13 : ^O : discard :
	{ "werase" , VWERASE , CTRL('W' )},  // 14 : ^W : werase  :
	{ "lnext"  , VLNEXT  , CTRL('V' )},  // 15 : ^V : lnext   : Literal next
#ifdef VEOL2
	{ "eol2"   , VEOL2   , 0         },  // 16 :    : eol2    :
#endif
#ifdef VSWTCH
	{ "swtch"  , VSWTCH  , 0         },  // 
#endif
	{ NULL,0,0 }
};

static  char            literal[256]   = {0};  // Allow for 256 literals
static  int             litCnt         = 0;    // How many are there
static  char            lnext          = 0;    // Value of LNext

static  int             ttyfd          = 0;    // TTY File Descriptor
static  struct termios  orig           = {0};  // Original settings
static  struct termios  attr           = {0};  // Current settings

//+============================================================================ ========================================
// Find human name of ctrl value
//
char*  ctrls (char c,  char* r)
{
	static char  s[3] = {0};

	c &= 0x7F;

	if      ((c >0x20) && (c < 0x7F))  s[0] = c  , s[1] = ' ' ;
	else if  (c == 0x00)               s[0] = ' ', s[1] = ' ' ;
	else if  (c == 0x7F)               s[0] = '^', s[1] = '?' ;
	else    /* 01..31 */               s[0] = '^', s[1] = c + 0x40 ;

	if (r)  return strcpy(r, s);  // NOT SAFE!
	return s;                     // NOT RELIABLE!
}

//+============================================================================ ========================================
// Open TTY
//
err_t  tty_open (char* tty)
{
	if ((ttyfd = open(tty, O_RDWR)) == -1) {
		INFO("! Open TTY |%s| - ", tty);
		fflush(stdout);
		perror(NULL);
		return ERR_;
	}

	// Get TTY attributes
	tcgetattr(ttyfd, &orig);
	memcpy(&attr, &orig, sizeof(attr));

	return ERR_OK;
}

//+============================================================================ ========================================
// Collate all the values bound to signals
//
err_t  tty_getSig (void)
{
	EXTRA("# Original STTY signal bindings:\n");
	for (sig_t* sp = sig;  sp->name;  sp++) {
		char  ctl = orig.c_cc[sp->idx];
		if (ctl != 0)  literal[litCnt++] = ctl ;
		EXTRA( "   %2d: %-8s [0x%02X = %s]", sp->idx, sp->name, sp->dflt, ctrls(sp->dflt, NULL));
		EXTRA( " --> 0x%02X = %s\n", ctl, ctrls(ctl, NULL));
	}

	// Append CR and LF
	EXTRA("# Add \\n and \\r to Literal set\n");
	literal[litCnt++] = '\r';
	literal[litCnt++] = '\n';

	return ERR_OK;
}

//+============================================================================ ========================================
// Set Terminal speed
//
err_t  tty_setSpeed (void)
{
	cfsetospeed(&attr, B115200); // Set output speed
	cfsetispeed(&attr, B115200); // Set input speed
	tcsetattr(ttyfd, TCSANOW, &attr);

	return ERR_OK;
}

//+============================================================================ ========================================
// Set Canonical mode
//
err_t  tty_setCanon (void)
{
	attr.c_lflag &= ~(ECHO | ECHONL | ISIG | ICANON | IEXTEN);
	attr.c_cc[VTIME] = 0;
	attr.c_cc[VMIN] = 0;
	tcsetattr(ttyfd, TCSANOW, &attr);

	return ERR_OK;
}

//+============================================================================ ========================================
// Show all the values that will be prefixed as literals
//
err_t  tty_showLit (void)
{
	if (cli.noise < nEXTRA)  return ERR_OK ;

	EXTRA("# %d literal values:", litCnt);

	for (char* cp = literal;  *cp;  cp++) {
		if (!((cp-literal)%7))  EXTRA("\n  ");
		EXTRA( "(0x%02X:%s), ", *cp, ctrls(*cp, NULL));
	}
	EXTRA("\b\b \n");

	return ERR_OK;
}

//+============================================================================ ========================================
// LNEXT - Literal Next - this is our prefix code
// If one is not allocated (eg. ^V), then allocate one
//
err_t  tty_setLnext (void)
{
	lnext = orig.c_cc[VLNEXT];

	EXTRA("# LNEXT : 0x%02X = %s\n", lnext, ctrls(lnext, NULL));
	if (!lnext) {
		char  c;
		for (c = CTRL('V');  c && strchr(literal, c);  c--) ;
		if (!c)  FERROR(ERR_, "! Cannot allocate LNEXT\n") ;

		attr.c_cc[VLNEXT] = (literal[litCnt++] = (lnext = c));
		tcsetattr(ttyfd, TCSANOW, &attr);
		EXTRA("# LNEXT assigned to : 0x%02X = %s\n", lnext, ctrls(lnext, NULL));
	}

	return ERR_OK;
}

//+============================================================================ ========================================
// Restore original terminal settings
//
err_t  tty_close (void)
{
	// Restore original terminal settings
	tcsetattr(ttyfd, TCSANOW, &orig);

	close(ttyfd);

	return ERR_OK;
}

//+============================================================================ ========================================
// Does the specified character require an LNext prefix?
//
static
int  isliteral (char ch)
{
	if (!litCnt)  return 0 ;

	char*  cp = literal;

	for (int i = 0;  i < litCnt;  i++, cp++)
		if (*cp == ch)  return 1 ;

	return 0;
}

//+============================================================================
// Send the character
//
static
err_t  tty_stuffc (char* s)
{
	if (ioctl(ttyfd, TIOCSTI, s) == -1) {
		FERROR(ERR_, "Cannot write to TTY ...Try `sudo stuff`\n");  // Fatal error - bail
		//return ERR_;
	}

	// Pause 0.0001 seconds to allow keystroke to propogate
	usleep(100);
	return ERR_OK;
}

//+============================================================================
err_t  tty_stuff (char* s,  int len,  int rat)
{
	int   err;
	char  ln  = (char)(rat & 0xFF);

	(void)lnext;

	for (int i = 0;  i < len;  i++) {
		char  ch = s[i];

		// Forced RAT, prefixes EVERY character
		if (rat >= 0) {
			if ((err = tty_stuffc(&ln)) != ERR_OK)  return err ;

		// RAT_LNEXT, uses LNEXT from the TTY
		} else if ((rat == RAT_LNEXT) && isliteral(ch)) {  
			if ((err = tty_stuffc(&lnext)) != ERR_OK)  return err ;
		}

		// Stuff the character
		if ((err = tty_stuffc(&ch)) != ERR_OK)  return err ;
	}

	return ERR_OK;
}
