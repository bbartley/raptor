/**************************************************************
 * Original:
 * Patrick Powell Tue Apr 11 09:48:21 PDT 1995
 * A bombproof version of doprnt (dopr) included.
 * Sigh.  This sort of thing is always nasty do deal with.  Note that
 * the version here does not include floating point...
 *
 * snprintf() is used instead of sprintf() as it does limit checks
 * for string length.  This covers a nasty loophole.
 *
 * The other functions are there to prevent NULL pointers from
 * causing nast effects.
 *
 * More Recently:
 *  Brandon Long <blong@fiction.net> 9/15/96 for mutt 0.43
 *  This was ugly.  It is still ugly.  I opted out of floating point
 *  numbers, but the formatter understands just about everything
 *  from the normal C string format, at least as far as I can tell from
 *  the Solaris 2.5 printf(3S) man page.
 *
 *  Brandon Long <blong@fiction.net> 10/22/97 for mutt 0.87.1
 *    Ok, added some minimal floating point support, which means this
 *    probably requires libm on most operating systems.  Don't yet
 *    support the exponent (e,E) and sigfig (g,G).  Also, fmtint()
 *    was pretty badly broken, it just wasn't being exercised in ways
 *    which showed it, so that's been fixed.  Also, formated the code
 *    to mutt conventions, and removed dead code left over from the
 *    original.  Also, there is now a builtin-test, just compile with:
 *           gcc -DTEST_SNPRINTF -o snprintf snprintf.c -lm
 *    and run snprintf for results.
 * 
 *  Thomas Roessler <roessler@does-not-exist.org> 01/27/98 for mutt 0.89i
 *    The PGP code was using unsigned hexadecimal formats. 
 *    Unfortunately, unsigned formats simply didn't work.
 *
 *  Michael Elkins <me@mutt.org> 03/05/98 for mutt 0.90.8
 *    The original code assumed that both snprintf() and vsnprintf() were
 *    missing.  Some systems only have snprintf() but not vsnprintf(), so
 *    the code is now broken down under HAVE_SNPRINTF and HAVE_VSNPRINTF.
 *
 *  Holger Weiss <holger@zedat.fu-berlin.de> 07/23/06 for mutt 1.5.13
 *    A C99 compliant [v]snprintf() returns the number of characters that
 *    would have been written to a sufficiently sized buffer (excluding
 *    the '\0').  Mutt now relies on this behaviour, but the original
 *    code simply returned the length of the resulting output string, so
 *    that's been fixed.
 *
 **************************************************************/

#ifdef HAVE_CONFIG_H
#include <raptor_config.h>
#endif

#include "raptor.h"
#include "raptor_internal.h"

#if !defined(HAVE_SNPRINTF) || !defined(HAVE_VSNPRINTF)

#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <stdarg.h>
#include <float.h>

#define __USE_ISOC99
#include <math.h>

#if 0
int snprintf(char *str, size_t count, const char *fmt, ...);
int vsnprintf(char *str, size_t count, const char *fmt, va_list arg);
#endif

/*
static int dopr(char *buffer, size_t maxlen, const char *format, 
                va_list args);
static void fmtstr(char *buffer, size_t *currlen, size_t maxlen,
		  char *value, int flags, int min, int max);
static void fmtint(char *buffer, size_t *currlen, size_t maxlen,
		  long value, int base, int min, int max, int flags);
const char* raptor_format_float(char *buffer, size_t *currlen, size_t maxlen,
      double fvalue, int min, int max, int flags);
*/
#if 0
static void dopr_outch(char *buffer, size_t *currlen, size_t maxlen, char c );
#endif

/*
 * dopr(): poor man's version of doprintf
 */

/* format read states */
#define DP_S_DEFAULT 0
#define DP_S_FLAGS   1
#define DP_S_MIN     2
#define DP_S_DOT     3
#define DP_S_MAX     4
#define DP_S_MOD     5
#define DP_S_CONV    6
#define DP_S_DONE    7

