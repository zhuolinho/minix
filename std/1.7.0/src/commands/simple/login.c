/* login - log into the system			Author: Patric van Kleef */

/* Original version by Patrick van Kleef.  History of modifications:
 *
 * Peter S. Housel   Jan. 1988
 *  - Set up $USER, $HOME and $TERM.
 *  - Set signals to SIG_DFL.
 *
 * Terrence W. Holm   June 1988
 *  - Allow a username as an optional argument.
 *  - Time out if a password is not typed within 30 seconds.
 *  - Perform a dummy delay after a bad username is entered.
 *  - Don't allow a login if "/etc/nologin" exists.
 *  - Cause a failure on bad "pw_shell" fields.
 *  - Record the login in "/usr/adm/wtmp".
 *
 * Peter S. Housel   Dec. 1988
 *  - Record the login in "/etc/utmp" also.
 *
 * F. van Kempen     June 1989
 *  - various patches for Minix V1.4a.
 *
 * F. van Kempen     September 1989
 *  - added login-failure administration (new utmp.h needed!).
 *  - support arguments in pw_shell field
 *  - adapted source text to MINIX Style Sheet
 *
 * F. van Kempen     October 1989
 *  - adapted to new utmp database.
 * F. van Kempen,    December 1989
 *  - fixed 'slot' assumption in wtmp()
 *  - fixed all MSS-stuff
 *  - adapted to POSIX (MINIX 1.5)
 * F. van Kempen,    January 1990
 *  - made all 'bad login accounting' optional by "#ifdef BADLOG".
 * F. van Kempen,    Februari 1990
 *  - fixed 'first argument' bug and added some casts.
 *
 * Andy Tanenbaum April 1990
 * - if /bin/sh cannot be located, try /usr/bin/sh
 *
 * Michael A. Temari	October 1990
 *  - handle more than single digit tty devices
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sgtty.h>
#include <signal.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <utmp.h>
#include <stdlib.h>
#include <time.h>
#include <minix/minlib.h>

#define MOTD 		    "/etc/motd"
#define TTY  		         "tty"
#define CONS 		         "tty0"

static char *Version = "@(#) LOGIN 1.13 (02/10/90)";
int time_out;
char user[32];
char logname[35];
char home[64];
char shell[64];
char minus_shell[1 + 64];
char *env[] = {
  user,
  logname,
  home,
  shell,
  "TERM=minix",
  NULL
};

_PROTOTYPE(int main, (int argc, char **argv));
_PROTOTYPE(void wtmp, (char *line, char *user));
_PROTOTYPE(void addlog, (char *nam, char *pwd, char *tty));
_PROTOTYPE(void show_file, (char *nam));
_PROTOTYPE(void say, (char *s));
_PROTOTYPE(void Time_out, (int dummy));

void wtmp(line, user)
char *line;			/* tty device name */
char *user;			/* user name */
{
  /* Make entries in /usr/adm/wtmp and /etc/utmp. */
  struct utmp entry;
  register int fd;
  register char *p;
  int lineno;
  off_t lineoff;

  /* Strip off the /dev part of the TTY name. */
  p = strrchr(line, '/');
  if (p != NULL) line = p+1;

  /* First, read the current UTMP entry. we need some of its
   * parameters! (like PID, ID etc...). */
  if ((fd = open(UTMP, O_RDONLY)) < 0) return;
  lineno = 0;
  while (read(fd, (char *) &entry, sizeof(struct utmp))
					== sizeof(struct utmp)) {
	if (entry.ut_pid == getpid()) break;
	lineno++;
  }
  lineoff = lineno * (off_t) sizeof(struct utmp);

  close(fd);

  /* Enter new string fields. */
  strncpy(entry.ut_user, user, sizeof(entry.ut_user));
  strncpy(entry.ut_line, line, sizeof(entry.ut_line));

  /* Change new numeric fields. */
  entry.ut_type = USER_PROCESS;	/* we are past login... */
  time(&(entry.ut_time));

  /* Write a WTMP record. */
  if ((fd = open(WTMP, O_WRONLY)) > 0) {
	if (lseek(fd, (off_t)0, SEEK_END) != (off_t)-1) {
		write(fd, (char *) &entry, sizeof(struct utmp));
	}
	close(fd);
  }
  /* Rewrite the UTMP entry. */
  if ((fd = open(UTMP, O_WRONLY)) > 0) {
	if (lseek(fd, lineoff, SEEK_SET) != (off_t)-1) {
		write(fd, (char *) &entry, sizeof(struct utmp));
	}
	close(fd);
  }
}


#ifdef BADLOG
void addlog(nam, pwd, tty)
char *nam;			/* name of attempting user */
char *pwd;			/* attempted password */
char *tty;			/* name of terminal line */
{
/* Register a failed login if the logfile exists. */
  struct utmp entry;
  int fd;

  strncpy(entry.ut_name, nam, sizeof(entry.ut_name));
  strncpy(entry.ut_line, tty, sizeof(entry.ut_line));
  entry.ut_time = time((time_t *)0);

  if ((fd = open(BTMP, O_WRONLY)) < 0) return;

  if (lseek(fd, (off_t)0, SEEK_END) != (off_t)-1) {
	/* Append the entry to the btmp file. */
	write(fd, (char *) &entry, sizeof(struct utmp));
  }
  close(fd);
}
#endif /* BADLOG */


