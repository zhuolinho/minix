/*==========================================================================*
 *		rs232.c - serial driver for 8250 and 16450 UARTs 	    *
 *==========================================================================*/

#include "kernel.h"
#include <sgtty.h>
#include "tty.h"

/* Switches.
 * #define C_RS232_INT_HANDLERS to use the interrupt handlers in this file.
 * #define NO_HANDSHAKE to avoid requiring CTS for output.
 */

/* 8250 constants. */
#define DEF_BAUD             1200	/* default baud rate */
#define UART_FREQ         115200L	/* timer frequency */

/* Interrupt enable bits. */
#define IE_RECEIVER_READY       1
#define IE_TRANSMITTER_READY    2
#define IE_LINE_STATUS_CHANGE   4
#define IE_MODEM_STATUS_CHANGE  8

/* Interrupt status bits. */
#define IS_MODEM_STATUS_CHANGE  0
#define IS_TRANSMITTER_READY    2
#define IS_RECEIVER_READY       4
#define IS_LINE_STATUS_CHANGE   6

/* Line control bits. */
#define LC_NO_PARITY            0
#define LC_DATA_BITS            3
#define LC_ODD_PARITY           8
#define LC_EVEN_PARITY       0x18
#define LC_ADDRESS_DIVISOR   0x80
#define LC_STOP_BITS_SHIFT      2

#define DATA_BITS_SHIFT         8	/* amount data bits shifted in mode */

/* Line status bits. */
#define LS_OVERRUN_ERR          2
#define LS_PARITY_ERR           4
#define LS_FRAMING_ERR          8
#define LS_BREAK_INTERRUPT   0x10
#define LS_TRANSMITTER_READY 0x20

/* Modem control bits. */
#define MC_DTR                  1
#define MC_RTS                  2
#define MC_OUT2                 8	/* required for PC & AT interrupts */

/* Modem status bits. */
#define MS_CTS               0x10

/* Input buffer watermarks.
 * The external device is asked to stop sending when the buffer
 * exactly reaches high water, or when TTY requests it.
 * TTY is also notified directly (rather than at a later clock tick) when
 * this watermark is reached.
 * A lower threshold could be used, but the buffer size and wakeup intervals
 * are chosen so the watermark shouldn't be hit at reasonable baud rates,
 * so this is unnecessary - the throttle is applied when TTY's buffers
 * get too full.
 * The low watermark is invisibly 0 since the buffer is always emptied all
 * at once.
 */
#define RS_IHIGHWATER (3 * RS_IBUFSIZE / 4)

/* Macros to handle flow control.
 * Interrupts must be off when they are used.
 * Time is critical - already the function call for out_byte() is annoying.
 * If out_byte() can be done in-line, tests to avoid it can be dropped.
 * istart() tells external device we are ready by raising RTS.
 * istop() tells external device we are not ready by dropping RTS.
 * DTR is kept high all the time (it probably should be raised by open and
 * dropped by close of the device).
 * OUT2 is also kept high all the time.
 */
#define istart(rs) \
  (out_byte( (rs)->modem_ctl_port, MC_OUT2 | MC_RTS | MC_DTR), \
   (rs)->idevready = TRUE)
#define istop(rs) \
  (out_byte( (rs)->modem_ctl_port, MC_OUT2 | MC_DTR), (rs)->idevready = FALSE)

/* Macro to tell if device is ready.
 * Don't require DSR, since modems drop this to indicate the line is not
 * ready even when the modem itself is ready.
 * If NO_HANDSHAKE, read the status port to clear the interrupt, then force
 * the ready bit.
 */
#if NO_HANDSHAKE
# define devready(rs) (in_byte(rs->modem_status_port), MS_CTS)
#else
# define devready(rs) (in_byte(rs->modem_status_port) & MS_CTS)
#endif

/* Macro to tell if transmitter is ready. */
#define txready(rs) (in_byte(rs->line_status_port) & LS_TRANSMITTER_READY)

/* Types. */
typedef char bool_t;		/* boolean */
typedef unsigned port_t;	/* hardware port */

/* RS232 device structure, one per device. */
struct rs232_s {
  int minor;			/* minor number of this line (base 0) */

  bool_t idevready;		/* nonzero if we are ready to receive (RTS) */
  bool_t ittyready;		/* nonzero if TTY is ready to receive */
  char *ibuf;			/* start of input buffer */
  char *ibufend;		/* end of input buffer */
  char *ihighwater;		/* threshold in input buffer */
  char *iptr;			/* next free spot in input buffer */

