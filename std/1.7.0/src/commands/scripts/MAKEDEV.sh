#!/bin/sh
#
# MAKEDEV 2.13 - Make special devices.			Author: Kees J. Bot

case $1 in
-n)	e=echo; shift ;;	# Just echo when -n is given.
*)	e=
esac

case $#:$1 in
1:std)		# Standard devices.
	set -$- mem fd0 fd1 fd0a fd1a \
		hd0 hd1a hd5 hd6a sd0 sd1a sd5 sd6a \
		st4 tty eth
	;;
0:|1:-\?)
	cat >&2 <<EOF
Usage:	$0 [-n] key ...
Where key is one of the following:
	ram mem kmem null	# One of these makes all these memory devices
	fd0 fd1 ...		# Floppy devices for drive 0, 1, ...
	fd0a fd1a ...		# Make fd0[a-d], fd1[a-d], ...
	hd0 hd5 ...		# Make hd[0-4], hd[5-9], ...
	hd1a hd2a ... hd6a ...	# Make hd1[a-d], hd2[a-d], hd6[a-d], ...
	sd0 sd5 sd1a ...	# Make SCSI disks
	st0 st1 ...		# Make SCSI tapes rst0, nrst0, rst1 ...
	console lp tty tty[0-2]	# One of these makes all six
	eth psip ip tcp udp	# One of these makes TCP/IP devices
	std			# All standard devices
EOF
	exit 1
esac

umask 077
ex=0

for dev
do
	case $dev in
	ram|mem|kmem|null)
		# Memory devices.
		#
		$e mknod ram b 1 0;	$e chmod 600 ram
		$e mknod mem c 1 1;	$e chmod 640 mem
		$e mknod kmem c 1 2;	$e chmod 640 kmem
		$e mknod null c 1 3;	$e chmod 666 null
		$e chgrp kmem ram mem kmem null
		;;
	fd[0-3]|pc[0-3]|at[0-3]|qd[0-3]|ps[0-3]|pat[0-3]|qh[0-3]|PS[0-3])
		# Floppy disk drive n.
		#
		n=`expr $dev : '.*\\(.\\)'`	# Drive number.
		m=$n				# Minor device number.

		$e mknod fd$n  b 2 $m;	m=`expr $m + 4`
		$e mknod pc$n  b 2 $m;	m=`expr $m + 4`
		$e mknod at$n  b 2 $m;	m=`expr $m + 4`
		$e mknod qd$n  b 2 $m;	m=`expr $m + 4`
		$e mknod ps$n  b 2 $m;	m=`expr $m + 4`
		$e mknod pat$n b 2 $m;	m=`expr $m + 4`
		$e mknod qh$n  b 2 $m;	m=`expr $m + 4`
		$e mknod PS$n  b 2 $m;	m=`expr $m + 4`

		$e chmod 666 fd$n pc$n at$n qd$n ps$n pat$n qh$n PS$n
		;;
	fd[0-3][a-d])
		# Floppy disk partitions.
		#
		dev=`expr $dev : '\\(.*\\).'`	# Chop off the letter.
		drive=`expr $dev : '..\\(.\\)'`	# Drive number.
		n=`expr 112 + $drive`		# Partition 'a'.
		alldev=

		for par in a b c d
		do
			$e mknod $dev$par b 2 $n	# Make e.g. fd0a - fd0d
			alldev="$alldev $dev$par"
			n=`expr $n + 4`
		done
		$e chmod 666 $alldev
		;;
	[hs]d[0-9]|[hs]d[123][0-9])
		# Hard disk drive & partitions.
		#
		case $dev in
		h*)	name=hd maj=3		# Winchester.
			;;
		s*)	name=sd maj=10		# SCSI.
		esac
		n=`expr $dev : '[^0-9]*\\(.*\\)'`  # Minor device number.
		n=`expr $n / 5 '*' 5`		# Down to a multiple of 5.
		alldev=

		for par in 0 1 2 3 4
		do
			$e mknod $name$n b $maj $n	# Make e.g. hd5 - hd9
			alldev="$alldev $name$n"
			n=`expr $n + 1`
		done
		$e chmod 600 $alldev
		;;
	[hs]d[1-9][a-d]|[hs]d[123][1-9][a-d])
		# Hard disk subpartitions.
		#
		case $dev in
		h*)	name=hd maj=3		# Winchester.
			;;
		s*)	name=sd maj=10		# SCSI.
		esac
		par=`expr $dev : '..\\(.*\\).'`	# Partition number.
		drive=`expr $par / 5`		# Drive number.
		n=`expr $drive '*' 16 + 128`	# Subpartition '1a', '6a', ...
		alldev=

		for par in 1 2 3 4
		do
			dev=$name`expr $drive '*' 5 + $par`
			for sub in a b c d
			do
				# Make e.g. hd6a, hd6b, ... hd9d
				$e mknod $dev$sub b $maj $n
				alldev="$alldev $dev$sub"
				n=`expr $n + 1`
			done
		done
		$e chmod 600 $alldev
		;;
	st[0-7]|rst[0-7]|nrst[0-7])
		# SCSI tape.
		#
		n=`expr $dev : '.*\\(.\\)'`
		m=`expr 64 + $n '*' 2`		# Minor of rstX.

		$e mknod nrst$n c 10 $m
		$e mknod rst$n c 10 `expr $m + 1`
		$e chmod 660 rst$n nrst$n
		;;
	console|lp|tty|tty[0-2])
		# Console, line printer, anonymous tty, standard ttys.
		#
		$e mknod console c 4 0
		$e chmod 600 console
		$e chgrp tty console
		$e mknod lp c 6 0
		$e chown daemon lp
		$e chgrp daemon lp
		$e chmod 200 lp
		$e mknod tty c 5 0
		$e chmod 666 tty
		$e ln console tty0
		$e mknod tty1 c 4 1
		$e mknod tty2 c 4 2
		$e chmod 666 tty[12]
		$e chgrp tty tty[12]
		;;
	eth|ip|tcp|udp)
		# TCP/IP devices.
		#
		$e mknod eth c 7 1
		$e mknod ip c 7 2
		$e mknod tcp c 7 3
		$e mknod udp c 7 4
		$e chmod 666 eth ip	# These two should be mode 600!
		$e chmod 666 tcp udp
		;;
	*)
		echo "$0: don't know about $dev" >&2
		ex=1
	esac
done

exit $ex
