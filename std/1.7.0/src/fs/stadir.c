/* This file contains the code for performing four system calls relating to
 * status and directories.
 *
 * The entry points into this file are
 *   do_chdir:	perform the CHDIR system call
 *   do_chroot:	perform the CHROOT system call
 *   do_stat:	perform the STAT system call
 *   do_fstat:	perform the FSTAT system call
 */

#include "fs.h"
#include <sys/stat.h>
#include "file.h"
#include "fproc.h"
#include "inode.h"
#include "param.h"

FORWARD _PROTOTYPE( int change, (struct inode **iip, char *name_ptr, int len));
FORWARD _PROTOTYPE( int stat_inode, (struct inode *rip, struct filp *fil_ptr,
			char *user_addr)				);

/*===========================================================================*
 *				do_chdir				     *
 *===========================================================================*/
PUBLIC int do_chdir()
{
/* Change directory.  This function is  also called by MM to simulate a chdir
 * in order to do EXEC, etc.  It also changes the root directory, the uids and
 * gids, and the umask.  The name of this function is now misleading and the
 * cd_flag argument is no longer supported.
 */

  int r;
  register struct fproc *rfp;

  if (who == MM_PROC_NR) {
	rfp = &fproc[slot1];
	put_inode(fp->fp_rootdir);
	dup_inode(fp->fp_rootdir = rfp->fp_rootdir);
	put_inode(fp->fp_workdir);
	dup_inode(fp->fp_workdir = rfp->fp_workdir);

	/* MM uses access() to check permissions.  To make this work, pretend
	 * that the user's real ids are the same as the user's effective ids.
	 * FS calls other than access() do not use the real ids, so are not
	 * affected.
	 */
	fp->fp_realuid =
	fp->fp_effuid = rfp->fp_effuid;
	fp->fp_realgid =
	fp->fp_effgid = rfp->fp_effgid;
	fp->fp_umask = rfp->fp_umask;
	return(OK);
  }

  /* Perform the chdir(name) system call. */
  r = change(&fp->fp_workdir, name, name_length);
  return(r);
}


/*===========================================================================*
 *				do_chroot				     *
 *===========================================================================*/
PUBLIC int do_chroot()
{
/* Perform the chroot(name) system call. */

  register int r;

  if (!super_user) return(EPERM);	/* only su may chroot() */
  r = change(&fp->fp_rootdir, name, name_length);
  return(r);
}


/*===========================================================================*
 *				change					     *
 *===========================================================================*/
PRIVATE int change(iip, name_ptr, len)
struct inode **iip;		/* pointer to the inode pointer for the dir */
char *name_ptr;			/* pointer to the directory name to change to */
int len;			/* length of the directory name string */
{
/* Do the actual work for chdir() and chroot(). */

  struct inode *rip;
  register int r;

  /* Try to open the new directory. */
  if (fetch_name(name_ptr, len, M3) != OK) return(err_code);
  if ( (rip = eat_path(user_path)) == NIL_INODE) return(err_code);

  /* It must be a directory and also be searchable. */
  if ( (rip->i_mode & I_TYPE) != I_DIRECTORY)
	r = ENOTDIR;
  else
	r = forbidden(rip, X_BIT, 0);	/* check if dir is searchable */

  /* If error, return inode. */
  if (r != OK) {
	put_inode(rip);
	return(r);
  }

  /* Everything is OK.  Make the change. */
  put_inode(*iip);		/* release the old directory */
  *iip = rip;			/* acquire the new one */
  return(OK);
}


/*===========================================================================*
 *				do_stat					     *
 *===========================================================================*/
PUBLIC int do_stat()
{
/* Perform the stat(name, buf) system call. */

  register struct inode *rip;
  register int r;

  /* Both stat() and fstat() use the same routine to do the real work.  That
   * routine expects an inode, so acquire it temporarily.
   */
  if (fetch_name(name1, name1_length, M1) != OK) return(err_code);
  if ( (rip = eat_path(user_path)) == NIL_INODE) return(err_code);
  r = stat_inode(rip, NIL_FILP, name2);	/* actually do the work.*/
  put_inode(rip);		/* release the inode */
  return(r);
}


/*===========================================================================*
 *				do_fstat				     *
 *===========================================================================*/
PUBLIC int do_fstat()
{
/* Perform the fstat(fd, buf) system call. */

  register struct filp *rfilp;

  /* Is the file descriptor valid? */
  if ( (rfilp = get_filp(fd)) == NIL_FILP) return(err_code);

  return(stat_inode(rfilp->filp_ino, rfilp, buffer));
}


/*===========================================================================*
 *				stat_inode				     *
 *===========================================================================*/
PRIVATE int stat_inode(rip, fil_ptr, user_addr)
register struct inode *rip;	/* pointer to inode to stat */
struct filp *fil_ptr;		/* filp pointer, supplied by 'fstat' */
char *user_addr;		/* user space address where stat buf goes */
{
/* Common code for stat and fstat system calls. */

  register struct stat *stp;
  struct stat statbuf;
  mode_t mo;
  int r, s;
  vir_bytes v;

  /* Update the atime, ctime, and mtime fields in the inode, if need be. */
  if (rip->i_update) update_times(rip);

  /* Fill in the statbuf struct. */
  mo = rip->i_mode & I_TYPE;
  s = (mo == I_CHAR_SPECIAL || mo == I_BLOCK_SPECIAL);	/* true iff special */
  stp = &statbuf;		/* set up pointer to the buffer */
  stp->st_dev = rip->i_dev;
  stp->st_ino = rip->i_num;
  stp->st_mode = rip->i_mode;
  stp->st_nlink = rip->i_nlinks & BYTE;
  stp->st_uid = rip->i_uid;
  stp->st_gid = rip->i_gid & BYTE;
  stp->st_rdev = (dev_t) (s ? rip->i_zone[0] : NO_DEV);
  stp->st_size = rip->i_size;

  if (rip->i_pipe == I_PIPE) {
	stp->st_mode &= ~I_REGULAR;	/* wipe out I_REGULAR bit for pipes */
	if (fil_ptr != NIL_FILP && fil_ptr->filp_mode & R_BIT) 
		stp->st_size -= fil_ptr->filp_pos;
  }

  stp->st_atime = rip->i_atime;
  stp->st_mtime = rip->i_mtime;
  stp->st_ctime = rip->i_ctime;

  /* Copy the struct to user space. */
  v = (vir_bytes) user_addr;
  r = rw_user(D, who, v, (vir_bytes) sizeof statbuf, (char *) stp, TO_USER);
  return(r);
}