  unsigned char ostate;		/* combination of flags: */
#define ODEVREADY MS_CTS	/* external device hardware ready (CTS) */
#define ODONE          1	/* output completed (< output enable bits) */
#define OQUEUED     0x20	/* output buffer not empty */
#define ORAW           2	/* raw mode for xoff disable (< enab. bits) */
#define OSWREADY    0x40	/* external device software ready (no xoff) */
#define OWAKEUP        4	/* tty_wakeup() pending (asm code only) */
#if (ODEVREADY | 0x63) == 0x63
#error				/* bits are not unique */
#endif
  unsigned char oxoff;		/* char to stop output */
  char *obufend;		/* end of output buffer */
  char *optr;			/* next char to output */

  port_t xmit_port;		/* i/o ports */
  port_t recv_port;
  port_t div_low_port;
  port_t div_hi_port;
  port_t int_enab_port;
  port_t int_id_port;
  port_t line_ctl_port;
  port_t modem_ctl_port;
  port_t line_status_port;
  port_t modem_status_port;

  unsigned char lstatus;	/* last line status */
  unsigned char pad;		/* ensure alignment for 16-bit ints */
  unsigned framing_errors;	/* error counts (no reporting yet) */
  unsigned overrun_errors;
  unsigned parity_errors;
  unsigned break_interrupts;

  char ibuf1[RS_IBUFSIZE + 1];	/* 1st input buffer, guard at end */
  char ibuf2[RS_IBUFSIZE + 1];	/* 2nd input buffer (for swapping) */
};

/* Table and macro to translate an RS232 minor device number to its
 * struct rs232_s pointer.
 */
struct rs232_s *p_rs_addr[NR_RS_LINES];

#define rs_addr(minor) (p_rs_addr[minor])

/* 8250 base addresses. */
PRIVATE port_t addr_8250[] = {
  0x3F8,			/* COM1: (line 0); COM3 might be at 0x3E8 */
  0x2F8,			/* COM2: (line 1); COM4 might be at 0x2E8 */
};

PUBLIC struct rs232_s rs_lines[NR_RS_LINES];

#if C_RS232_INT_HANDLERS
FORWARD void in_int();
FORWARD void line_int();
FORWARD void modem_int();

#endif
FORWARD void out_int();
FORWARD int rs_config();


/* High level routines (should only be called by TTY). */

/*==========================================================================*
 *				rs_config			 	    *
 *==========================================================================*/
PRIVATE int rs_config(minor, in_baud, out_baud, parity, stop_bits, data_bits,
		      mode)
int minor;			/* which rs line */
int in_baud;			/* input speed: 110, 300, 1200, etc */
int out_baud;			/* output speed: 110, 300, 1200, etc */
int parity;			/* LC_something */
int stop_bits;			/* 2 (110 baud) or 1 (other speeds) */
int data_bits;			/* 5, 6, 7, or 8 */
int mode;			/* sgtty.h sg_mode word */
{
/* Set various line control parameters for RS232 I/O.
 * If DataBits == 5 and StopBits == 2, 8250 will generate 1.5 stop bits.
 * The 8250 can't handle split speed, but we have propagated both speeds
 * anyway for the benefit of future UART chips.
 */

  int divisor;
  int line_controls;
  register struct rs232_s *rs;

  rs = rs_addr(minor);

  /* Precalculate divisor and line_controls for reduced latency. */
  if (in_baud < 50) in_baud = DEF_BAUD;	/* prevent divide overflow */
  if (out_baud < 50) out_baud = DEF_BAUD;	/* prevent divide overflow */
  divisor = (int) (UART_FREQ / in_baud);	/* 8250 can't hack 2 speeds */
  line_controls = parity | ((stop_bits - 1) << LC_STOP_BITS_SHIFT)
			 | (data_bits - 5);

  /* Lock out interrupts while setting the speed. The receiver register is
   * going to be hidden by the div_low register, but the input interrupt
   * handler relies on reading it to clear the interrupt and avoid looping
   * forever.
   */
  lock();

  /* Select the baud rate divisor registers and change the rate. */
  out_byte(rs->line_ctl_port, LC_ADDRESS_DIVISOR);
  out_byte(rs->div_low_port, divisor);
  out_byte(rs->div_hi_port, divisor >> 8);

  /* Change the line controls and reselect the usual registers. */
  out_byte(rs->line_ctl_port, line_controls);

  if (mode & RAW)
	rs->ostate |= ORAW;
  else
	rs->ostate &= ~ORAW;
  unlock();
  return( (out_baud / 100) << 8) | (in_baud / 100);
}


