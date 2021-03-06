|*===========================================================================*
|*		rs232 interrupt handlers for real and protected modes	     *
|*===========================================================================*
| This is a fairly direct translation of the interrupt handlers in rs232.c.
| See the comments there.
| It is about 5 times as efficient, by avoiding save/restart and slow function
| calls for port i/o as well as the compiler!

#include <minix/config.h>

#if !C_RS232_INT_HANDLERS /* otherwise, don't use anything in this file */

#include <minix/const.h>
#include "const.h"
#include "sconst.h"

| exported functions

	.text
.define		_prs232_int
.define		_psecondary_int
.define		_rs232_int		| used only for real mode
.define		_secondary_int		| used only for real mode

| imported functions

.extern		kernel_ds
.extern		save
.extern		_tty_wakeup

| imported variables

	.bss
.extern		_rs_lines
.extern		_tty_events

#undef MINOR

#if !INTEL_32BITS
#define eax ax			/* use 32-bit register names throughout */
#define ebx bx			/* but modify them for 16-bit mode/CPU */
#define edx dx
#define esi si
#define iretd iret
#endif

#if ASLD
#define add1_and_align(n)	[[[n]+SIZEOF_INT] / SIZEOF_INT * SIZEOF_INT]
#else
#define add1_and_align(n)	(((n)+SIZEOF_INT) / SIZEOF_INT * SIZEOF_INT)
#endif

| These constants are defined in tty.h. That has C stuff so can't be included.
EVENT_THRESHOLD		=	64
RS_IBUFSIZE		=	256

| These constants are defined in rs232.c.
IS_LINE_STATUS_CHANGE	=	6
IS_MODEM_STATUS_CHANGE	=	0
IS_NO_INTERRUPT		=	1
IS_RECEIVER_READY	=	4
IS_TRANSMITTER_READY	=	2
LS_OVERRUN_ERR		=	2
LS_PARITY_ERR		=	4
LS_FRAMING_ERR		=	8
LS_BREAK_INTERRUPT	=	0x10
LS_TRANSMITTER_READY	=	0x20
MC_DTR			=	1
MC_OUT2			=	8
MS_CTS			=	0x10
ODEVREADY		=	MS_CTS
ODONE			=	1
OQUEUED			=	0x20
ORAW			=	2
OSWREADY		=	0x40
OWAKEUP			=	4
RS_IHIGHWATER		=	3*RS_IBUFSIZE/4

| These port offsets are hard-coded in rs232.c.
XMIT_OFFSET		=	0
RECV_OFFSET		=	0
INT_ID_OFFSET		=	2
MODEM_CTL_OFFSET	=	4
LINE_STATUS_OFFSET	=	5
MODEM_STATUS_OFFSET	=	6

| Offsets in struct rs232_s. They must match rs232.c
MINOR			=	0
IDEVREADY		=	MINOR+SIZEOF_INT
ITTYREADY		=	IDEVREADY+1
IBUF			=	add1_and_align(ITTYREADY)
IBUFEND			=	IBUF+SIZEOF_INT
IHIGHWATER		=	IBUFEND+SIZEOF_INT
IPTR			=	IHIGHWATER+SIZEOF_INT
OSTATE			=	IPTR+SIZEOF_INT
OXOFF			=	OSTATE+1
OBUFEND			=	add1_and_align(OXOFF)
OPTR			=	OBUFEND+SIZEOF_INT
XMIT_PORT		=	OPTR+SIZEOF_INT
RECV_PORT		=	XMIT_PORT+SIZEOF_INT
DIV_LOW_PORT		=	RECV_PORT+SIZEOF_INT
DIV_HI_PORT		=	DIV_LOW_PORT+SIZEOF_INT
INT_ENAB_PORT		=	DIV_HI_PORT+SIZEOF_INT
INT_ID_PORT		=	INT_ENAB_PORT+SIZEOF_INT
LINE_CTL_PORT		=	INT_ID_PORT+SIZEOF_INT
MODEMCTL_PORT		=	LINE_CTL_PORT+SIZEOF_INT
LINESTATUS_PORT		=	MODEMCTL_PORT+SIZEOF_INT
MODEMSTATUS_PORT	=	LINESTATUS_PORT+SIZEOF_INT
LSTATUS			=	MODEMSTATUS_PORT+SIZEOF_INT
FRAMING_ERRORS		=	add1_and_align(LSTATUS)
OVERRUN_ERRORS		=	FRAMING_ERRORS+SIZEOF_INT
PARITY_ERRORS		=	OVERRUN_ERRORS+SIZEOF_INT
BREAK_INTERRUPTS	=	PARITY_ERRORS+SIZEOF_INT
IBUF1			=	BREAK_INTERRUPTS+SIZEOF_INT
IBUF2			=	IBUF1+RS_IBUFSIZE+1
SIZEOF_STRUCT_RS232_S	=	add1_and_align(IBUF2+RS_IBUFSIZE)

	.text

| PUBLIC void interrupt _psecondary_int();

_psecondary_int:
	push	ds
	push	esi
	mov	si,ss
	mov	ds,si
	mov	esi,#_rs_lines+SIZEOF_STRUCT_RS232_S
	j	common

| PUBLIC void interrupt _secondary_int();

_secondary_int:
	push	ds
	push	esi
	mov	esi,#_rs_lines+SIZEOF_STRUCT_RS232_S
	seg	cs
	mov	ds,kernel_ds
	j	common

| PUBLIC void interrupt _prs232_int();

_prs232_int:
	push	ds
	push	esi
	mov	si,ss
	mov	ds,si
	mov	esi,#_rs_lines
	j	common

