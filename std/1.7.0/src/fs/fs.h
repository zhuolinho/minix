/* This is the master header for fs.  It includes some other files
 * and defines the principal constants.
 */
#define _POSIX_SOURCE      1	/* tell headers to include POSIX stuff */
#define _MINIX             1	/* tell headers to include MINIX stuff */
#define _SYSTEM            1	/* tell headers that this is the kernel */

/* The ANSI C namespace pollution rules forbid the use of sendrec etc. */
#define send _send
#define receive _receive
#define sendrec _sendrec

/* The following are so basic, all the *.c files get them automatically. */
#include <minix/config.h>	/* MUST be first */
#include <ansi.h>		/* MUST be second */
#include <sys/types.h>
#include <minix/const.h>
#include <minix/type.h>

#include <limits.h>
#include <errno.h>

#include <minix/syslib.h>

#include "const.h"
#include "type.h"
#include "proto.h"
#include "glo.h"
