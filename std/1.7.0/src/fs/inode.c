/* This file manages the inode table.  There are procedures to allocate and
 * deallocate inodes, acquire, erase, and release them, and read and write
 * them from the disk.
 *
 * The entry points into this file are
 *   get_inode:	   search inode table for a given inode; if not there, read it
 *   put_inode:	   indicate that an inode is no longer needed in memory
 *   alloc_inode:  allocate a new, unused inode
 *   wipe_inode:   erase some fields of a newly allocated inode
 *   free_inode:   mark an inode as available for a new file
 *   update_times: update atime, ctime, and mtime
 *   rw_inode:	   read a disk block and extract an inode, or corresp. write
 *   old_icopy:	   copy to/from in-core inode struct and disk inode (V1.x)
 *   new_icopy:	   copy to/from in-core inode struct and disk inode (V2.x)
 *   dup_inode:	   indicate that someone else is using an inode table entry
 */

#include "fs.h"
#include <minix/boot.h>
#include "buf.h"
#include "file.h"
#include "fproc.h"
#include "inode.h"
#include "super.h"

FORWARD _PROTOTYPE( void old_icopy, (struct inode *rip, d1_inode *dip,
						int direction, int norm));
FORWARD _PROTOTYPE( void new_icopy, (struct inode *rip, d2_inode *dip,
						int direction, int norm));


/*===========================================================================*
 *				get_inode				     *
 *===========================================================================*/
PUBLIC struct inode *get_inode(dev, numb)
dev_t dev;			/* device on which inode resides */
int numb;			/* inode number (ANSI: may not be unshort) */
{
/* Find a slot in the inode table, load the specified inode into it, and
 * return a pointer to the slot.  If 'dev' == NO_DEV, just return a free slot.
 */

  register struct inode *rip, *xp;

  /* Search the inode table both for (dev, numb) and a free slot. */
  xp = NIL_INODE;
  for (rip = &inode[0]; rip < &inode[NR_INODES]; rip++) {
	if (rip->i_count > 0) { /* only check used slots for (dev, numb) */
		if (rip->i_dev == dev && rip->i_num == numb) {
			/* This is the inode that we are looking for. */
			rip->i_count++;
			return(rip);	/* (dev, numb) found */
		}
	} else {
		xp = rip;	/* remember this free slot for later */
	}
  }

  /* Inode we want is not currently in use.  Did we find a free slot? */
  if (xp == NIL_INODE) {	/* inode table completely full */
	err_code = ENFILE;
	return(NIL_INODE);
  }

  /* A free inode slot has been located.  Load the inode into it. */
  xp->i_dev = dev;
  xp->i_num = numb;
  xp->i_count = 1;
  if (dev != NO_DEV) rw_inode(xp, READING);	/* get inode from disk */
  xp->i_update = 0;		/* all the times are initially up-to-date */

  return(xp);
}


/*===========================================================================*
 *				put_inode				     *
 *===========================================================================*/
PUBLIC void put_inode(rip)
register struct inode *rip;	/* pointer to inode to be released */
{
/* The caller is no longer using this inode.  If no one else is using it either
 * write it back to the disk immediately.  If it has no links, truncate it and
 * return it to the pool of available inodes.
 */

  int inum;

  if (rip == NIL_INODE) return;	/* checking here is easier than in caller */
  if (--rip->i_count == 0) {	/* i_count == 0 means no one is using it now */
	if ((rip->i_nlinks & BYTE) == 0) {
		/* i_nlinks == 0 means free the inode. */
		truncate(rip);	/* return all the disk blocks */
		rip->i_mode = I_NOT_ALLOC;	/* clear I_TYPE field */
		rip->i_dirt = DIRTY;
		inum = (int) rip->i_num;	/* do not pass an unshort */
		free_inode(rip->i_dev, inum);
	} else {
		if (rip->i_pipe == I_PIPE) truncate(rip);
	}
	rip->i_pipe = NO_PIPE;  /* should always be cleared */
	if (rip->i_dirt == DIRTY) rw_inode(rip, WRITING);
  }
}

/*===========================================================================*
 *				alloc_inode				     *
 *===========================================================================*/