| input interrupt

inint:
	addb	dl,#RECV_OFFSET-INT_ID_OFFSET
	in
	mov	ebx,IPTR(esi)
	movb	(ebx),al
	cmp	ebx,IBUFEND(esi)
	jge	checkxoff
	inc	_tty_events
	inc	ebx
	mov	IPTR(esi),ebx
	cmp	ebx,IHIGHWATER(esi)
	jne	checkxoff
	addb	dl,#MODEM_CTL_OFFSET-RECV_OFFSET
	movb	al,#MC_OUT2+MC_DTR
	out
	movb	IDEVREADY(esi),#FALSE
checkxoff:
	testb	ah,#ORAW
	jne	rsnext
	cmpb	al,OXOFF(esi)
	je	gotxoff
	testb	ah,#OSWREADY
	jne	rsnext
	orb	ah,#OSWREADY
	mov	edx,LINESTATUS_PORT(esi)
	in
	testb	al,#LS_TRANSMITTER_READY
	je	rsnext
	addb	dl,#XMIT_OFFSET-LINE_STATUS_OFFSET
	j	outint1

gotxoff:
	andb	ah,#notop(OSWREADY)
	j	rsnext

| PUBLIC void interrupt rs232_int();

_rs232_int:
	push	ds
	push	esi
	mov	esi,#_rs_lines
	seg	cs
	mov	ds,kernel_ds

common:
	push	eax
	push	ebx
	push	edx
	movb	ah,OSTATE(esi)
	mov	edx,INT_ID_PORT(esi)
	in
rsmore:
	cmpb	al,#IS_RECEIVER_READY
	je	inint
	cmpb	al,#IS_TRANSMITTER_READY
	je	outint
	cmpb	al,#IS_MODEM_STATUS_CHANGE
	je	modemint
	cmpb	al,#IS_LINE_STATUS_CHANGE
	jne	rsdone		| fishy

| line status change interrupt

	addb	dl,#LINE_STATUS_OFFSET-INT_ID_OFFSET
	in
	testb	al,#LS_FRAMING_ERR
	je	over_framing_error
	inc	FRAMING_ERRORS(esi)	
over_framing_error:
	testb	al,#LS_OVERRUN_ERR
	je	over_overrun_error
	inc	OVERRUN_ERRORS(esi)	
over_overrun_error:
	testb	al,#LS_PARITY_ERR
	je	over_parity_error
	inc	PARITY_ERRORS(esi)
over_parity_error:
	testb	al,#LS_BREAK_INTERRUPT
	je	over_break_interrupt
	inc	BREAK_INTERRUPTS(esi)
over_break_interrupt:

rsnext:
	mov	edx,INT_ID_PORT(esi)
	in
	cmpb	al,#IS_NO_INTERRUPT
	jne	rsmore
rsdone:
	movb	al,#ENABLE
	out	INT_CTL
	testb	ah,#OWAKEUP
	jne	owakeup
	movb	OSTATE(esi),ah
	pop	edx
	pop	ebx
	pop	eax
	pop	esi
	pop	ds
	iretd

| output interrupt

outint:
	addb	dl,#XMIT_OFFSET-INT_ID_OFFSET
outint1:
	cmpb	ah,#ODEVREADY+OQUEUED+OSWREADY
	jb	rsnext		| not all are set
	mov	ebx,OPTR(esi)
	movb	al,(ebx)
	out
	inc	ebx
	mov	OPTR(esi),ebx
	cmp	ebx,OBUFEND(esi)
	jb	rsnext
	add	_tty_events,#EVENT_THRESHOLD
	xorb	ah,#ODONE+OQUEUED+OWAKEUP	| OQUEUED off, others on
	j	rsnext		| direct exit might lose interrupt

| modem status change interrupt

modemint:
	addb	dl,#MODEM_STATUS_OFFSET-INT_ID_OFFSET
	in
#if NO_HANDSHAKE
	orb	al,#MS_CTS
#endif
	testb	al,#MS_CTS
	jne	m_devready
	andb	ah,#notop(ODEVREADY)
	j	rsnext

m_devready:
	testb	ah,#ODEVREADY
	jne	rsnext
	orb	ah,#ODEVREADY
	addb	dl,#LINE_STATUS_OFFSET-MODEM_STATUS_OFFSET
	in
	testb	al,#LS_TRANSMITTER_READY
	je	rsnext
	addb	dl,#XMIT_OFFSET-LINE_STATUS_OFFSET
	j	outint1

| special exit for output just completed

owakeup:
	andb	ah,#notop(OWAKEUP)
	movb	OSTATE(esi),ah

| determine mask bit (it would be better to precalculate it in the struct)

	movb	ah,#SECONDARY_MASK
	cmp	esi,#_rs_lines
	jne	got_rs_mask
	movb	ah,#RS232_MASK
got_rs_mask:
	mov	rs_mask,eax	| save mask to clear later
	in	INT_CTLMASK
	orb	al,ah
	out	INT_CTLMASK

| rearrange context to call tty_wakeup()

	pop	edx
	pop	ebx
	pop	eax
	pop	esi
	pop	ds
	call	save
	push	rs_mask		| save the mask again, reentrantly
	sti
	call	_tty_wakeup
	cli
	pop	eax
	notb	ah		| return this
	in	INT_CTLMASK
	andb	al,ah
	out	INT_CTLMASK
	ret


	.data
rs_mask:
	.space	2
	.space	2		| align

#endif /* !C_RS232_INT_HANDLERS */