void show_file(nam)
char *nam;
{
/* Read a textfile and show it on the desired terminal. */
  register int fd, len;
  char buf[80];

  if ((fd = open(nam, O_RDONLY)) > 0) {
	len = 1;
	while (len > 0) {
		len = read(fd, buf, 80);
		write(1, buf, len);
	}
	close(fd);
  }
}


void say(s) char *s;
{
  write(1, s, strlen(s));
}


int main(argc, argv)
int argc;
char *argv[];
{
  char name[30];
  char password[30];
  char ttyname[16];
  int bad, n, ttynr, ap;
  struct sgttyb args;
  struct passwd *pwd;
  struct stat statbuf;
  char *bp, *argx[8];		/* pw_shell arguments */
  char *sh = "/bin/sh";		/* sh/pw_shell field value */
  char *sh2= "/usr/bin/sh";	/* other possibility */

  /* Reset some of the line parameters in case they have been mashed. */
  if (ioctl(0, TIOCGETP, &args) < 0) exit(1);
#ifdef NOGETTY
  args.sg_kill = '\030';	/* CTRL-X */
  args.sg_erase = '\b';
  args.sg_flags |= (XTABS | CRMOD | ECHO);
  ioctl(0, TIOCSETP, &args);
#endif /* NOGETTY */

  /* Look up /dev/tty number. */
  fstat(0, &statbuf);
  ttynr = statbuf.st_rdev & 0377;
  if (ttynr == 0)
	strcpy(ttyname, CONS);	/* system console */
  else {
	strcpy(ttyname, TTY);
	strcat(ttyname, itoa(ttynr));
  }

  /* Get login name and passwd. */
  for (;;) {
	bad = 0;

	if (argc > 1) {
		strcpy(name, argv[1]);
		argc = 1;
	} else {
		/* Sync the disk so that one can cut the power at logout. */
		sync();
		do {
			say("login: ");
			n = read(0, name, 30);
		} while (n < 2);
		name[n - 1] = 0;
	}

	/* Look up login/passwd. */
	if ((pwd = getpwnam(name)) == NULL) bad++;

	/* If login name wrong or password exists, ask for pw. */
	if (bad || strlen(pwd->pw_passwd) != 0) {
		args.sg_flags &= ~ECHO;
		ioctl(0, TIOCSETP, &args);
		say("Password: ");

		time_out = 0;
		signal(SIGALRM, Time_out);
		alarm(30);

		n = read(0, password, 30);

		alarm(0);
		if (time_out) {
			n = 1;
			bad++;
		}
		password[n - 1] = 0;
		say("\n");
		args.sg_flags |= ECHO;
		ioctl(0, TIOCSETP, &args);

		if ((bad && crypt(password, "aaaa")) ||
		    strcmp(pwd->pw_passwd, crypt(password, pwd->pw_passwd))) {
#ifdef BADLOG
			addlog(name, password, ttyname);
#endif /* BADLOG */
			say("Login incorrect\n");
			continue;
		}
	}
	/* Check if the system is going down  */
	if (access("/etc/nologin", 0) == 0 && strcmp(name, "root") != 0) {
		say("System going down\n\n");
		continue;
	}
	/* Write login record to /usr/adm/wtmp and /etc/utmp */
	wtmp(ttyname, name);

	/* Create the argv[] array from the pw_shell field. */
	ap = 1;
	if (pwd->pw_shell[0]) {
		sh = pwd->pw_shell;
		bp = sh;
		while (*bp) {
			while (*bp && *bp != ' ' && *bp != '\t') bp++;
			if (*bp == ' ' || *bp == '\t') {
				*bp++ = '\0';	/* mark end of string */
				argx[ap++] = bp;
			}
		}
	} else
	argx[ap] = NULL;

	/* Set the environment */
	strcpy(user, "USER=");
	strcat(user, name);
	strcpy(logname, "LOGNAME=");
	strcat(logname, name);
	strcpy(home, "HOME=");
	strcat(home, pwd->pw_dir);
	strcpy(shell, "SHELL=");
	strcat(shell, sh);

	chdir(pwd->pw_dir);

	/* Reset signals to default values. */
	for (n = 1; n <= _NSIG; ++n) signal(n, SIG_DFL);

	/* Show the message-of-the-day. */
	show_file(MOTD);

	/* Assign the terminal to this user. */
	strcpy(name, "/dev/");
	strcat(name, ttyname);
	chown(name, pwd->pw_uid, pwd->pw_gid);

	setgid(pwd->pw_gid);
	setuid(pwd->pw_uid);

	/* Most shells need argv[0] to begin with "-" for a login shell.
	 * Follow that with the name of the shell for ps.
	 */
	argx[0] = minus_shell;
	minus_shell[0] = '-';
	strcpy(minus_shell + 1, sh);
	execve(sh, argx, env);

	/* Normal shell is absent, try /usr/bin/sh. */
	strcpy(minus_shell + 1, sh2);
	execve(sh2, argx, env);

	say("login: cannot exec ");
	say(sh);
	say(" or ");
	say(sh2);
	say("\n");
	exit(1);
  }
  return(0);
}


void Time_out(dummy)
int dummy;			/* to satisfy the prototype */
{
   time_out = 1;
}