PUBLIC struct inode *alloc_inode(dev, bits)
dev_t dev;			/* device on which to allocate the inode */
mode_t bits;			/* mode of the inode */
{
/* Allocate a free inode on 'dev', and return a pointer to it. */

  register struct inode *rip;
  register struct super_block *sp;
  int major, minor, inumb;
  bit_t b, bit;

  sp = get_super(dev);	/* get pointer to super_block */
  if (sp->s_rd_only) {	/* can't allocate an inode on a read only device. */
	err_code = EROFS;
	return(NIL_INODE);
  }

  /* Acquire an inode from the bit map. */
  bit = sp->s_isearch;		/* start at first unused inode */
  b = alloc_bit(sp->s_imap, (bit_t)sp->s_ninodes+1, sp->s_imap_blocks, bit);
  if (b == NO_BIT) {
	err_code = ENFILE;
	major = (int) (sp->s_dev >> MAJOR) & BYTE;
	minor = (int) (sp->s_dev >> MINOR) & BYTE;
	printf("Out of i-nodes on %sdevice %d/%d\n",
		sp->s_dev == ROOT_DEV ? "root " : "", major, minor);
	return(NIL_INODE);
  }
  sp->s_isearch = b;		/* next time start here */
  inumb = (int) b;		/* be careful not to pass unshort as param */

  /* Try to acquire a slot in the inode table. */
  if ( (rip = get_inode(NO_DEV, inumb)) == NIL_INODE) {
	/* No inode table slots available.  Free the inode just allocated. */
	free_bit(sp->s_imap, b);
  } else {
	/* An inode slot is available. Put the inode just allocated into it. */
	rip->i_mode = bits;		/* set up RWX bits */
	rip->i_nlinks = (nlink_t) 0;	/* initial no links */
	rip->i_uid = fp->fp_effuid;	/* file's uid is owner's */
	rip->i_gid = fp->fp_effgid;	/* ditto group id */
	rip->i_dev = dev;		/* mark which device it is on */
	rip->i_ndzones = sp->s_ndzones;	/* number of direct zones */
	rip->i_nindirs = sp->s_nindirs;	/* number of indirect zones per blk*/
	rip->i_sp = sp;			/* pointer to super block */

	/* Fields not cleared already are cleared in wipe_inode().  They have
	 * been put there because truncate() needs to clear the same fields if
	 * the file happens to be open while being truncated.  It saves space
	 * not to repeat the code twice.
	 */
	wipe_inode(rip);
  }

  return(rip);
}

/*===========================================================================*
 *				wipe_inode				     *
 *===========================================================================*/
PUBLIC void wipe_inode(rip)
register struct inode *rip;	/* the inode to be erased */
{
/* Erase some fields in the inode.  This function is called from alloc_inode()
 * when a new inode is to be allocated, and from truncate(), when an existing
 * inode is to be truncated.
 */

  register int i;

  rip->i_size = 0;
  rip->i_update = ATIME | CTIME | MTIME;	/* update all times later */
  rip->i_dirt = DIRTY;
  for (i = 0; i < V2_NR_TZONES; i++) rip->i_zone[i] = NO_ZONE;
}


/*===========================================================================*
 *				free_inode				     *
 *===========================================================================*/
PUBLIC void free_inode(dev, inumb)
dev_t dev;			/* on which device is the inode */
int inumb;			/* number of inode to be freed */
{
/* Return an inode to the pool of unallocated inodes. */

  register struct super_block *sp;
  bit_t b;

  /* Locate the appropriate super_block. */
  sp = get_super(dev);
  b = (bit_t) ((ino_t) inumb);	/* needed to avoid sign extension */
  if (b <= 0 || b > sp->s_ninodes) return;
  free_bit(sp->s_imap, b);
  if (inumb < sp->s_isearch) sp->s_isearch = (bit_t) ((ino_t) inumb);
}

/*===========================================================================*
 *				update_times				     *
 *===========================================================================*/
PUBLIC void update_times(rip)
register struct inode *rip;	/* pointer to inode to be read/written */
{
/* Various system calls are required by the standard to update atime, ctime,
 * or mtime.  Since updating a time requires sending a message to the clock
 * task--an expensive business--the times are marked for update by setting
 * bits in i_update.  When a stat, fstat, or sync is done, or an inode is 
 * released, update_times() may be called to actually fill in the times.
 */

  time_t cur_time;
  struct super_block *sp;

  sp = rip->i_sp;		/* get pointer to super block. */
  if (sp->s_rd_only) return;	/* no updates for read-only file systems */

  cur_time = clock_time();
  if (rip->i_update & ATIME) rip->i_atime = cur_time;
  if (rip->i_update & CTIME) rip->i_ctime = cur_time;
  if (rip->i_update & MTIME) rip->i_mtime = cur_time;
  rip->i_update = 0;		/* they are all up-to-date now */
}


/*=============================