/* format flags - Bits */
#define DP_F_MINUS 	(1 << 0)
#define DP_F_PLUS  	(1 << 1)
#define DP_F_SPACE 	(1 << 2)
#define DP_F_NUM   	(1 << 3)
#define DP_F_ZERO  	(1 << 4)
#define DP_F_UP    	(1 << 5)
#define DP_F_UNSIGNED 	(1 << 6)

/* Conversion Flags */
#define DP_C_SHORT   1
#define DP_C_LONG    2
#define DP_C_LDOUBLE 3

#define char_to_int(p) (p - '0')
#undef MAX
#define MAX(p,q) ((p >= q) ? p : q)

#if 0
static int
dopr(char *buffer, size_t maxlen, const char *format, va_list args)
{
  char ch;
  long value;
  long double fvalue;
  char *strvalue;
  int min;
  int max;
  int state;
  int flags;
  int cflags;
  size_t currlen;
  
  state = DP_S_DEFAULT;
  currlen = flags = cflags = min = 0;
  max = -1;
  ch = *format++;

  while (state != DP_S_DONE)
  {
    if(ch == '\0')
      state = DP_S_DONE;

    switch(state) 
    {
    case DP_S_DEFAULT:
      if(ch == '%') 
	state = DP_S_FLAGS;
      else 
	dopr_outch (buffer, &currlen, maxlen, ch);
      ch = *format++;
      break;
    case DP_S_FLAGS:
      switch (ch) 
      {
      case '-':
	flags |= DP_F_MINUS;
        ch = *format++;
	break;
      case '+':
	flags |= DP_F_PLUS;
        ch = *format++;
	break;
      case ' ':
	flags |= DP_F_SPACE;
        ch = *format++;
	break;
      case '#':
	flags |= DP_F_NUM;
        ch = *format++;
	break;
      case '0':
	flags |= DP_F_ZERO;
        ch = *format++;
	break;
      default:
	state = DP_S_MIN;
	break;
      }
      break;
    case DP_S_MIN:
      if(isdigit((unsigned char)ch)) 
      {
	min = 10*min + char_to_int (ch);
	ch = *format++;
      } 
      else if(ch == '*') 
      {
	min = va_arg (args, int);
	ch = *format++;
	state = DP_S_DOT;
      } 
      else 
	state = DP_S_DOT;
      break;
    case DP_S_DOT:
      if(ch == '.') 
      {
	state = DP_S_MAX;
	ch = *format++;
      } 
      else 
	state = DP_S_MOD;
      break;
    case DP_S_MAX:
      if(isdigit((unsigned char)ch)) 
      {
	if(max < 0)
	  max = 0;
	max = 10*max + char_to_int (ch);
	ch = *format++;
      } 
      else if(ch == '*') 
      {
	max = va_arg (args, int);
	ch = *format++;
	state = DP_S_MOD;
      } 
      else 
	state = DP_S_MOD;
      break;
    case DP_S_MOD:
      /* Currently, we don't support Long Long, bummer */
      switch (ch) 
      {
      case 'h':
	cflags = DP_C_SHORT;
	ch = *format++;
	break;
      case 'l':
	cflags = DP_C_LONG;
	ch = *format++;
	break;
      case 'L':
	cflags = DP_C_LDOUBLE;
	ch = *format++;
	break;
      default:
	break;
      }
      state = DP_S_CONV;
      break;
    case DP_S_CONV:
      switch (ch) 
      {
      case 'd':
      case 'i':
	if(cflags == DP_C_SHORT) 
	  value = va_arg (args, int);
	else if(cflags == DP_C_LONG)
	  value = va_arg (args, long int);
	else
	  value = va_arg (args, int);
	fmtint (buffer, &currlen, maxlen, value, 10, min, max, flags);
	break;
      case 'o':
	flags |= DP_F_UNSIGNED;
	if(cflags == DP_C_SHORT)
	  value = va_arg (args, unsigned int);
	else if(cflags == DP_C_LONG)
	  value = va_arg (args, unsigned long int);
	else
	  value = va_arg (args, unsigned int);
	fmtint (buffer, &currlen, maxlen, value, 8, min, max, flags);
	break;
      case 'u':
	flags |= DP_F_UNSIGNED;
	if(cflags == DP_C_SHORT)
	  value = va_arg (args, unsigned int);
	else if(cflags == DP_C_LONG)
	  value = va_arg (args, unsigned long int);
	else
	  value = va_arg (args, unsigned int);
	fmtint (buffer, &currlen, maxlen, value, 10, min, max, flags);
	break;
      case 'X':
	flags |= DP_F_UP;
      case 'x':
	flags |= DP_F_UNSIGNED;
	if(cflags == DP_C_SHORT)
	  value = va_arg (args, unsigned int);
	else if(cflags == DP_C_LONG)
	  value = va_arg (args, unsigned long int);
	else
	  value = va_arg (args, unsigned int);
	fmtint (buffer, &currlen, maxlen, value, 16, min, max, flags);
	break;
      case 'f':
	if(cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, long double);
	else
	  fvalue = va_arg (args, double);
	/* um, floating point? */
	fmtfp (buffer, &currlen, maxlen, fvalue, min, max, flags);
	break;
      case 'E':
	flags |= DP_F_UP;
      case 'e':
	if(cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, long double);
	else
	  fvalue = va_arg (args, double);
	break;
      case 'G':
	flags |= DP_F_UP;
      case 'g':
	if(cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, long double);
	else
	  fvalue = va_arg (args, double);
	break;
      case 'c':
	dopr_outch (buffer, &currlen, maxlen, va_arg (args, int));
	break;
      case 's':
	strvalue = va_arg (args, char *);
	fmtstr (buffer, &currlen, maxlen, strvalue, flags, min, max);
	break;
      case 'p':
	strvalue = va_arg (args, void *);
	fmtint (buffer, &currlen, maxlen, (long) strvalue, 16, min, max, flags);
	break;
      case 'n':
	if(cflags == DP_C_SHORT) 
	{
	  short int *num;
	  num = va_arg (args, short int *);
	  *num = currlen;
        } 
	else if(cflags == DP_C_LONG) 
	{
	  long int *num;
	  num = va_arg (args, long int *);
	  *num = currlen;
        } 
	else 
	{
	  int *num;
	  num = va_arg (args, int *);
	  *num = currlen;
        }
	break;
      case '%':
	dopr_outch (buffer, &currlen, maxlen, ch);
	break;
      case 'w':
	/* not supported yet, treat as next char */
	ch = *format++;
	break;
      default:
	/* Unknown, skip */
	break;
      }
      ch = *format++;
      state = DP_S_DEFAULT;
      flags = cflags = min = 0;
      max = -1;
      break;
    case DP_S_DONE:
      break;
    default:
      /* hmm? */
      break; /* some picky compilers need this */
    }
  }
  if(currlen < maxlen - 1) 
    buffer[currlen] = '\0';
  else 
    buffer[maxlen - 1] = '\0';

  return (int)currlen;
}


