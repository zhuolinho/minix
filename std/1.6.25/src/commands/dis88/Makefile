# Makefile for dis

# @(#) Makefile, Ver. 2.1 created 00:00:00 87/09/01
# Makefile for 8088 symbolic disassembler

# Copyright (C) 1987 G. M. Harding, all rights reserved.
# Permission to copy and redistribute is hereby granted,
# provided full source code, with all copyright notices,
# accompanies any redistribution.

# This Makefile  automates the process of compiling and linking
# a symbolic  object-file  disassembler  program  for the Intel
# 8088 CPU. Relatively machine-independent code is contained in
# the file dismain.c; lookup tables and handler routines, which
# are by their nature  machine-specific,  are  contained in two
# files named distabs.c and dishand.c,  respectively.  (A third
# machine-specific file, disfp.c, contains handler routines for
# floating-point  coprocessor opcodes.)  A header file,  dis.h,
# attempts to mediate between the machine-specific and machine-
# independent portions of the code. An attempt has been made to
# isolate machine  dependencies and to deal with them in fairly
# straightforward ways. Thus, it should be possible to target a
# different CPU by rewriting the handler  routines and changing
# the initialization  data in the lookup tables.  It should not
# be necessary to alter the formats of the tables.

CFLAGS =-O -wo
O=o
OBJ = disrel.$O dismain.$O distabs.$O dishand.$O disfp.$O

dis88:	$(OBJ)
	@rm -rf dis88
	@echo Start linking dis88
	@cc -o dis88 $(OBJ) >/dev/null
	@chmem =10000 dis88


disrel.$O:	disrel.c
dismain.$O:	dismain.c dis.h
distabs.$O:	distabs.c dis.h
dishand.$O:	dishand.c dis.h
disfp.$O:	disfp.c dis.h


clean:	
	@rm -f *.bak *.o *.s core dis88
