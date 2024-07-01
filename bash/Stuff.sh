#!/bin/bash


RP=`realpath $0`
source ${RP%/*}/Lib.shi

VER=1.0

RAT=0
VERBOSE=0
KEEPSUDO=0
EXE=""
PID=
APPEND=0
APPCHAR='\x0a'
ECHO=0
BAN=""
BANDEC=""
CHARS=""
TTY=

#+=============================================================================
Help () {
	Silent 0
	cat <<-HELP
		! Use: $0 -x <exename> [options]
		!      $0 -p <num>     [options]
		!      $0 -t <tty>     [options]
		!
		!    -a           append \n
		!    -b  <chars>  ban characters
		!    -c  <chars>  push all chars to remote tty
		|
		!    -e           echo CLI (overrides -s)
		!    -v           verbose output (overrides -s)
		!    -s           silent (result only)
		|
		!    -r           RAT mode (use lnext)*
		!    -k           keep sudo privs on exit
		!    -@           show all lines which call sudo
		|
		! In [-s]ilent mode, you MUST specify a PID
		!
		! <chars> are bash character strings, which allows
		!    ASCII "~",  Octal "\176",  or hex "\x7E"
		!
		! The char string is/may be formatted as any combination of:
		!   0x00..0x0FF : \dDDD  (3 decimal digits)
		!   0x00..0x0FF : \OOO   (3 octal digits)
		!   0x00..0x0FF : \xHH   (2 hex digits)
		!   0x20..0x05B : A      (ASCII char)
		!      0x05C    : \\\\     (backslash)
		!   0x5D..0x07E : A      (ASCII char)
		!
		!  Eg.  0x42 ('B') -> \x42 or \d066 or \102 or B
		!       0x5C ('\') -> \x5C or \d092 or \134 or \\\\
		!  * RAT mode will prefix  a 'Signal' keys with 'lnext'
		!    'lnext' will be enabled remotely if required
	HELP
	exit $1
}

#+=============================================================================
ParseCLI() {
	CLI=("$@")

	((${#CLI})) || Help 1

	[[ $* =~ .*-s.* ]] && Silent 1

	echo "# CLI: ${CLI[@]}"

	for ((i = 0;  i < ${#CLI[@]}; i++)) ; do
		case "${CLI[$i]}" in
			-x )  ((i++)) 
			      if [[ -n ${PID} ]] ; then
			      	echo "# CLI: PID already set. EXE ignored: ${CLI[$i]}"
			      else
			      	EXE="${CLI[$i]}"
			      	echo "# CLI: EXE=|${EXE}|"
			      fi
			      ;;
			-p )  ((i++)) 
			      PID="${CLI[$i]}"
			      if [[ -n "${EXE}" ]] ; then
			      	echo "# CLI: PID overrides, EXE: ${EXE} -> ${PID}"
			      	EXE=""
			      else
			      	echo "# CLI: PID=|${PID}|"
			      fi
			      ;;
			-t )  ((i++)) 
			      TTY="${CLI[$i]}"
			      ;;

			-a )  APPEND=1
			      echo "# CLI: +append \n \x0d"
			      ;;
			-b )  ((i++)) 
			      printf -v BAN "${CLI[$i]}"
			      ;;
			-c )  ((i++)) 
			      CHARS="${CLI[$i]}"
			      ;;

			-e )  ECHO=1
			      ;;
			-v )  VERBOSE=1 && Silent 0
			      echo "# CLI: +verbose"
			      ;;
			-s )  ((!VERBOSE)) && Silent 1
			      ;;

			-r)   RAT=1
			      echo "# CLI: +RAT Mode"
			      ;;
			-k )  KEEPSUDO=1
			      echo "# CLI: +keepsudo"
			      ;;
			-@ )  set -x
			      true "---------------------------------------------------"
			      true "--- REVEAL ALL USES OF sudo ---"
			      true "---------------------------------------------------"
			      eval "sed -n '/.*[-]EOF.*/,/.*[E]OF.*/p' $0"
			      true "---------------------------------------------------"
			      grep -n sudo $0 |
			        grep -v "^[0-9]*:#" |\
			        grep -v "^[0-9]*:\s*echo"
			      true "---------------------------------------------------"
			      exit 0
			      ;;

			-h|--help )  Help 0 ;;

			* )   Silent 0
			      echo "# CLI: Bad arg: ${CLI[$i]}"
			      Help 99
			      ;;
		esac
	done
}

