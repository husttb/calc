/*
 * Copyright (c) 1996 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Scanf and printf routines for arbitrary precision rational numbers
 */

#include "qmath.h"
#include "config.h"
#include "args.h"


#define	PUTCHAR(ch)		math_chr(ch)
#define	PUTSTR(str)		math_str(str)
#define	PRINTF1(fmt, a1)	math_fmt(fmt, a1)
#define	PRINTF2(fmt, a1, a2)	math_fmt(fmt, a1, a2)

#if 0
static long	etoalen;
static char	*etoabuf = NULL;
#endif

static long	scalefactor;
static ZVALUE	scalenumber = { 0, 0, 0 };


/*
 * Print a formatted string containing arbitrary numbers, similar to printf.
 * ALL numeric arguments to this routine are rational NUMBERs.
 * Various forms of printing such numbers are supplied, in addition
 * to strings and characters.  Output can actually be to any FILE
 * stream or a string.
 */
void
qprintf(char *fmt, ...)
{
	va_list ap;
	NUMBER *q;
	int ch, sign = 1;
	long width = 0, precision = 0;
	int trigger = 0;

	va_start(ap, fmt);
	while ((ch = *fmt++) != '\0') {
		if (trigger == 0) {
			if (ch == '\\') {
				ch = *fmt++;
				switch (ch) {
					case 'n': ch = '\n'; break;
					case 'r': ch = '\r'; break;
					case 't': ch = '\t'; break;
					case 'f': ch = '\f'; break;
					case 'v': ch = '\v'; break;
					case 'b': ch = '\b'; break;
					case 0:
						va_end(ap);
						return;
				}
				PUTCHAR(ch);
				continue;
			}
			if (ch != '%') {
				PUTCHAR(ch);
				continue;
			}
			ch = *fmt++;
			width = 0; precision = 8; sign = 1;
			trigger = 1;
		}

		switch (ch) {
		case 'd':
			q = va_arg(ap, NUMBER *);
			qprintfd(q, width);
			break;
		case 'f':
			q = va_arg(ap, NUMBER *);
			qprintff(q, width, precision);
			break;
		case 'e':
			q = va_arg(ap, NUMBER *);
			qprintfe(q, width, precision);
			break;
		case 'r':
		case 'R':
			q = va_arg(ap, NUMBER *);
			qprintfr(q, width, (BOOL) (ch == 'R'));
			break;
		case 'N':
			q = va_arg(ap, NUMBER *);
			zprintval(q->num, 0L, width);
			break;
		case 'D':
			q = va_arg(ap, NUMBER *);
			zprintval(q->den, 0L, width);
			break;
		case 'o':
			q = va_arg(ap, NUMBER *);
			qprintfo(q, width);
			break;
		case 'x':
			q = va_arg(ap, NUMBER *);
			qprintfx(q, width);
			break;
		case 'b':
			q = va_arg(ap, NUMBER *);
			qprintfb(q, width);
			break;
		case 's':
			PUTSTR(va_arg(ap, char *));
			break;
		case 'c':
			PUTCHAR(va_arg(ap, int));
			break;
		case 0:
			va_end(ap);
			return;
		case '-':
			sign = -1;
			ch = *fmt++;
		default:
			if (('0' <= ch && ch <= '9') ||
			    ch == '.' || ch == '*') {
				if (ch == '*') {
					q = va_arg(ap, NUMBER *);
					width = sign * qtoi(q);
					ch = *fmt++;
				} else if (ch != '.') {
					width = ch - '0';
					while ('0' <= (ch = *fmt++) &&
					       ch <= '9')
						width = width * 10 + ch - '0';
					width *= sign;
				}
				if (ch == '.') {
					if ((ch = *fmt++) == '*') {
						q = va_arg(ap, NUMBER *);
						precision = qtoi(q);
						ch = *fmt++;
					} else {
						precision = 0;
						while ('0' <= (ch = *fmt++) &&
						       ch <= '9')
							precision *= 10+ch-'0';
					}
				}
			}
		}
	}
	va_end(ap);
}


