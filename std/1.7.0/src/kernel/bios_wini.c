/* This file contains the "device dependent" part of a hard disk driver that
 * uses the ROM BIOS.  It makes a call and just waits for the transfer to
 * happen.  It is not interrupt driven and thus will (*) have poor performance.
 * The advantage is that it should work on virtually any PC, XT, 386, PS/2
 * or clone.  The demo disk uses this driver.  It is suggested that all
 * MINIX users try the other drivers, and use this one only as a last resort,
 * if all else fails.
 *
 * (*) The performance is within 10% of the AT driver for reads on any disk
 *     and writes on a 2:1 interleaved disk, it will be DMA_BUF_SIZE bytes
 *     per revolution for a minimum of 60 kb/s for writes to 1:1 disks.
 *
 * The file contains one entry point:
 *
 *	 bios_winchester_task:	main entry when system is brought up
 *
 *
 * Changes:
 *	30 Apr 1992 by Kees J. Bot: device dependent/independent split.
 */

#include "kernel.h"
#include "driver.h"

#if ENABLE_BIOS_WINI

/* If the DMA buffer is large enough then use it always. */
#define USE_BUF		(DMA_BUF_SIZE > BLOCK_SIZE)

/* Error codes */
#define ERR		 (-1)	/* general error */

/* Parameters for the disk drive. */
#define MAX_DRIVES         2	/* this driver supports 2 drives (hd0 - hd9)*/
#define MAX_SECS	 255	/* bios can transfer this many sectors */
#define NR_DEVICES      (MAX_DRIVES * DEV_PER_DRIVE)
#define SUB_PER_DRIVE	(NR_PARTITIONS * NR_PARTITIONS)
#define NR_SUBDEVS	(MAX_DRIVES * SUB_PER_DRIVE)

/* BIOS parameters */
#define BIOS_ASK        0x08	/* opcode for asking BIOS for parameters */
#define BIOS_RESET      0x00	/* opcode for resetting disk BIOS */
#define BIOS_READ       0x02	/* opcode for BIOS read */
#define BIOS_WRITE      0x03	/* opcode for BIOS write */
#define HD_CODE         0x80	/* BIOS code for hard disk drive 0 */

/* Variables. */
PRIVATE struct wini {		/* main drive struct, one entry per drive */
  unsigned cylinders;		/* number of cylinders */
  unsigned heads;		/* number of heads */
  unsigned sectors;		/* number of sectors per track */
  unsigned open_ct;		/* in-use count */
  struct device part[DEV_PER_DRIVE];    /* primary partitions: hd[0-4] */
  struct device subpart[SUB_PER_DRIVE]; /* subpartititions: hd[1-4][a-d] */
} wini[MAX_DRIVES], *w_wn;

PRIVATE struct trans {
  struct iorequest_s *iop;	/* belongs to this I/O request */
  unsigned long block;		/* first sector to transfer */
  unsigned count;		/* byte count */
  phys_bytes phys;		/* user physical address */
  phys_bytes dma;		/* DMA physical address */
} wtrans[NR_IOREQS];

PRIVATE int nr_drives = MAX_DRIVES;	/* Number of drives */
PRIVATE struct trans *w_tp;		/* to add transfer requests */
PRIVATE unsigned w_count;		/* number of bytes to transfer */
PRIVATE unsigned long w_nextblock;	/* next block on disk to transfer */
PRIVATE int w_opcode;			/* DEV_READ or DEV_WRITE */
PRIVATE int w_drive;			/* selected drive */
PRIVATE struct device *w_dv;		/* device's base and size */

FORWARD _PROTOTYPE( struct device *w_prepare, (int device) );
FORWARD _PROTOTYPE( char *w_name, (void) );
FORWARD _PROTOTYPE( int w_schedule, (int proc_nr, struct iorequest_s *iop) );
FORWARD _PROTOTYPE( int w_finish, (void) );
FORWARD _PROTOTYPE( int w_do_open, (struct driver *dp, message *m_ptr) );
FORWARD _PROTOTYPE( int w_do_close, (struct driver *dp, message *m_ptr) );
FORWARD _PROTOTYPE( void w_init, (void) );
FORWARD _PROTOTYPE( void replace, (int from, int to) );
FORWARD _PROTOTYPE( void w_geometry, (unsigned *chs));


