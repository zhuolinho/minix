/*  users - list the users		Author: Terrence W. Holm */

/*  Usage: users
 *
 *  A simple users(1) command for MINIX. Assumes tty0
 *  to tty9 are the only possible login devices.
 *  See last.c for more robust code for reading wtmp.
 */


#include <sys/types.h>
#include <utmp.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef  WTMP
#define  WTMP   "/usr/adm/wtmp"
#endif

#define  min( a, b )     ( (a < b) ? a : b )

#define  BUFFER_SIZE     1024	/* Room for wtmp records  */
#define  MAX_WTMP_COUNT  ( BUFFER_SIZE / sizeof(struct utmp) )

struct utmp wtmp_buffer[MAX_WTMP_COUNT];

_PROTOTYPE(int main, (void));
_PROTOTYPE(int Strncmp, (const void *str1,const void *str2));
_PROTOTYPE(void Print_Users, (int user_count, char users[10][8]));

int main()
{
  FILE *f;
  long size;			/* Number of wtmp records in the file	 */
  int wtmp_count;		/* How many to read into wtmp_buffer	 */
  int used = 0;
  int user_count = 0;
  char users[10][8];


  if ((f = fopen(WTMP, "r")) == NULL)	/* No login/logout records kept  */
	exit(0);

  if (fseek(f, 0L, 2) != 0 || (size = ftell(f)) % sizeof(struct utmp) != 0) {
	fprintf(stderr, "users: invalid wtmp file\n");
	exit(1);
  }
  size /= sizeof(struct utmp);	/* Number of records in wtmp	 */


  while (size > 0) {
	wtmp_count = (int) min(size, MAX_WTMP_COUNT);
	size -= (long) wtmp_count;
	fseek(f, size * sizeof(struct utmp), 0);
	if (fread(&wtmp_buffer[0], sizeof(struct utmp), (size_t)wtmp_count, f) !=
	    wtmp_count) {
		fprintf(stderr, "users: read error on wtmp file\n");
		exit(1);
	}
	while (--wtmp_count >= 0) {
		int tty;

		if (strcmp(wtmp_buffer[wtmp_count].ut_line, "~") == 0) {
			Print_Users(user_count, users);
			exit(0);
		}
		tty = wtmp_buffer[wtmp_count].ut_line[3] - '0';

		if (tty < 0 || tty > 9) {
			fprintf(stderr, "users: encountered unknown tty in wtmp file\n");
			exit(1);
		}
		if (!(used & (1 << tty))) {
			used |= 1 << tty;
			memcpy(users[user_count], wtmp_buffer[wtmp_count].ut_name, (size_t)8);

			if (users[user_count][0] != '\0') ++user_count;
		}
	}

  }				/* end while( size > 0 ) */

  Print_Users(user_count, users);

  return(0);
}



int Strncmp(str1, str2)
_CONST void *str1;
_CONST void *str2;

{
  return(strncmp(str1, str2, (size_t)8));
}



void Print_Users(user_count, users)
int user_count;
char users[10][8];
{
  int i;

  qsort(users, (size_t)user_count, (size_t)8, Strncmp);

  for (i = 0; i < user_count; i++) {
	printf("%.8s ", users[i]);
  }
}
