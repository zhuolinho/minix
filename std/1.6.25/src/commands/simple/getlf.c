/* getlf - get a line feed		Author: Andy Tanenbaum */

#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <minix/minlib.h>

_PROTOTYPE(int main, (int argc, char **argv));

int main(argc, argv)
int argc;
char *argv[];
{
  char c;

  /* Echo argument, if present. */
  if (argc == 2) {
	std_err(argv[1]);
	std_err("\n");
  }

  close(0);
  open("/dev/tty", O_RDONLY);

  do {
	if (read(0, &c, 1) <= 0) exit(1);
  } while (c != '\n');
  return(0);
}