/* Entry points to this driver. */
PRIVATE struct driver w_dtab = {
  w_name,	/* current device's name */
  w_do_open,	/* open or mount request, initialize device */
  w_do_close,	/* release device */
  do_diocntl,	/* get or set a partition's geometry */
  w_prepare,	/* prepare for I/O on a given minor device */
  w_schedule,	/* precompute cylinder, head, sector, etc. */
  w_finish,	/* do the I/O */
  nop_cleanup,	/* no cleanup needed */
  w_geometry	/* tell the geometry of the disk */
};


/*===========================================================================*
 *				bios_winchester_task			     *
 *===========================================================================*/
PUBLIC void bios_winchester_task()
{
  driver_task(&w_dtab);
}


/*===========================================================================*
 *				w_prepare				     *
 *===========================================================================*/
PRIVATE struct device *w_prepare(device)
int device;
{
/* Prepare for I/O on a device. */

  /* Nothing to transfer as yet. */
  w_count = 0;

  if (device < NR_DEVICES) {			/* hd0, hd1, ... */
	w_drive = device / DEV_PER_DRIVE;	/* save drive number */
	w_wn = &wini[w_drive];
	w_dv = &w_wn->part[device % DEV_PER_DRIVE];
  } else
  if ((unsigned) (device -= MINOR_hd1a) < NR_SUBDEVS) {	/* hd1a, hd1b, ... */
	w_drive = device / SUB_PER_DRIVE;
	w_wn = &wini[w_drive];
	w_dv = &w_wn->subpart[device % SUB_PER_DRIVE];
  } else
	return(NIL_DEV);

  return(w_drive < nr_drives ? w_dv : NIL_DEV);
}


/*===========================================================================*
 *				w_name					     *
 *===========================================================================*/
PRIVATE char *w_name()
{
/* Return a name for the current device. */
  static char name[] = "bios-hd5";

  name[7] = '0' + w_drive * DEV_PER_DRIVE;
  return name;
}


/*===========================================================================*
 *				w_schedule				     *
 *===========================================================================*/
PRIVATE int w_schedule(proc_nr, iop)
int proc_nr;			/* process doing the request */
struct iorequest_s *iop;	/* pointer to read or write request */
{
/* Gather I/O requests on consecutive blocks so they may be read/written
 * in one bios command if using a buffer.  Check and gather all the requests
 * and try to finish them as fast as possible if unbuffered.
 */
  int r, opcode;
  unsigned long pos;
  unsigned nbytes, count, dma_count;
  unsigned long block;
  phys_bytes user_phys, dma_phys;

  /* This many bytes to read/write */
  nbytes = iop->io_nbytes;
  if ((nbytes & SECTOR_MASK) != 0) return(iop->io_nbytes = EINVAL);

  /* From/to this position on the device */
  pos = iop->io_position;
  if ((pos & SECTOR_MASK) != 0) return(iop->io_nbytes = EINVAL);

  /* To/from this user address */
  user_phys = numap(proc_nr, (vir_bytes) iop->io_buf, nbytes);
  if (user_phys == 0) return(iop->io_nbytes = EINVAL);

  /* Read or write? */
  opcode = iop->io_request & ~OPTIONAL_IO;

  /* Which block on disk and how close to EOF? */
  if (pos >= w_dv->dv_size) return(OK);		/* At EOF */
  if (pos + nbytes > w_dv->dv_size) nbytes = w_dv->dv_size - pos;
  block = (w_dv->dv_base + pos) >> SECTOR_SHIFT;

  if (USE_BUF && w_count > 0 && block != w_nextblock) {
	/* This new request can't be chained to the job being built */
	if ((r = w_finish()) != OK) return(r);
  }

  /* The next consecutive block */
  if (USE_BUF) w_nextblock = block + (nbytes >> SECTOR_SHIFT);

