/* umount - unmount a file system		Author: Andy Tanenbaum */

#define _MINIX 1		/* for proto of the non-POSIX umount() */
#define _POSIX_SOURCE 1		/* for PATH_MAX from limits.h */

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <minix/minlib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

_PROTOTYPE(int main, (int argc, char **argv));
_PROTOTYPE(void update_mtab, (char *devname));
_PROTOTYPE(void usage, (void));

int main(argc, argv)
int argc;
char *argv[];
{

  if (argc != 2) usage();
  if (umount(argv[1]) < 0) {
	if (errno == EINVAL)
		std_err("Device not mounted\n");
	else
		perror("umount");
	exit(1);
  }
  std_err(argv[1]);
  std_err(" unmounted\n");
  update_mtab(argv[1]);
  return(0);
}

void update_mtab(devname)
char *devname;
{
/* Remove an entry from /etc/mtab. */
  int n;
  char special[PATH_MAX+1], mounted_on[PATH_MAX+1], version[10], rw_flag[10];

  if (load_mtab("umount") < 0) {
	std_err("/etc/mtab not updated.\n");
	exit(1);
  }
  while (1) {
	n = get_mtab_entry(special, mounted_on, version, rw_flag);
	if (n < 0) break;
	if (strcmp(devname, special) == 0) continue;
	(void) put_mtab_entry(special, mounted_on, version, rw_flag);
  }
  n = rewrite_mtab("umount");
  if (n < 0) {
	std_err("/etc/mtab not updated.\n");
	exit(1);
  }
}

void usage()
{
  std_err("Usage: umount special\n");
  exit(1);
}