/*==========================================================================*
 *				rs_ioctl			 	    *
 *==========================================================================*/
PUBLIC int rs_ioctl(minor, mode, speeds)
int minor;			/* which rs line */
int mode;			/* sgtty.h sg_mode word */
int speeds;			/* low byte is input speed, next is output */
{
/* Set the UART parameters. */

  int data_bits;
  int in_baud;
  int out_baud;
  int parity;
  int stop_bits;

  in_baud = 100 * (speeds & BYTE);
  if (in_baud == 100) in_baud = 110;
  out_baud = 100 * ((speeds >> 8) & BYTE);
  if (out_baud == 100) out_baud = 110;
  parity = LC_NO_PARITY;
  if (mode & ODDP) parity = LC_ODD_PARITY;
  if (mode & EVENP) parity = LC_EVEN_PARITY;
  stop_bits = in_baud == 110 ? 2 : 1;	/* not quite cricket */
  data_bits = 5 + ((mode >> DATA_BITS_SHIFT) & LC_DATA_BITS);
  return(rs_config(minor, in_baud, out_baud, parity, stop_bits, data_bits,
		     mode));
}


/*==========================================================================*
 *				rs_inhibit				    *
 *==========================================================================*/
PUBLIC void rs_inhibit(minor, inhibit)
int minor;			/* which rs line */
bool_t inhibit;			/* nonzero to inhibit, zero to uninhibit */
{
/* Update inhibition state to keep in sync with TTY. */

  register struct rs232_s *rs;

  rs = rs_addr(minor);
  lock();
  if (inhibit)
	rs->ostate &= ~OSWREADY;
  else
	rs->ostate |= OSWREADY;
  unlock();
}


/*==========================================================================*
 *				rs_init					    *
 *==========================================================================*/
PUBLIC int rs_init(minor)
int minor;			/* which rs line */
{
/* Initialize RS232 for one line. */

  register struct rs232_s *rs;
  int speed;
  port_t this_8250;

  rs = rs_addr(minor) = &rs_lines[minor];

  /* Record minor number. */
  rs->minor = minor;

  /* Set up input queue. */
  rs->iptr = rs->ibuf = rs->ibuf1;
  rs->ibufend = rs->ibuf1 + RS_IBUFSIZE;
  rs->ihighwater = rs->ibuf1 + RS_IHIGHWATER;
  rs->ittyready = TRUE;		/* idevready set to TRUE by istart() */

  /* Precalculate port numbers for speed. Magic numbers in the code (once). */
  this_8250 = addr_8250[minor];
  rs->xmit_port = this_8250 + 0;
  rs->recv_port = this_8250 + 0;
  rs->div_low_port = this_8250 + 0;
  rs->div_hi_port = this_8250 + 1;
  rs->int_enab_port = this_8250 + 1;
  rs->int_id_port = this_8250 + 2;
  rs->line_ctl_port = this_8250 + 3;
  rs->modem_ctl_port = this_8250 + 4;
  rs->line_status_port = this_8250 + 5;
  rs->modem_status_port = this_8250 + 6;

  /* Set up the hardware to a base state, in particular
   *	o turn off DTR (MC_DTR) to try to stop the external device.
   *	o disable interrupts at the chip level, to force an edge transition
   *	  on the 8259 line when interrupts are next enabled and active.
   *	  RS232 interrupts are guaranteed to be disabled now by the 8259
   *	  mask, but there used to be trouble if the mask was set without
   *	  handling a previous interrupt.
   *	o be careful about the divisor latch.  It may be enabled now, and
   *	  that used to cause trouble when interrupts were enabled too early
   *	  (see comment in rs_config()).  Call rs_config() early to avoid this.
   */
  istop(rs);			/* sets modem_ctl_port */
  out_byte(rs->int_enab_port, 0);
  speed = rs_config(minor, DEF_BAUD, DEF_BAUD, LC_NO_PARITY, 1, 8, RAW);

  /* Clear any harmful leftover interrupts.  An output interrupt is harmless
   * and will occur when interrupts are enabled anyway.  Set up the output
   * queue using the status from clearing the modem status interrupt.
   */
  in_byte(rs->line_status_port);
  in_byte(rs->recv_port);
  rs->ostate = devready(rs) | ORAW | OSWREADY;	/* reads modem_ctl_port */

  /* Enable interrupts for both interrupt controller and device. */
  if (minor & 1)		/* COM2 on IRQ3 */
	enable_irq(SECONDARY_IRQ);
  else				/* COM1 on IRQ4 */
	enable_irq(RS232_IRQ);
  out_byte(rs->int_enab_port, IE_LINE_STATUS_CHANGE | IE_MODEM_STATUS_CHANGE
				| IE_RECEIVER_READY | IE_TRANSMITTER_READY);

  /* Tell external device we are ready. */
  istart(rs);

  return(speed);
}