  /* While there are "unscheduled" bytes in the request: */
  do {
	count = nbytes;

	if (USE_BUF) {
		if (w_count == DMA_BUF_SIZE) {
			/* Can't transfer more than the buffer allows. */
			if ((r = w_finish()) != OK) return(r);
		}

		if (w_count + count > DMA_BUF_SIZE)
			count = DMA_BUF_SIZE - w_count;
	} else {
		if (w_tp == wtrans + NR_IOREQS) {
			/* All transfer slots in use. */
			if ((r = w_finish()) != OK) return(r);
		}
	}

	if (w_count == 0) {
		/* The first request in a row, initialize. */
		w_opcode = opcode;
		w_tp = wtrans;
	}

	if (USE_BUF) {
		dma_phys = tmp_phys + w_count;
	} else {
		/* Memory chunk to DMA. */
		dma_phys = user_phys;
		dma_count = dma_bytes_left(dma_phys);

		if (dma_count < count) {
			/* Nearing a 64K boundary. */
			if (dma_count >= SECTOR_SIZE) {
				/* Can read a few sectors before hitting the
				 * boundary.
				 */
				count = dma_count & ~SECTOR_MASK;
			} else {
				/* Must use the special buffer for this. */
				count = SECTOR_SIZE;
				dma_phys = tmp_phys;
			}
		}
	}

	/* Store I/O parameters */
	w_tp->iop = iop;
	w_tp->block = block;
	w_tp->count = count;
	w_tp->phys = user_phys;
	w_tp->dma = dma_phys;

	/* Update counters */
	w_tp++;
	w_count += count;
	block += count >> SECTOR_SHIFT;
	user_phys += count;
	nbytes -= count;
  } while (nbytes > 0);

  return(OK);
}


/*===========================================================================*
 *				w_finish				     *
 *===========================================================================*/
PRIVATE int w_finish()
{
/* Carry out the I/O requests gathered in wtrans[]. */

  struct trans *tp = wtrans, *tp2;
  unsigned count, cylinder, sector, head, hicyl, locyl;
  unsigned secspcyl = w_wn->heads * w_wn->sectors;
  int many = USE_BUF;

  if (w_count == 0) return(OK);	/* Spurious finish. */

  do {
	if (w_opcode == DEV_WRITE) {
		tp2 = tp;
		count = 0;
		do {
			if (USE_BUF || tp2->dma == tmp_phys) {
				phys_copy(tp2->phys, tp2->dma,
						(phys_bytes) tp2->count);
			}
			count += tp2->count;
			tp2++;
		} while (many && count < w_count);
	} else {
		count = many ? w_count : tp->count;
	}

	/* Do the transfer */
	cylinder = tp->block / secspcyl;
	sector = (tp->block % w_wn->sectors) + 1;
	head = (tp->block % secspcyl) / w_wn->sectors;

	Ax = w_opcode == DEV_WRITE ? BIOS_WRITE : BIOS_READ;
	Ax = (Ax << 8) | (count >> SECTOR_SHIFT);	/* opcode & count */
	Bx = (unsigned) tp->dma % HCLICK_SIZE;		/* low order 4 bits */
	Es = physb_to_hclick(tp->dma);		/* high order 16 bits */
	hicyl = (cylinder & 0x300) >> 8;	/* two high-order bits */
	locyl = (cylinder & 0xFF);		/* 8 low-order bits */
	Cx = sector | (hicyl << 6) | (locyl << 8);
	Dx = (HD_CODE + w_drive) | (head << 8);
	level0(bios13);
	if ((Ax & 0xFF00) != 0) {
		/* An error occurred, try again block by block unless */
		if (!many) return(tp->iop->io_nbytes = EIO);
		many = 0;
		continue;
	}

	w_count -= count;

	do {
		if (w_opcode == DEV_READ) {
			if (USE_BUF || tp->dma == tmp_phys) {
				phys_copy(tp->dma, tp->phys,
						(phys_bytes) tp->count);
			}
		}
		tp->iop->io_nbytes -= tp->count;
		count -= tp->count;
		tp++;
	} while (count > 0);
  } while (w_count > 0);

  return(OK);
}


/*============================================================================*
 *				w_do_open				      *
 *============================================================================*/
