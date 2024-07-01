Purpose
=======

Push keystrokes in to stdin of another process or tty.


Build
=====

To build from source, type: `make`


Usagee
======

stuff --help

Usage: stuff [OPTION...] "char string"

The contents of the environment variable stuff_CLI will be added to the START
of the command line arguments ...Use -~ as the FIRST argument to disable this
feature.

 Target process:
  -p, --pid={0..65535}       PID of target program
  -t, --tty=<tty device>     eg. /dev/tty0

 Common options:
  -a, --append=<char list>   Append terminator to each string
  -b, --ban=<char list>      Banned values
  -d, --delay=<delay mS>     Delay between strings
  -r, --rat[=value]          RAT mode

 Output detail:
  -q, --quiet                Do not display a commentary (-qq)
  -v, --verbose              Display verbose commentary (-vvv)
  -~                         Exclude environment variable |stuff_CLI|
                             This MUST be the FIRST argument

 System options:
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Notes
=====

You MUST specify EITHER a PID (--pid, -p) *OR* a TTY (--tty, -t)

Charcaters may be specified in any mix of any of the following formats:

	Printable ASCII may be specified by it's common character
	With the exception of '\', which is used to 'escape' non-printable values.
		' ' ==> 0x20
		    ::   :
		'[' ==> 0x5B
		
		']' ==> 0x5D
		    ::   :
		'~' ==> 0x7E

	Backslash may be represented as:
		\\   \d092   \x5C   \134

	Control codes may be specified \^* ...where '*' the control key
		  \^?    ==> 0x7F
		  \^@    ==> 0x00
		\^A \^a  ==> 0x01
		         ::   :
		\^Z \^z  ==> 0x1A
		         ::   :
		  \^_    ==> 0x1F

	Any value may be specified in decimal, hex, or octal
		\d000   \x00   \000
		  :      :      :
		\d255   \xFF   \377

You MUST specify one or more key strings to be sent

You MAY specify a string to be appended to the end of every key string
	This will NOT be scanned for "banned" characters!

You MAY specify a string of banned characters
	If a banned character is detected in one of the key strings,
		the program will abort.
	Banning is NOT applied to the append string.

You MAY set an environment variable 'stuff_CLI'.
	Its contents are PREpended to the CLI parameters.
	You can disable this feature by placing '-~' as the FIRST CLI option

You MAY increase or decrease the amount of noise the program makes.
	The levels are OFF, ERRORS, WARNINGS, INFORMATION, EXTRA INFORMATION, DEBUG
	You start on "INFORMATION" and may make the program
		quieter with --quiet   or -q
		noisier with --verbose or -v
	You may use each switch multiple times.
		EG. '-qqq' will disable all stdout AND all stderr output

RAT Mode
========

stuff -r      stuff --rat       Use/Enable+use LNEXT where required
stuff -rDDD   stuff --rat=DDD   Use DDD (decimal value) before EVERY character

Keys such as {^C, ^Z, ^Q, ^S, etc.} are often bound to system signals.

Stuff will retrieve the bindings on the remote TTY.

One binding is LNEXT [Literal Next], which may be used to prefix a bound key. 
This is typically ^V
So to type a literal ^C (ie. without terminating the program) type:  ^V ^C
To type a literal ^V, type:  ^V ^V

-r   --rat    will automatically prefix bound keys with LNEXT

-r31 --rat=31 will forcibly inject a characer (decimal-31) before EVERY 
              character sent. ...This (currently) only works with decimal values.

If you do NOT specify --rat (or -r) your string will be sent exactly as specified.

Examples
========

stuff --pid `pgrep taget` --ban '\x0a' --append '\x0a' "myPassword"

stuff --pid `pgrep taget` --ban '\x0a' --append '\x0a' "\^@\x48\d068\117\\*~\^?\x7F\x03"

		\^@   \x48  \d068  \117  \\  *  ~  \^?   \x7F  \x03"
		0x00  'H'   'D'    'O'   \   *  ~  0x7F  0x7F   ^C "

stuff --pid `pgrep taget` --ban '\x0a' --append '\x0a' --rat "\^S^TABC^U^V"
		--> ^V^S   ^T   A   B   C   ^V^U   ^V^V

stuff --pid `pgrep taget` --ban '\x0a' --append '\x0a' --rat=42 "\^S^TABC^U^V"
		--> *^S   *^T   *A   *B   *C   *^U   *^V

stuff --pid `pgrep taget` --append '\x0d\x0a' -d 1000 "line 1" "line 2" "line 3"