#+=============================================================================
PushChr() {
	local  term=$1
	local  char="$2"
	local  x y

	((VERBOSE)) && {
		printf -v x "%d" "'${char}"
		printf "+> %2X  %03o  %3d  " $x $x $x
		(( (x >= 0x80) )) && {
			printf "M-"
			((x -= 0x80))
		}
		if ((x == 0x7F)) ; then
			printf "^?\n" ${char}
		else
			if ((x < 0x20)) ; then
				((x += 0x40))
				echo -en "^"
			fi
			echo -e "\\x$(printf %02x $x)"
		fi
	}

	# Need to escape these for perl
	[[ "${char}" == "\\" ]] && char="\\\\"
	[[ "${char}" == "\"" ]] && char="\\\""
	[[ "${char}" == "\$" ]] && char="\\\$"

	sudo perl <<-EOF
		\$TIOCSTI = 0x5412;
		\$tty     = "${term}";
		\$char    = "${char}";
		open(\$fh, ">", \$tty);
		ioctl(\$fh, \$TIOCSTI, \$char);
	EOF
}

#+=========================================================
PushStr() {
	local       term=$1
	declare -n  str=$2

	local  i     # string index
	local  c     # char from str(ing)
	local  val   # decimal value of char
	local  lnxt  # value of lnext
	local  cnt   # counter
	
	((VERBOSE)) && echo "=> |$str|[${#str}]"

	cnt=0
	for ((i = 0;  i < ${#str};  i++)) ; do
		c="${str:$i:1}"

		# escaped chars
		if [[ "$c" == "\\" ]] ; then
			case "${str:$((++i)):1}" in
				\\)	val=92
					;;
				d)	val=$(( 10#${str:$((i+1)):3} ))
					((i+=3))
					;;
				x)	val=$(( 16#${str:$((i+1)):2} ))
					((i+=2))
					;;
				*)	val=$(( 8#${str:$i:3} ))
					((i+=2))
					;;
			esac
			printf -v c "\x$(printf %x ${val})"

		else
			printf -v val "%i" "'$c"
		fi

		[[ "${BANDEC}" =~ " ${val} " ]] && {
			Silent 0
			printf "! Banned character: idx=%d, byte=%d, val=0x%02X\n" $i $cnt ${val}
			return 66  # error
		}

		# signal bindings - prefix with lnext
		((RAT)) && [[ "${SIGDEC}" =~ " ${val} " ]] && {
			echo -en "\033[0;31m"  # red
			PushChr ${term} "${SIGLNEXT}"
			echo -en "\033[0m"     # normal
		}

		PushChr ${term} "$c"
		((cnt++))
	done

	((APPEND)) && PushChr ${term} ${APPCHAR}

	return 0  # success
}

#+=============================================================================
SigAdd() {
	local  sig=$1
	local  bnd=$2
	local  hex=$3
	local  oct=$4
	local  dec=$5

	SIGLST+=" ${sig} "
	SIGDEC="${SIGDEC} ${dec} "
	eval SIGARR[$sig,bnd]="$bnd"
	eval SIGARR[$sig,hex]="$hex"
	eval SIGARR[$sig,oct]="$oct"
	eval SIGARR[$sig,dec]="$dec"
}

#+=============================================================================
# We need a list of the signals for 2 reasons
# 1. We need to know what characters are "untypable"
# 2. We need to know/set lnext so we can type untypable characters
#
GetSigList() {
	local tty=$1

	declare -g -A  SIGARR
	declare -g     SIGLST=""
	declare -g     SIGDEC=""
	declare -g     SIGLNEXT=""

	# get signal bindings
	readarray -t SIGS < <(stty -F ${tty} -a | sed s'/;/\n/g' | grep = | grep -v 'min\|time\|line\|undef')
	((VERBOSE)) && echo "# Signal count: ${#SIGS[@]}"

	#stty inst '^C'    ^  = -0x40
	#stty inst 'M-^C'  M- = +128
	#stty intr '@'     char
	#stty intr \64     decimal
	#stty intr \064    octal

	for ((i = 0;  i < ${#SIGS[@]};  i++)) ; do
		s=${SIGS[$i]## }
		sig="`cut -d' ' -f1 <<<$s`"
		bnd="`cut -d' ' -f3 <<<$s`"

		# Meta key +0x80
		[[ ${bnd:0:2} == "M-" ]] && {
			meta=1
			key=${bnd:2}
		} || {
			meta=0
			key=${bnd}
		}

		# Ctrl key +/-0x40
		[[ ${key:0:1} == "^" ]] && {
			ctl=1
			key=${key:1}
		} || ctl=0

		# space will have been truncated, put it back
		[[ -z ${key} ]] && key=" "

		printf -v dec "%d" "'$key"

		((ctl)) && {
			[[ ${key} == "?" ]] && ((dec += 0x40)) || ((dec -= 0x40))
		}

		((meta)) && ((dec += 0x80))

		printf -v hex "0x%02x" "$dec"
		printf -v oct "0%03o" "$dec"

		((VERBOSE)) && echo -e "#\t$s \t|  ${sig}  \t|$bnd| \t->\t$meta + $ctl + $key \t==\t$hex, $oct, $dec"

		# add to list
		SigAdd ${sig} ${bnd} ${hex} ${oct} ${dec}

		# track lnext
		[[ ${sig} =~ lnext ]] && printf -v SIGLNEXT "\x${hex:2}"
	done

	# dump table
	((VERBOSE)) && {
		echo "# Signal  array : |${SIGLST[@]}|"
		echo "# Decimal list  : |${SIGDEC}|"
		echo "# lnext         : |${SIGLNEXT}|"
		for i in ${SIGLST[@]} ; do
			echo -e "#\t$i\t ${SIGARR[${i},bnd]}\t ${SIGARR[${i},hex]}, ${SIGARR[${i},oct]}, ${SIGARR[${i},dec]}"
		done
	}
}

#+=============================================================================
SetLnext() {
	local  tty=$1
	local  val=$2
	local  oct

	printf -v oct "\%03o" $((val))
	eval "sudo stty lnext ${oct}  -F ${tty}"

	SigAdd lnext ${bnd} ${hex} ${oct} ${dec}
}

#++============================================================================
#++============================================================================

# ----- check silent mode
[[ $* =~ .*-s.* ]] || echo "# Stuff v${VER}"
[[ $* =~ .*-e.* ]] && echo "# $0 $@"

# ----- Parse CLI
ParseCLI "$@"

[[ -z ${CHARS} ]] && {
	echo "! Nothing to send!"
	exit 250
}
 
if [[ -n $TTY ]] ; then
	[[ -c ${TTY} ]] || {
		echo "! Not a character device: ${TTY}"
		exit 234
	}
	echo "# Using TTY: ${TTY}"

else
	if [[ -z "${EXE}" && -z ${PID} ]] ; then
		echo "! Must specify -e EXE, or -p PID, or -t TTY"
		exit 48
	
	elif [[ -n "${EXE}" ]] ; then
		GetPID PID "${EXE}" || {
			echo "! Exe not running: ${EXE}"
			exit 49
		}
		EXE=""
	fi

	[[ ! -d /proc/${PID} ]] && {
		echo "! PID not found: ${PID}"
		exit 45
	}

	# ----- set PID, EXE, TTY
	l=$(echo `ps aux | grep "[^0-9]*[${PID:0:1}]${PID:1}"`)
	[[ -z $l ]] && {
		echo "! Specified PID not found: ${PID}"
		exit 87
	} || true

	EXE=`cut -d' ' -f11 <<<$l` ; EXE=${EXE##*/}
	PROCPID="/proc/${PID}"
	TTY=`readlink "${PROCPID}/fd/1"`

	echo "# Using EXE: ${EXE}"
	echo "#       PID: ${PID} -> ${PROCPID}"
	echo "#       TTY: ${TTY}"
fi



# ----- Parse BAN list
BANDEC=" "
for ((i = 0;  i < ${#BAN};  i++)) ; do
	printf -v BANDEC "%s%d " "${BANDEC}" "'${BAN:$i:1}"
done
echo "# Ban List: |${BANDEC}|"

# ------ get sudo privs
echo "# \`sudo\` privs are required to push keys to another process"
GetSudo 1 || exit $?

((RAT)) && {
	# ----- get signal list
	echo "# Acquire remote termial settings"
	GetSigList ${TTY}

	# ----- add cr/lf so they get stuffed with lnext
	SigAdd 'LF' '^J' '0x0A' '012' '10'  # name, key, hex, oct, dev
	SigAdd 'CR' '^M' '0x0D' '015' '13'

	# ----- make sure we have an 'lnext'
	[[ -z ${SIGLNEXT} ]] && {
		echo "! WARNING: 'lnext' not defined"

		# choose a ^KEY to bind to
		for ((i = 22;  i > 0;  i--)) ; do
			[[ "${SIGDEC}" =~ " $i " ]] || break
		done
		((!i)) && echo "# No available binding found" && exit 109  # this should never happen

		echo "# Set lnext to: $i (decimal)"
		SaveTTY ${TTY}
		SetLnext ${TTY} $i
	}
	echo "# lnext : \\${SIGARR[lnext,hex]:1}"
}

echo "# Send : |${CHARS}|"

# ----- go go gadget remote attack
PushStr ${TTY} CHARS
EL=$?

# ----- do we want to keep the sudo privs?
((KEEPSUDO)) && {
	echo "# ** KEEPING SUDO PRIVS **"
} || {
	echo "# Force sudo timeout"
	DropSudo
}

# ----- clean up and go home
RestoreTTY ${TTY}
exit $EL