#if 0
/*
 * Read a number from the specified FILE stream (NULL means stdin).
 * The number can be an integer, a fraction, a real number, an
 * exponential number, or a hex, octal or binary number.  Leading blanks
 * are skipped.  Illegal numbers return NULL.  Unrecognized characters
 * remain to be read on the line.
 *	q = qreadval(fp);
 *
 * given:
 *	fp		file stream to read from (or NULL)
 */
NUMBER *
qreadval(FILE *fp)
{
	NUMBER *r;		/* returned number */
	char *cp; 		/* current buffer location */
	long savecc;		/* characters saved in buffer */
	long scancc; 		/* characters parsed correctly */
	int ch;			/* current character */

	if (fp == NULL)
		fp = stdin;
	if (etoabuf == NULL) {
		etoabuf = (char *)malloc(OUTBUFSIZE + 2);
		if (etoabuf == NULL)
			return NULL;
		etoalen = OUTBUFSIZE;
	}
	cp = etoabuf;
	ch = fgetc(fp);
	while ((ch == ' ') || (ch == '\t'))
		ch = fgetc(fp);
	savecc = 0;
	for (;;) {
		if (ch == EOF)
			return NULL;
		if (savecc >= etoalen)
		{
			cp = (char *)realloc(etoabuf, etoalen + OUTBUFSIZE + 2);
			if (cp == NULL)
				return NULL;
			etoabuf = cp;
			etoalen += OUTBUFSIZE;
			cp += savecc;
		}
		*cp++ = (char)ch;
		*cp = '\0';
		scancc = qparse(etoabuf, QPF_SLASH);
		if (scancc != ++savecc)
			break;
		ch = fgetc(fp);
	}
	ungetc(ch, fp);
	if (scancc < 0)
		return NULL;
	r = str2q(etoabuf);
	if (ziszero(r->den)) {
		qfree(r);
		r = NULL;
	}
	return r;
}
#endif


/*
 * Print a number in the specified output mode.
 * If MODE_DEFAULT is given, then the default output mode is used.
 * Any approximate output is flagged with a leading tilde.
 * Integers are always printed as themselves.
 */
void
qprintnum(NUMBER *q, int outmode)
{
	NUMBER tmpval;
	long prec, exp;

	if (outmode == MODE_DEFAULT)
		outmode = conf->outmode;
	switch (outmode) {
		case MODE_INT:
			if (conf->tilde_ok && qisfrac(q))
				PUTCHAR('~');
			qprintfd(q, 0L);
			break;

		case MODE_REAL:
			prec = qplaces(q);
			if ((prec < 0) || (prec > conf->outdigits)) {
				if (conf->tilde_ok)
				    PUTCHAR('~');
			}
			if (conf->fullzero || (prec < 0) ||
			    (prec > conf->outdigits))
				prec = conf->outdigits;
			qprintff(q, 0L, prec);
			break;

		case MODE_FRAC:
			qprintfr(q, 0L, FALSE);
			break;

		case MODE_EXP:
			if (qiszero(q)) {
				PUTCHAR('0');
				return;
			}
			tmpval = *q;
			tmpval.num.sign = 0;
			exp = qilog10(&tmpval);
			if (exp == 0) {		/* in range to output as real */
				qprintnum(q, MODE_REAL);
				return;
			}
			tmpval.num = _one_;
			tmpval.den = _one_;
			if (exp > 0)
				ztenpow(exp, &tmpval.den);
			else
				ztenpow(-exp, &tmpval.num);
			q = qmul(q, &tmpval);
			zfree(tmpval.num);
			zfree(tmpval.den);
			qprintnum(q, MODE_REAL);
			qfree(q);
			PRINTF1("e%ld", exp);
			break;

		case MODE_HEX:
			qprintfx(q, 0L);
			break;

		case MODE_OCTAL:
			qprintfo(q, 0L);
			break;

		case MODE_BINARY:
			qprintfb(q, 0L);
			break;

		default:
			math_error("Bad mode for print");
			/*NOTREACHED*/
	}
}


/*
 * Print a number in floating point representation.
 * Example:  193.784
 */
