/* ELLE - Copyright 1982, 1985, 1987 by Ken Harrenstien, SRI International
 *	This software is quasi-public; it may be used freely with
 *	like software, but may NOT be sold or made part of licensed
 *	products without permission of the author.
 */
/*
 * EEFILL	Fill Mode functions
 */

#include "elle.h"

extern int ev_fcolumn;	/* Fill Column variable (defined in EEVINI) */
#if FX_SFPREF
char *fill_prefix;	/* Fill Prefix variable */
int fill_plen;		/* Length of Fill Prefix (0 = no prefix) */
#endif /*FX_SFPREF*/

#if FX_FILLMODE
int fill_mode = 0;	/* TRUE when Auto Fill Mode is on */
int *fill_trig;		/* Pointer to fill-trigger chbit array */
static char *fill_initrig = " \t.,;:)!";
#endif /*FX_FILLMODE*/

/* Following stuff for testing routines on */
/*

          1         2         3         4         5	    6         7
0123456789012345678901234567890123456789012345678901234567890123456789012345

Okay...  more stuff to hack.  Okay.  a b c d e f g h i j k l m
n o p q r s t u v w x y z dfsd stuff to hack 01234 Okay testing
more stuff to hack.  Okay...  more stuff to hack more stuff to
hack.  Okay...  more stuff to line long stuff to hack.  Okay...
even more gap and.  period.  okay, end of stuff.
	This is another fence.
*/


#if FX_SFCOL
/* EFUN: "Set Fill Column" */
f_sfcol()
{	register int linel;
	char temp[20];

	linel = exp_p ? exp : d_curind();
	if(linel < 0) linel = 0;
	say("Fill column = ");
	dottoa(temp,(chroff)linel);
	saytoo(temp);
	ev_fcolumn = linel;
}
#endif /*FX_SFCOL*/


#if FX_SFPREF
/* EFUN: "Set Fill Prefix" */
f_sfpref()
{	register int i;
	register char *cp;

	if((i = cur_dot - e_boldot()) > MAXLINE)
	  {	ding("Absurd Fill Prefix");
		return;
	  }
	if(fill_prefix)
	  {	chkfree(fill_prefix);
		fill_plen = 0;
	  }
	if(i <= 0)
	  {	fill_prefix = 0;
		cp = "";
	  }
	else
	  {	fill_prefix = cp = memalloc((SBMO)(i+1));
		fill_plen = i;
		e_gobol();
		do { *cp++ = e_getc(); }
		while(--i);
		*cp = 0;
		cp = fill_prefix;
	  }
	say("Fill Prefix = \"");
	saytoo(cp);
	saytoo("\"");
}


/* TSTFILLP(lim) - Check for existence of Fill Prefix at current dot.  If
 *	not there, returns 0 without changing dot.  If there, returns
 *	1 and leaves dot immediately after the Fill Prefix.
 *	Lim = # of chars allowed to scan from buffer.
 */
tstfillp(lim)
int lim;
{	register int i, c;
	register char *cp;
	chroff savdot;

	if(!(i = fill_plen) || (i > lim))
		return(0);
	savdot = e_dot();
	cp = fill_prefix;
	do {	if(*cp++ != e_getc())
		  {	e_go(savdot);
			return(0);
		  }
	  } while(--i);
	return(1);
}
#endif /*FX_SFPREF*/

#if FX_FILLREG || FX_FILLPARA

/* ED_FILL(start, end, flag) - Fill a region.
 *	Flag	0 for full filling; extra whitespace is flushed.  First
 *			word is always retained.
 *		1 for skimpy filling such as Auto-Fill likes.
 *			Extra whitespace is NOT flushed, except at
 *			beginning of a newly created line.
 *			This is not yet implemented however.
 * Note: updates cur_dot to compensate for changes in buffer, and returns
 *	there when done!
 * Note: Checks for Fill Prefix when it exists.
 */