static void
fmtstr(char *buffer, size_t *currlen, size_t maxlen,
       char *value, int flags, int min, int max)
{
  int padlen, strln;     /* amount to pad */
  int cnt = 0;
  
  if(value == 0)
  {
    value = (char*)"<NULL>";
  }

  for(strln = 0; value[strln]; ++strln); /* strlen */
  padlen = min - strln;
  if(padlen < 0) 
    padlen = 0;
  if(flags & DP_F_MINUS) 
    padlen = -padlen; /* Left Justify */

  while ((padlen > 0) && (max == -1 || cnt < max)) 
  {
    dopr_outch (buffer, currlen, maxlen, ' ');
    --padlen;
    ++cnt;
  }
  while (*value && (max == -1 || cnt < max)) 
  {
    dopr_outch (buffer, currlen, maxlen, *value++);
    ++cnt;
  }
  while ((padlen < 0) && (max == -1 || cnt < max)) 
  {
    dopr_outch (buffer, currlen, maxlen, ' ');
    ++padlen;
    ++cnt;
  }
}


/* Have to handle DP_F_NUM (ie 0x and 0 alternates) */

static void
fmtint(char *buffer, size_t *currlen, size_t maxlen,
       long value, int base, int min, int max, int flags)
{
  int signvalue = 0;
  unsigned long uvalue;
  char convert[20];
  int place = 0;
  int spadlen = 0; /* amount to space pad */
  int zpadlen = 0; /* amount to zero pad */
  int caps = 0;
  
  if(max < 0)
    max = 0;

  uvalue = value;

  if(!(flags & DP_F_UNSIGNED))
  {
    if( value < 0 ) {
      signvalue = '-';
      uvalue = -value;
    }
    else
      if(flags & DP_F_PLUS)  /* Do a sign (+/i) */
	signvalue = '+';
    else
      if(flags & DP_F_SPACE)
	signvalue = ' ';
  }
  
  if(flags & DP_F_UP) caps = 1; /* Should characters be upper case? */

  do {
    convert[place++] =
      (caps? "0123456789ABCDEF":"0123456789abcdef")
      [uvalue % (unsigned)base  ];
    uvalue = (uvalue / (unsigned)base );
  } while(uvalue && (place < 20));
  if(place == 20) place--;
  convert[place] = 0;

  zpadlen = max - place;
  spadlen = min - MAX (max, place) - (signvalue ? 1 : 0);
  if(zpadlen < 0) zpadlen = 0;
  if(spadlen < 0) spadlen = 0;
  if(flags & DP_F_ZERO)
  {
    zpadlen = MAX(zpadlen, spadlen);
    spadlen = 0;
  }
  if(flags & DP_F_MINUS) 
    spadlen = -spadlen; /* Left Justifty */

#ifdef DEBUG_SNPRINTF
  dprint (1, (debugfile, "zpad: %d, spad: %d, min: %d, max: %d, place: %d\n",
      zpadlen, spadlen, min, max, place));
#endif

  /* Spaces */
  while (spadlen > 0) 
  {
    dopr_outch (buffer, currlen, maxlen, ' ');
    --spadlen;
  }

  /* Sign */
  if(signvalue) 
    dopr_outch (buffer, currlen, maxlen, signvalue);

  /* Zeros */
  if(zpadlen > 0) 
  {
    while (zpadlen > 0)
    {
      dopr_outch (buffer, currlen, maxlen, '0');
      --zpadlen;
    }
  }

  /* Digits */
  while (place > 0) 
    dopr_outch (buffer, currlen, maxlen, convert[--place]);
  
  /* Left Justified spaces */
  while (spadlen < 0) {
    dopr_outch (buffer, currlen, maxlen, ' ');
    ++spadlen;
  }
}
#endif