void
qprintff(NUMBER *q, long width, long precision)
{
	ZVALUE z, z1;

	if (precision != scalefactor) {
		if (scalenumber.v)
			zfree(scalenumber);
		ztenpow(precision, &scalenumber);
		scalefactor = precision;
	}
	if (scalenumber.v)
		zmul(q->num, scalenumber, &z);
	else
		z = q->num;
	if (qisfrac(q)) {
		zquo(z, q->den, &z1, conf->outround);
		if (z.v != q->num.v)
			zfree(z);
		z = z1;
	}
	if (qisneg(q) && ziszero(z))
		PUTCHAR('-');
	zprintval(z, precision, width);
	if (z.v != q->num.v)
		zfree(z);
}


/*
 * Print a number in exponential notation.
 * Example: 4.1856e34
 */
/*ARGSUSED*/
void
qprintfe(NUMBER *q, long width, long precision)
{
	long exponent;
	NUMBER q2;
	ZVALUE num, zden, tenpow, tmp;

	if (qiszero(q)) {
		PUTSTR("0.0");
		return;
	}
	num = q->num;
	zden = q->den;
	num.sign = 0;
	exponent = zdigits(num) - zdigits(zden);
	if (exponent > 0) {
		ztenpow(exponent, &tenpow);
		zmul(zden, tenpow, &tmp);
		zfree(tenpow);
		zden = tmp;
	}
	if (exponent < 0) {
		ztenpow(-exponent, &tenpow);
		zmul(num, tenpow, &tmp);
		zfree(tenpow);
		num = tmp;
	}
	if (zrel(num, zden) < 0) {
		zmuli(num, 10L, &tmp);
		if (num.v != q->num.v)
			zfree(num);
		num = tmp;
		exponent--;
	}
	q2.num = num;
	q2.den = zden;
	q2.num.sign = q->num.sign;
	qprintff(&q2, 0L, precision);
	if (exponent)
		PRINTF1("e%ld", exponent);
	if (num.v != q->num.v)
		zfree(num);
	if (zden.v != q->den.v)
		zfree(zden);
}


/*
 * Print a number in rational representation.
 * Example: 397/37
 */
void
qprintfr(NUMBER *q, long width, BOOL force)
{
	zprintval(q->num, 0L, width);
	if (force || qisfrac(q)) {
		PUTCHAR('/');
		zprintval(q->den, 0L, width);
	}
}


/*
 * Print a number as an integer (truncating fractional part).
 * Example: 958421
 */
void
qprintfd(NUMBER *q, long width)
{
	ZVALUE z;

	if (qisfrac(q)) {
		zquo(q->num, q->den, &z, conf->outround);
		zprintval(z, 0L, width);
		zfree(z);
	} else {
		zprintval(q->num, 0L, width);
	}
}


/*
 * Print a number in hex.
 * This prints the numerator and denominator in hex.
 */
void
qprintfx(NUMBER *q, long width)
{
	zprintx(q->num, width);
	if (qisfrac(q)) {
		PUTCHAR('/');
		zprintx(q->den, 0L);
	}
}


/*
 * Print a number in binary.
 * This prints the numerator and denominator in binary.
 */
void
qprintfb(NUMBER *q, long width)
{
	zprintb(q->num, width);
	if (qisfrac(q)) {
		PUTCHAR('/');
		zprintb(q->den, 0L);
	}
}


/*
 * Print a number in octal.
 * This prints the numerator and denominator in octal.
 */
void
qprintfo(NUMBER *q, long width)
{
	zprinto(q->num, width);
	if (qisfrac(q)) {
		PUTCHAR('/');
		zprinto(q->den, 0L);
	}
}


/*
 * Convert a string to a number in rational, floating point,
 * exponential notation, hex, or octal.
 *	q = str2q(string);
 */
