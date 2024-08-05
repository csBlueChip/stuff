#include  <stdint.h>
#include  <string.h>
#include  <stdio.h>

#include  "stuff.h"
#include  "error.h"
#include  "tty.h"
#include  "argp.h"

//----------------------------------------------------------------------------- ----------------------------------------
typedef
	enum chid {
		ID_NONE = 0x00,

		ID_ESC  = 0x01,
		ID_CTL  = 0x02,
		ID_PRT  = 0x04,
		ID_CMP  = 0x08,
		ID_MSK  = 0x0F,

		ID_BRT  = 0x10,
		ID_RAT  = 0x20,
		ID_BAN  = 0x40,
		ID_RST  = 0x80,
	}
chid_t;

chid_t  prevID = ID_NONE;

//----------------------------------------------------------------------------- ----------------------------------------
// Done like this in case I decide I want to allow them to be user-defined
//
static const char*  brt = "\e[0;1m" ;
static const char*  rst = "\e[0m" ;

static const char*  esc = "\e[35m";
static const char*  ctl = "\e[33m";
static const char*  prt = "\e[32m";
static const char*  cmp = "\e[36m";

static const char*  rat = "\e[44m";  // commonly ^X
static const char*  ban = "\e[0;1;33;41m";

//+============================================================================
// 3- = ink     4- = paper
//
// -0 = blk     -1 = red      -2 = grn     -3 = yel    
// -4 = blu     -5 = mag      -6 = cyn     -7 = wht
//
// 0; = reset   1; = bright   2; = dim
//
static
char*  colour (uint8_t n,  chid_t id)
{
	if (!cli.colour)   return ERR_OK ;

	if (id & ID_BRT)   printf(brt) ;
	else               printf(rst) ;

	if (id & ID_RST)   return ERR_OK ;

	switch (id & ID_MSK) {
		case ID_ESC :  printf(esc);  break ;
		case ID_CTL :  printf(ctl);  break ;
		case ID_PRT :  printf(prt);  break ;
		case ID_CMP :  printf(cmp);  break ;
		default:       break ;
	}

	if (id & ID_RAT)   printf(rat) ;
	if (id & ID_BAN)   printf(ban) ;  // banned overrides literal

	return ERR_OK;
}

//+============================================================================
static
err_t  humanise_ (uint8_t n)
{
	char    name[8] = {0};
	chid_t  id      = ID_NONE;

	if      (n == 0x09)  {  sprintf(name, "\\t");             id = ID_ESC;  }  // tab        -->|      ,
	else if (n == 0x0D)  {  sprintf(name, "\\r");             id = ID_ESC;  }  // return            <--'
	else if (n == 0x1B)  {  sprintf(name, "\\e");             id = ID_ESC;  }  // escape     [Esc]
	else if (n == 0x5C)  {  sprintf(name, "\\\\");            id = ID_ESC;  }  // backslash         '\'
	else if (n == 0x7F)  {  sprintf(name, "\\^?");            id = ID_ESC;  }  // backspace   <--
	else if (n <= 0x1F)  {  sprintf(name, "\\^%c", n +0x40);  id = ID_CTL;  }  // ^@..^_
	else if (n <= 0x7E)  {  sprintf(name, "%c",    n      );  id = ID_PRT;  }  // printable
	else {  // >= 0x80                                                         // compose
		switch (cli.human) {
			case   8 :      sprintf(name, "\\%03o" , n);      break ;
			case  10 :      sprintf(name, "\\d%03i", n);      break ;
			case  16 :      sprintf(name, "\\x%02X", n);      break ;
			case -16 :      sprintf(name, "\\x%02x", n);      break ;
			default  :      return ERR_ ;  // should never happen - sanitised by argp
		}
		id = ID_CMP;
	}

	// In octal ALL non-printables are in numeric format
	if ((cli.human == 8) && (*name == '\\')) {
		sprintf(name, "\\%03o", n);
		id = ID_CMP;
	}

	if (isliteral(n))  id |= ID_RAT ;
	if (isban(n))      id |= ID_BAN ;

	// do not toggle brightness during printable strings
	if ( ((id & ID_MSK) == ID_PRT) && ((prevID & ID_MSK) == ID_PRT) )
		id = (id & ~ID_BRT) | (prevID  & ID_BRT) ;  // copy brightness
	else 
		id = (id & ~ID_BRT) | (~prevID & ID_BRT) ;  // toggle brightness

	colour(n, id);
	printf("%s", name);

	prevID = id;
	return ERR_OK;
}

//+============================================================================
err_t  humanise (char* s,  int len)
{
	int  err = ERR_OK;

	for (int i = 0;  i < len;  i++) {
		if ( (err = humanise_(s[i])) != ERR_OK)  return err ;
	}
	colour(0, ID_RST);

	return ERR_OK;
}