/** Convert a double to xsd:decimal representation.
 * Returned is a pointer to the first character of the number
 * in buffer (don't free it).
 */
char*
raptor_format_float(char *buffer, size_t *currlen, size_t maxlen,
                    double fvalue, unsigned int min, unsigned int max,
                    int flags)
{
  /* DBL_EPSILON = 52 digits */
  #define FRAC_MAX_LEN 52

  double ufvalue;
  double intpart;
  double fracpart = 0;
  double frac;
  double frac_delta = 10;
  double mod_10;
  size_t exp_len;
  size_t frac_len = 0;
  size_t idx;

  if (max < min)
    max = min;
  
  /* index to the last char */
  idx = maxlen - 1;

  buffer[idx--] = '\0';
  
  ufvalue = fabs (fvalue);
  intpart = round(ufvalue);

  /* We "cheat" by converting the fractional part to integer by
   * multiplying by a factor of 10
   */


  frac = (ufvalue - intpart);
  
  for (exp_len=0; exp_len <= max; ++exp_len) {
    frac *= 10;

    mod_10 = trunc(fmod(trunc(frac), 10));
    
    if (fabs(frac_delta - (fracpart / pow(10, exp_len))) < (DBL_EPSILON * 2.0)) {
      break;
    }
    
    frac_delta = fracpart / pow(10, exp_len);

    /* Only "append" (numerically) if digit is not a zero */
    if (mod_10 > 0 && mod_10 < 10) {
        fracpart = round(frac);
        frac_len = exp_len;
    }
  }
  
  if (frac_len < min) {
    buffer[idx--] = '0';
  } else {
    /* Convert/write fractional part (right to left) */
    do {
      mod_10 = fmod(trunc(fracpart), 10);
      --frac_len;
      
      buffer[idx--] = "0123456789"[(unsigned)mod_10];
      fracpart /= 10;

    } while(fracpart > 1 && (frac_len + 1) > 0);
  }

  buffer[idx--] = '.';

  /* Convert/write integer part (right to left) */
  do {
    buffer[idx--] = "0123456789"[(int)fmod(intpart, 10)];
    intpart /= 10;
  } while(round(intpart));
  
  /* Write a sign, if requested */
  if(fvalue < 0)
    buffer[idx--] = '-';
  else if(flags & DP_F_PLUS)
    buffer[idx--] = '+';
  
  *currlen = maxlen - idx - 2;
  return buffer + idx + 1;
}


