# Makefile for commands.

MAKE	= exec make -$(MAKEFLAGS)

usage:
	@echo "Usage: make all       # Compile all commands" >&2
	@echo "       make install   # Install the result (run as bin!)" >&2
	@echo "       make clean     # Delete .o files and other junk" >&2
	@false

all install clean:
	cd `arch` && $(MAKE) $@
	cd aal && $(MAKE) $@
	cd advent && $(MAKE) $@
	cd ash && $(MAKE) $@
	cd autil && $(MAKE) $@
	cd bawk && $(MAKE) $@
	cd bc && $(MAKE) $@
	cd byacc && $(MAKE) $@
	cd cawf && $(MAKE) $@
	cd de && $(MAKE) $@
	cd dis88 && $(MAKE) $@
	cd elle && $(MAKE) $@
	cd elvis && $(MAKE) $@
	cd flex-2.3.7 && $(MAKE) $@
	cd ftpd && $(MAKE) $@
	cd ibm && $(MAKE) $@
	cd ic && $(MAKE) $@
	cd indent && $(MAKE) $@
	cd kermit && $(MAKE) $@
	cd m4 && $(MAKE) $@
	cd make && $(MAKE) $@
	cd mined && $(MAKE) $@
	cd patch && $(MAKE) $@
	cd reboot && $(MAKE) $@
	cd scripts && $(MAKE) $@
	cd sh && $(MAKE) $@
	cd simple && $(MAKE) $@
	cd telnet && $(MAKE) $@
	cd telnetd && $(MAKE) $@
	cd yap && $(MAKE) $@
	cd zmodem && $(MAKE) $@
