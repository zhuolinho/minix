/* clr - clear the screen		Author: Andy Tanenbaum */

/* This version modified by Rich Heffel */

/* Use termcap for normal, reverse, clear-line. */

#include <sys/types.h>
#include <sys/stat.h>
#include <sgtty.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termcap.h>
#include <unistd.h>
#include <stdio.h>

#define reverse()	write(1, SO, strlen(SO))	/* reverse video */
#define normal()	write(1, SE, strlen(SE))	/* undo reverse() */
#define clearln()	write(1,"\r",1); \
		write(1, CD, strlen(CD))	/* clear line */


#define  TC_BUFFER  1024	/* Size of termcap(3) buffer	 */

struct sgttyb ttymode;		/* and the terminal modes */
char *SO, *SE, *CD;
char buffer[TC_BUFFER];
char *term;
char clear[30];
char *p = &clear[0];

_PROTOTYPE(int main, (void));
_PROTOTYPE(void get_termcap, (void));
_PROTOTYPE(void Error, (char *str));

int main()
{

  get_termcap();

  /* Clear the screen  */
  normal();
  clearln();
  printf("%s", clear);

  return(0);
}

void get_termcap()
{
  static char termbuf[50];
  char *loc = termbuf;
  char entry[1024];

  
  if ((term = getenv("TERM")) == NULL) {
	Error("$TERM not defined");
  }
  if (tgetent(entry, getenv("TERM")) <= 0) {
        Error("Unknown terminal.");
  }
  if (tgetent(buffer, term) != 1) {
	Error("No termcap definition for $TERM");
  }
  if ((tgetstr("cl", &p)) == NULL) {
	Error("No clear (cl) entry for $TERM");
  }
  SO = tgetstr("so", &loc);
  SE = tgetstr("se", &loc);
  CD = tgetstr("cd", &loc);

  if (CD == (char *) 0) CD = "             \r";

}

void Error(str)
char *str;
{
  fprintf(stderr, "clr: %s\n", str);
  exit(1);
}