PRIVATE int w_do_open(dp, m_ptr)
struct driver *dp;
message *m_ptr;
{
/* Device open: Initialize the controller and read the partition table. */

  static int init_done = FALSE;

  if (!init_done) { w_init(); init_done = TRUE; }

  if (w_prepare(m_ptr->DEVICE) == NIL_DEV) return(ENXIO);

  if (w_wn->open_ct++ == 0) {
	/* Partition the disk. */
	partition(&w_dtab, w_drive * DEV_PER_DRIVE, P_PRIMARY);
  }
  return(OK);
}


/*============================================================================*
 *				w_do_close				      *
 *============================================================================*/
PRIVATE int w_do_close(dp, m_ptr)
struct driver *dp;
message *m_ptr;
{
/* Device close: Release a device. */

  if (w_prepare(m_ptr->DEVICE) == NIL_DEV) return(ENXIO);
  w_wn->open_ct--;
  return(OK);
}


/*===========================================================================*
 *				w_init					     *
 *===========================================================================*/
PRIVATE void w_init()
{
/* This routine is called at startup to initialize the drive parameters. */

  int drive;
  struct wini *wn;
  int irq;

  /* Give control to the BIOS interrupt handler. */
  irq = pc_at ? AT_WINI_IRQ : XT_WINI_IRQ;

  replace(BIOS_VECTOR(irq), VECTOR(irq));
  enable_irq(irq);	/* ready for winchester interrupts */

  /* Set the geometry of the drives */
  for (drive = 0, wn = wini; drive < nr_drives; drive++, wn++) {
	Dx = drive + HD_CODE;
	Ax = (BIOS_ASK << 8);
	level0(bios13);
	nr_drives = (Ax & 0xFF00) == 0 ? (Dx & 0xFF) : drive;
	if (drive >= nr_drives) break;

	wn->heads = (Dx >> 8) + 1;
	wn->sectors = (Cx & 0x3F);
	wn->cylinders = ((Cx << 2) & 0x0300) + ((Cx >> 8) & 0x00FF) + 1;

	wn->part[0].dv_size = ((unsigned long) wn->cylinders
			* wn->heads * wn->sectors) << SECTOR_SHIFT;

	printf("%s: %d cylinders, %d heads, %d sectors per track\n",
		w_name(), wn->cylinders, wn->heads, wn->sectors);
  }
}


/*===========================================================================*
 *				replace					     *
 *===========================================================================*/
PRIVATE void replace(from, to)
int from;			/* vector to get replacement from */
int to;				/* vector to replace */
{
/* Replace the vector 'to' in the interrupt table with its original BIOS
 * vector 'from' in vec_table (they differ since the 8259 was reprogrammed).
 * On the first call only, also restore all software interrupt vectors from
 * vec_table except the trap vectors and SYS_VECTOR.
 * (Doing it only on the first call is redundant, since this is only called
 * once! We ought to swap our vectors back just before bios_wini replies.
 * Then this routine should be made more efficient.)
 */

  phys_bytes phys_b;
  static int repl_called = FALSE;	/* set on first call of replace */
  int vec;

  phys_b = vir2phys(vec_table);
  if (!repl_called) {
	for (vec = 16; vec < VECTOR_BYTES / 4; ++vec)
		if (vec != SYS_VECTOR &&
		    !(vec >= IRQ0_VECTOR && vec < IRQ0_VECTOR + 8) &&
		    !(vec >= IRQ8_VECTOR && vec < IRQ8_VECTOR + 8))
			phys_copy(phys_b + 4L * vec, 4L * vec, 4L);
	repl_called = TRUE;
  }
  lock();
  phys_copy(phys_b + 4L * from, 4L * to, 4L);
  unlock();
}


/*============================================================================*
 *				w_geometry				      *
 *============================================================================*/
PRIVATE void w_geometry(chs)
unsigned *chs;			/* {cylinder, head, sector} */
{
  chs[0] = w_wn->cylinders;
  chs[1] = w_wn->heads;
  chs[2] = w_wn->sectors;
}
#endif /* ENABLE_BIOS_WINI */