/*==========================================================================*
 *				rs_istop				    *
 *==========================================================================*/
PUBLIC void rs_istop(minor)
int minor;			/* which rs line */
{
/* TTY wants RS232 to stop input.
 * RS232 drops RTS but keeps accepting input until its buffer overflows.
 */

  register struct rs232_s *rs;

  rs = rs_addr(minor);
  lock();
  rs->ittyready = FALSE;
  istop(rs);
  unlock();
}


/*==========================================================================*
 *				rs_istart				    *
 *==========================================================================*/
PUBLIC void rs_istart(minor)
int minor;			/* which rs line */
{
/* TTY is ready for another buffer full of input from RS232.
 * RS232 raises RTS unless its own buffer is already too full.
 */

  register struct rs232_s *rs;

  rs = rs_addr(minor);
  lock();
  rs->ittyready = TRUE;
  if (rs->iptr < rs->ihighwater) istart(rs);
  unlock();
}


/*==========================================================================*
 *				rs_ocancel				    *
 *==========================================================================*/
PUBLIC void rs_ocancel(minor)
int minor;			/* which rs line */
{
/* Cancel pending output. */

  register struct rs232_s *rs;

  lock();
  rs = rs_addr(minor);
  if (rs->ostate & ODONE) tty_events -= EVENT_THRESHOLD;
  rs->ostate &= ~(ODONE | OQUEUED);
  unlock();
}


/*==========================================================================*
 *				rs_read					    *
 *==========================================================================*/
PUBLIC int rs_read(minor, bufindirect, odoneindirect)
int minor;			/* which rs line */
char **bufindirect;		/* where to return pointer to our buffer */
bool_t *odoneindirect;		/* where to return output-done status */
{
/* Swap the input buffers, giving the old one to TTY, and restart input. */

  register char *ibuf;
  int nread;
  register struct rs232_s *rs;

  rs = rs_addr(minor);
  *odoneindirect = rs->ostate & ODONE;
  if (rs->iptr == (ibuf = rs->ibuf)) return(0);
  *bufindirect = ibuf;
  lock();
  nread = rs->iptr - ibuf;
  tty_events -= nread;
  if (ibuf == rs->ibuf1)
	ibuf = rs->ibuf2;
  else
	ibuf = rs->ibuf1;
  rs->ibufend = ibuf + RS_IBUFSIZE;
  rs->ihighwater = ibuf + RS_IHIGHWATER;
  rs->iptr = ibuf;
  if (!rs->idevready && rs->ittyready) istart(rs);
  unlock();
  rs->ibuf = ibuf;
  return(nread);
}


/*==========================================================================*
 *				rs_setc					    *
 *==========================================================================*/
PUBLIC void rs_setc(minor, xoff)
int minor;			/* which rs line */
int xoff;			/* xoff character */
{
/* RS232 needs to know the xoff character. */

  rs_addr(minor)->oxoff = xoff;
}


/*==========================================================================*
 *				rs_write				    *
 *==========================================================================*/
PUBLIC void rs_write(minor, buf, nbytes)
int minor;			/* which rs line */
char *buf;			/* pointer to buffer to write */
int nbytes;			/* number of bytes to write */
{
/* Tell RS232 about the buffer to be written and start output.
 * Previous output must have completed.
 */

  register struct rs232_s *rs;

  rs = rs_addr(minor);
  lock();
  rs->obufend = (rs->optr = buf) + nbytes;
  rs->ostate |= OQUEUED;
  if (txready(rs)) out_int(rs);
  unlock();
}


/* Low level (interrupt) routines. */

#if C_RS232_INT_HANDLERS
/*==========================================================================*
 *				rs232_1handler				    *
 *==========================================================================*/
