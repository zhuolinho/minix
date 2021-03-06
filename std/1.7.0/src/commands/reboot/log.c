/*
  log - log the shutdown's and the halt's

  Author: Edvard Tuinder  <v892231@si.hhs.NL>

  shutdown is logged in /usr/adm/wtmp and in /usr/adm/authlog (if desired)
  halt is logged only in /usr/adm/wtmp as `halt' to prevent last from
       reporting halt's as crashes.

 */

#define _POSIX_SOURCE	1
#include <sys/types.h>
#include <stdio.h>
#include <utmp.h>
#include <pwd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#undef WTMP

static char WTMP[] = "/usr/adm/wtmp";	/* Record of logins and logouts. */
static char SHUT_LOG[] = "/usr/adm/authlog";

char who[8];
extern char *prog;
static char *month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
			 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void write_log _ARGS(( void ));

void write_log()
{
  int fd;
  static struct utmp wtmp;
  static struct passwd *pwd;
  char mes[90];
  struct tm *tm;
  time_t now;
  struct utsname utsname;
  char *host = "localhost";

  time(&now);
  tm = localtime(&now);

  if (uname(&utsname) >= 0) host = utsname.nodename;

  pwd = getpwuid(getuid());
  if (pwd == (struct passwd *)0)
    strcpy (who,"root");
  else
    strcpy (who,pwd->pw_name);
  fd = open(WTMP,O_APPEND|O_WRONLY,1);
  if (fd) {
    if (strcmp(prog,"reboot"))
      strcpy (wtmp.ut_user, prog);
    else
      strcpy (wtmp.ut_user, "shutdown"); /* last ... */
    strcpy (wtmp.ut_id, "~~");
    strcpy (wtmp.ut_line, "~");
    wtmp.ut_pid = 0;
    wtmp.ut_type = BOOT_TIME;
    wtmp.ut_time = now;
    wtmp.ut_host[0]= '\0';
    write (fd, (char *) &wtmp,sizeof(struct utmp));
    close(fd);
  }
  fd = open(SHUT_LOG,O_APPEND|O_WRONLY,1);
  if (!fd) 
    perror ("open");
  else {
    sprintf (mes,"System %s at %.02d:%.02d %s %d 19%.02d by %s@%s\n",prog,
	     tm->tm_hour,tm->tm_min,month[tm->tm_mon],tm->tm_mday,tm->tm_year,
	     who,host);
    write (fd,mes,strlen(mes));
    close(fd);
  }
  return;
}