ed_fill(begloc, endloc, flag)
chroff begloc, endloc;
int flag;
{	register int c;
	register int len, lastc;
	chroff savloc;
	int lastbrk;
	int parlen;

	parlen = endloc - begloc;
	if(parlen < 0)
	  {	begloc = endloc;
		parlen = -parlen;
	  }
	e_go(begloc);
	len = d_curind();		/* Set up current col */

#if FX_SFPREF
	/* If at beg of line, check for fill prefix and skip over it */
	if((len == 0) && tstfillp(parlen))
	  {	parlen -= fill_plen;
		len = d_curind();
	  }
#endif /*FX_SFPREF*/
	lastbrk = 0;			/* Put next word on no matter what. */
	c = 0;
	for(;;)
	  {
#if ICONOGRAPHICS
             if (c != ')' && c != '"')  /* allow for two sp after .) or ." */
#endif /*ICONOGRAPHICS*/
		lastc = c;
		if(--parlen < 0) break;
		c = e_getc();
		if(c == EOF)
			break;
#if FX_SFPREF
		/* If at beg of line, check for fill prefix and flush it */
		if((c == LF) && tstfillp(parlen))
		  {	e_igoff(-(fill_plen+1));
			e_ovwc(c = SP);
			e_deln((chroff)fill_plen);
			parlen -= fill_plen;
			if(cur_dot >= e_dot())
				cur_dot -= fill_plen;
		  }
#endif /*FX_SFPREF*/
		if(c == TAB || c == LF)		/* Replace tabs+eols by sps */
		  {	e_backc();		/* Back up 1 */
			e_ovwc(c = SP);
		  }
		if(c == SP)
		  {	if(lastc == SP)
			  {	e_rdelc();
				if(cur_dot > e_dot()) --cur_dot;
				continue;
			  }
			lastbrk = len;
			if(lastc == '.' || lastc == '!' || lastc == '?'
#if ICONOGRAPHICS
                                                        || lastc == ':'
#endif /*ICONOGRAPHICS*/
									)
			  {	if(--parlen < 0) goto done;
				if((c = e_getc()) == EOF)
					goto done;
				len++;
				if(c != SP)
				  {	e_backc();
					e_putc(c = SP);
					if(cur_dot >= e_dot()) ++cur_dot;
				  }
			  }
		  }
#if ICONOGRAPHICS
		if (c == BS)                    /* adjust for backspaces */
			if ((len -= 2) < 0) len = 0;
#endif /*ICONOGRAPHICS*/
		/* Normal char */
		if(++len > ev_fcolumn && lastbrk)	/* If went too far */
		  {	c = lastbrk - len;	/* Must put EOL at last SP */
			e_igoff(c);
			parlen -= c;	/* C is negative, actually adding */
			parlen--;
			e_ovwc(LF);
			lastbrk = 0;
			len = 0;
			c = SP;		/* Pretend this char was space */
#if FX_SFPREF
			if(fill_plen)
			  {	if(cur_dot >= e_dot())
					cur_dot += fill_plen;
				/* Better hope no nulls in prefix! */
				e_sputz(fill_prefix);
				len = d_curind();
			  }
#endif /*FX_SFPREF*/
		  }
	  }
done:	savloc = cur_dot;
	e_setcur();	/* Reached paragraph end, set cur_dot temporarily */
	buf_tmod(begloc-cur_dot);	/* So that proper range is marked */
	e_gosetcur(savloc);		/* Then restore original cur_dot */
}
#endif /*FX_FILLREG || FX_FILLPARA*/

#if FX_FILLMODE

/* EFUN: "Auto Fill Mode" */
/*	Toggles Auto Fill Mode (a minor mode). */
f_fillmode()
{	register char *cp;
	int *chballoc();

	fill_mode = fill_mode ? 0 : 1;
	if(!fill_trig)
	  {	fill_trig = chballoc(128);
		for(cp = fill_initrig; *cp; ++cp)
			chbis(fill_trig, *cp);
	  }
	redp(RD_MODE);
}

/* Called by F_INSSELF to handle char insertion in Auto Fill mode */
fx_insfill(c)
int c;
{
	ed_insn(c,exp);
	if(chbit(fill_trig, c))
	  {	fill_cur_line();

	  }
}


fill_cur_line()
{
	register int foundit, i;
	chroff lastbrkdot, boldot, eoldot;

	boldot = e_boldot();

	/* First back up to find place to make first break. */
	e_bwsp();
	lastbrkdot = e_dot();
	foundit = 0;
	for(foundit = 0; foundit >= 0;)
	  {	if((i = d_curind()) <= ev_fcolumn)
		  {	if(foundit)
				foundit = -1;
			else break;
		  }
		else ++foundit;
		while (!c_wsp (e_rgetc ())) ;
		e_bwsp();
		lastbrkdot = e_dot();
		if(lastbrkdot <= boldot)
		  {	lastbrkdot = boldot;
			break;
		  }
	  }

	if(foundit)
		ed_fill(lastbrkdot, e_eoldot(), 1);
}
#endif /*FX_FILLMODE*/

#if IMAGEN

#if FX_TEXTMODE
/* EFUN: "Text Mode Toggle" (not EMACS) */
f_textmode()
{
	cur_buf->b_flags ^= B_TEXTMODE;
	redp(RD_MODE);
}
#endif /*FX_TEXTMODE*/

int curr_indent = -1;		/* Current indent (for text mode autowrap) */
				/*  (misnomered: actually current column) */
chroff best_break;		/* Best break point so far */


/* Fill-mode version of "I