PUBLIC void rs232_1handler()
{
/* Interrupt hander for IRQ4.
 * Only 1 line (usually COM1) should use it.
 */

#if NR_RS_LINES > 0
  register struct rs232_s *rs;

  rs = &rs_lines[0];
  while (TRUE) {
	/* Loop to pick up ALL pending interrupts for device.
	 * This usually just wastes time unless the hardware has a buffer
	 * (and then we have to worry about being stuck in the loop too long).
	 * Unfortunately, some serial cards lock up without this.
	 */
	switch (in_byte(rs->int_id_port)) {
	case IS_RECEIVER_READY:
		in_int(rs);
		continue;
	case IS_TRANSMITTER_READY:
		out_int(rs);
		continue;
	case IS_MODEM_STATUS_CHANGE:
		modem_int(rs);
		continue;
	case IS_LINE_STATUS_CHANGE:
		line_int(rs);
		continue;
	}
	return;
  }
#endif
}


/*==========================================================================*
 *				rs232_2handler				    *
 *==========================================================================*/
PUBLIC void rs232_2handler()
{
/* Interrupt hander for IRQ3.
 * Only 1 line (usually COM2) should use it.
 */

#if NR_RS_LINES > 1
  register struct rs232_s *rs;

  rs = &rs_lines[1];
  while (TRUE) {
	switch (in_byte(rs->int_id_port)) {
	case IS_RECEIVER_READY:
		in_int(rs);
		continue;
	case IS_TRANSMITTER_READY:
		out_int(rs);
		continue;
	case IS_MODEM_STATUS_CHANGE:
		modem_int(rs);
		continue;
	case IS_LINE_STATUS_CHANGE:
		line_int(rs);
		continue;
	}
	return;
  }
#endif
}


/*==========================================================================*
 *				in_int					    *
 *==========================================================================*/
PRIVATE void in_int(rs)
register struct rs232_s *rs;	/* line with input interrupt */
{
/* Read the data which just arrived.
 * If it is the oxoff char, clear OSWREADY, else if OSWREADY was clear, set
 * it and restart output (any char does this, not just xon).
 * Put data in the buffer if room, * otherwise discard it.
 * Set a flag for the clock interrupt handler to eventually notify TTY.
 */

  if (rs->ostate & ORAW)
	*rs->iptr = in_byte(rs->recv_port);
  else if ( (*rs->iptr = in_byte(rs->recv_port)) == rs->oxoff)
	rs->ostate &= ~OSWREADY;
  else if (!(rs->ostate & OSWREADY)) {
	rs->ostate |= OSWREADY;
	if (txready(rs)) out_int(rs);
  }
  if (rs->iptr < rs->ibufend) {
	++tty_events;
	if (++rs->iptr == rs->ihighwater) istop(rs);
  }
}


/*==========================================================================*
 *				line_int				    *
 *==========================================================================*/
PRIVATE void line_int(rs)
register struct rs232_s *rs;	/* line with line status interrupt */
{
/* Check for and record errors. */

  rs->lstatus = in_byte(rs->line_status_port);
  if (rs->lstatus & LS_FRAMING_ERR) ++rs->framing_errors;
  if (rs->lstatus & LS_OVERRUN_ERR) ++rs->overrun_errors;
  if (rs->lstatus & LS_PARITY_ERR) ++rs->parity_errors;
  if (rs->lstatus & LS_BREAK_INTERRUPT) ++rs->break_interrupts;
}


/*==========================================================================*
 *				modem_int				    *
 *==========================================================================*/
PRIVATE void modem_int(rs)
register struct rs232_s *rs;	/* line with modem interrupt */
{
/* Get possibly new device-ready status, and clear ODEVREADY if necessary.
 * If the device just became ready, restart output.
 */

  if (!devready(rs))
	rs->ostate &= ~ODEVREADY;
  else if (!(rs->ostate & ODEVREADY)) {
	rs->ostate |= ODEVREADY;
	if (txready(rs)) out_int(rs);
  }
}

#endif /* C_RS232_INT_HANDLERS (except out_int is used from high level) */


/*==========================================================================*
 *				out_int					    *
 *==========================================================================*/
PRIVATE void out_int(rs)
register struct rs232_s *rs;	/* line with output interrupt */
{
/* If there is output to do and everything is ready, do it (local device is
 * known ready).
 * Interrupt TTY to indicate completion.
 */

  if (rs->ostate >= (ODEVREADY | OQUEUED | OSWREADY)) {
	/* Bit test allows ORAW and requires the others. */
	out_byte(rs->xmit_port, *rs->optr);
	if (++rs->optr >= rs->obufend) {
		tty_events += EVENT_THRESHOLD;
		rs->ostate ^= (ODONE | OQUEUED);  /* ODONE on, OQUEUED off */
		unlock();	/* safe, for complicated reasons */
		tty_wakeup();
		lock();
	}
  }
}