#if 0
static void
dopr_outch(char *buffer, size_t *currlen, size_t maxlen, char c)
{
  if(*currlen < maxlen)
    buffer[*currlen] = c;
  (*currlen)++;
}
#endif

#endif /* !defined(HAVE_SNPRINTF) || !defined(HAVE_VSNPRINTF) */


#if 0
#ifndef HAVE_VSNPRINTF
int
vsnprintf(char *str, size_t count, const char *fmt, va_list args)
{
  str[0] = 0;
  return(dopr(str, count, fmt, args));
}
#endif /* !HAVE_VSNPRINTF */


#ifndef HAVE_SNPRINTF
/* VARARGS3 */
int
snprintf(char *str, size_t count, const char *fmt, ...)
{
  int len;
  va_list ap;
    
  va_start(ap, fmt);
  len = vsnprintf(str, count, fmt, ap);
  va_end(ap);
  return(len);
}
#endif


#ifdef TEST_SNPRINTF
#ifndef LONG_STRING
#define LONG_STRING 1024
#endif
int
main(void)
{
  char buf1[LONG_STRING];
  char buf2[LONG_STRING];
  char *fp_fmt[] = {
    "%-1.5f",
    "%1.5f",
    "%123.9f",
    "%10.5f",
    "% 10.5f",
    "%+22.9f",
    "%+4.9f",
    "%01.3f",
    "%4f",
    "%3.1f",
    "%3.2f",
    NULL
  };
  double fp_nums[] = { -1.5, 134.21, 91340.2, 341.1234, 0203.9, 0.96, 0.996, 
    0.9996, 1.996, 4.136, 0};
  char *int_fmt[] = {
    "%-1.5d",
    "%1.5d",
    "%123.9d",
    "%5.5d",
    "%10.5d",
    "% 10.5d",
    "%+22.33d",
    "%01.3d",
    "%4d",
    NULL
  };
  long int_nums[] = { -1, 134, 91340, 341, 0203, 0};
  int x, y;
  int fail = 0;
  int num = 0;

  printf ("Testing snprintf format codes against system sprintf...\n");

  for(x = 0; fp_fmt[x] != NULL ; x++)
    for(y = 0; fp_nums[y] != 0 ; y++)
    {
      snprintf (buf1, sizeof (buf1), fp_fmt[x], fp_nums[y]);
      sprintf (buf2, fp_fmt[x], fp_nums[y]);
      if(strcmp (buf1, buf2))
      {
	printf("snprintf doesn't match Format: %s\n\tsnprintf = %s\n\tsprintf  = %s\n",	/* __SPRINTF_CHECKED__ */
	    fp_fmt[x], buf1, buf2);
	fail++;
      }
      num++;
    }

  for(x = 0; int_fmt[x] != NULL ; x++)
    for(y = 0; int_nums[y] != 0 ; y++)
    {
      snprintf (buf1, sizeof (buf1), int_fmt[x], int_nums[y]);
      sprintf (buf2, int_fmt[x], int_nums[y]);
      if(strcmp (buf1, buf2))
      {
	printf("snprintf doesn't match Format: %s\n\tsnprintf = %s\n\tsprintf  = %s\n",	/* __SPRINTF_CHECKED__ */
	    int_fmt[x], buf1, buf2);
	fail++;
      }
      num++;
    }
  printf ("%d tests failed out of %d.\n", fail, num);
}
#endif /* SNPRINTF_TEST */

#endif /* !HAVE_SNPRINTF */