NUMBER *
str2q(char *s)
{
	register NUMBER *q;
	register char *t;
	ZVALUE div, newnum, newden, tmp;
	long decimals, exp;
	BOOL hex, negexp;

	q = qalloc();
	decimals = 0;
	exp = 0;
	negexp = FALSE;
	hex = FALSE;
	t = s;
	if ((*t == '+') || (*t == '-'))
		t++;
	if ((*t == '0') && ((t[1] == 'x') || (t[1] == 'X'))) {
		hex = TRUE;
		t += 2;
	}
	while (((*t >= '0') && (*t <= '9')) || (hex &&
		(((*t >= 'a') && (*t <= 'f')) || ((*t >= 'A') && (*t <= 'F')))))
			t++;
	if (*t == '/') {
		t++;
		str2z(t, &q->den);
	} else if ((*t == '.') || (*t == 'e') || (*t == 'E')) {
		if (*t == '.') {
			t++;
			while ((*t >= '0') && (*t <= '9')) {
				t++;
				decimals++;
			}
		}
		/*
		 * Parse exponent if any
		 */
		if ((*t == 'e') || (*t == 'E')) {
			t++;
			if (*t == '+')
				t++;
			else if (*t == '-') {
				negexp = TRUE;
				t++;
			}
			while ((*t >= '0') && (*t <= '9')) {
				exp = (exp * 10) + *t++ - '0';
				if (exp > 1000000) {
					math_error("Exponent too large");
					/*NOTREACHED*/
				}
			}
		}
		ztenpow(decimals, &q->den);
	}
	str2z(s, &q->num);
	if (qiszero(q)) {
		qfree(q);
		return qlink(&_qzero_);
	}
	/*
	 * Apply the exponential if any
	 */
	if (exp) {
		ztenpow(exp, &tmp);
		if (negexp) {
			zmul(q->den, tmp, &newden);
			zfree(q->den);
			q->den = newden;
		} else {
			zmul(q->num, tmp, &newnum);
			zfree(q->num);
			q->num = newnum;
		}
		zfree(tmp);
	}
	/*
	 * Reduce the fraction to lowest terms
	 */
	if (!zisunit(q->num) && !zisunit(q->den)) {
		zgcd(q->num, q->den, &div);
		if (!zisunit(div)) {
			zequo(q->num, div, &newnum);
			zfree(q->num);
			zequo(q->den, div, &newden);
			zfree(q->den);
			q->num = newnum;
			q->den = newden;
		}
		zfree(div);
	}
	return q;
}


/*
 * Parse a number in any of the various legal forms, and return the count
 * of characters that are part of a legal number.  Numbers can be either a
 * decimal integer, possibly two decimal integers separated with a slash, a
 * floating point or exponential number, a hex number beginning with "0x",
 * a binary number beginning with "0b", or an octal number beginning with "0".
 * The flags argument modifies the end of number testing for ease in handling
 * fractions or complex numbers.  Minus one is returned if the number format
 * is definitely illegal.
 */
long
qparse(char *cp, int flags)
{
	char *oldcp;

	oldcp = cp;
	if ((*cp == '+') || (*cp == '-'))
		cp++;
	if ((*cp == '+') || (*cp == '-'))
		return -1;

	/* hex */
	if ((*cp == '0') && ((cp[1] == 'x') || (cp[1] == 'X'))) {
		cp += 2;
		while (((*cp >= '0') && (*cp <= '9')) ||
			((*cp >= 'a') && (*cp <= 'f')) ||
			((*cp >= 'A') && (*cp <= 'F')))
				cp++;
		if (((*cp == 'i') || (*cp == 'I')) && (flags & QPF_IMAG))
			cp++;
		if ((*cp == '.') || ((*cp == '/') && (flags & QPF_SLASH)) ||
			((*cp >= '0') && (*cp <= '9')) ||
			((*cp >= 'a') && (*cp <= 'z')) ||
			((*cp >= 'A') && (*cp <= 'Z')))
				return -1;
		return (cp - oldcp);
	}

	/* binary */
	if ((*cp == '0') && ((cp[1] == 'b') || (cp[1] == 'B'))) {
		cp += 2;
		while ((*cp == '0') || (*cp == '1'))
			cp++;
		if (((*cp == 'i') || (*cp == 'I')) && (flags & QPF_IMAG))
			cp++;
		if ((*cp == '.') || ((*cp == '/') && (flags & QPF_SLASH)) ||
			((*cp >= '0') && (*cp <= '9')) ||
			((*cp >= 'a') && (*cp <= 'z')) ||
			((*cp >= 'A') && (*cp <= 'Z')))
				return -1;
		return (cp - oldcp);
	}

	/* octal */
	if ((*cp == '0') && (cp[1] >= '0') && (cp[1] <= '9')) {
		while ((*cp >= '0') && (*cp <= '7'))
			cp++;
		if (((*cp == 'i') || (*cp == 'I')) && (flags & QPF_IMAG))
			cp++;
		if ((*cp == '.') || ((*cp == '/') && (flags & QPF_SLASH)) ||
			((*cp >= '0') && (*cp <= '9')) ||
			((*cp >= 'a') && (*cp <= 'z')) ||
			((*cp >= 'A') && (*cp <= 'Z')))
				return -1;
		return (cp - oldcp);
	}

	/*
	 * Number is decimal but can still be a fraction or real or exponential
	 */
	while ((*cp >= '0') && (*cp <= '9'))
		cp++;
	if (*cp == '/' && flags & QPF_SLASH) {	/* fraction */
		cp++;
		while ((*cp >= '0') && (*cp <= '9'))
			cp++;
		if (((*cp == 'i') || (*cp == 'I')) && (flags & QPF_IMAG))
			cp++;
		if ((*cp == '.') || ((*cp == '/') && (flags & QPF_SLASH)) ||
			((*cp >= '0') && (*cp <= '9')) ||
			((*cp >= 'a') && (*cp <= 'z')) ||
			((*cp >= 'A') && (*cp <= 'Z')))
				return -1;
		return (cp - oldcp);
	}
	if (*cp == '.') {	/* floating point */
		cp++;
		while ((*cp >= '0') && (*cp <= '9'))
			cp++;
	}
	if ((*cp == 'e') || (*cp == 'E')) {	/* exponential */
		cp++;
		if ((*cp == '+') || (*cp == '-'))
			cp++;
		if ((*cp == '+') || (*cp == '-'))
			return -1;
		while ((*cp >= '0') && (*cp <= '9'))
			cp++;
	}

	if (((*cp == 'i') || (*cp == 'I')) && (flags & QPF_IMAG))
		cp++;
	if ((*cp == '.') || ((*cp == '/') && (flags & QPF_SLASH)) ||
		((*cp >= '0') && (*cp <= '9')) ||
		((*cp >= 'a') && (*cp <= 'z')) ||
		((*cp >= 'A') && (*cp <= 'Z')))
			return -1;
	return (cp - oldcp);
}


/*
 * Print an integer which is guaranteed to fit in the specified number
 * of columns, using imbedded '...' characters if numerator and/or
 * denominator is too large.
 */
void
fitprint(NUMBER *q, long width)
{
	long numdigits, dendigits, digits;
	long width1, width2;
	long n, k;

	if (width < 8)
		width = 8;
	numdigits = zdigits(q->num);
	n = numdigits;
	k = 0;
	while (++k, n)
		n /= 10;
	if (qisint(q)) {
		width -= k;
		k = 16 - k;
		if (k < 2)
			k = 2;
		PRINTF1("(%ld)", numdigits);
		while (k-- > 0)
			PUTCHAR(' ');
		fitzprint(q->num, numdigits, width);
		return;
	}
	dendigits = zdigits(q->den);
	PRINTF2("(%ld/%ld)", numdigits, dendigits);
	digits = numdigits + dendigits;
	n = dendigits;
	while (++k, n)
		n /= 10;
	width -= k;
	k = 16 - k;
	if (k < 2)
		k = 2;
	while (k-- > 0)
		PUTCHAR(' ');
	if (digits <= width) {
		qprintf("%r", q);
		return;
	}
	width1 = (width * numdigits)/digits;
	if (width1 < 8)
		width1 = 8;
	width2 = width - width1;
	if (width2 < 8) {
		width2 = 8;
		width1 = width - width2;
	}
	fitzprint(q->num, numdigits, width1);
	PUTCHAR('/');
	fitzprint(q->den, dendigits, width2);
}

/* END CODE */
