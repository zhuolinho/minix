# Makefile for lib/posix.

CFLAGS	= -O -D_MINIX -D_POSIX_SOURCE
CC1	= $(CC) $(CFLAGS) -c

LIBRARY	= ../libc.a
all:	$(LIBRARY)

OBJECTS	= \
	$(LIBRARY)(__exit.o) \
	$(LIBRARY)(_access.o) \
	$(LIBRARY)(_alarm.o) \
	$(LIBRARY)(_chdir.o) \
	$(LIBRARY)(_chmod.o) \
	$(LIBRARY)(_chown.o) \
	$(LIBRARY)(_chroot.o) \
	$(LIBRARY)(_close.o) \
	$(LIBRARY)(_closedir.o) \
	$(LIBRARY)(_creat.o) \
	$(LIBRARY)(_dup.o) \
	$(LIBRARY)(_dup2.o) \
	$(LIBRARY)(_exec.o) \
	$(LIBRARY)(_execn.o) \
	$(LIBRARY)(_fcntl.o) \
	$(LIBRARY)(_fork.o) \
	$(LIBRARY)(_fpathconf.o) \
	$(LIBRARY)(_fstat.o) \
	$(LIBRARY)(_getcwd.o) \
	$(LIBRARY)(_getegid.o) \
	$(LIBRARY)(_geteuid.o) \
	$(LIBRARY)(_getgid.o) \
	$(LIBRARY)(_getgroups.o) \
	$(LIBRARY)(_getpid.o) \
	$(LIBRARY)(_getppid.o) \
	$(LIBRARY)(_getuid.o) \
	$(LIBRARY)(_gtty.o) \
	$(LIBRARY)(_ioctl.o) \
	$(LIBRARY)(_isatty.o) \
	$(LIBRARY)(_kill.o) \
	$(LIBRARY)(_link.o) \
	$(LIBRARY)(_lseek.o) \
	$(LIBRARY)(_mkdir.o) \
	$(LIBRARY)(_mkfifo.o) \
	$(LIBRARY)(_mknod.o) \
	$(LIBRARY)(_mktemp.o) \
	$(LIBRARY)(_mount.o) \
	$(LIBRARY)(_open.o) \
	$(LIBRARY)(_opendir.o) \
	$(LIBRARY)(_pathconf.o) \
	$(LIBRARY)(_pause.o) \
	$(LIBRARY)(_pipe.o) \
	$(LIBRARY)(_ptrace.o) \
	$(LIBRARY)(_read.o) \
	$(LIBRARY)(_readdir.o) \
	$(LIBRARY)(_rename.o) \
	$(LIBRARY)(_rewinddir.o) \
	$(LIBRARY)(_rmdir.o) \
	$(LIBRARY)(_setgid.o) \
	$(LIBRARY)(_setuid.o) \
	$(LIBRARY)(_sigaction.o) \
	$(LIBRARY)(_sigpending.o) \
	$(LIBRARY)(_sigprocmask.o) \
	$(LIBRARY)(_sigreturn.o) \
	$(LIBRARY)(_sigset.o) \
	$(LIBRARY)(_sigsetjmp.o) \
	$(LIBRARY)(_sigsuspend.o) \
	$(LIBRARY)(_sleep.o) \
	$(LIBRARY)(_stat.o) \
	$(LIBRARY)(_stime.o) \
	$(LIBRARY)(_stty.o) \
	$(LIBRARY)(_sync.o) \
	$(LIBRARY)(_time.o) \
	$(LIBRARY)(_times.o) \
	$(LIBRARY)(_umask.o) \
	$(LIBRARY)(_umount.o) \
	$(LIBRARY)(_uname.o) \
	$(LIBRARY)(_unlink.o) \
	$(LIBRARY)(_utime.o) \
	$(LIBRARY)(_wait.o) \
	$(LIBRARY)(_waitpid.o) \
	$(LIBRARY)(_write.o) \

$(LIBRARY):	$(OBJECTS)
	aal cr $@ *.o
	rm *.o

$(LIBRARY)(__exit.o):	__exit.c
	$(CC1) __exit.c

$(LIBRARY)(_access.o):	_access.c
	$(CC1) _access.c

$(LIBRARY)(_alarm.o):	_alarm.c
	$(CC1) _alarm.c

$(LIBRARY)(_chdir.o):	_chdir.c
	$(CC1) _chdir.c

$(LIBRARY)(_chmod.o):	_chmod.c
	$(CC1) _chmod.c

$(LIBRARY)(_chown.o):	_chown.c
	$(CC1) _chown.c

$(LIBRARY)(_chroot.o):	_chroot.c
	$(CC1) _chroot.c

$(LIBRARY)(_close.o):	_close.c
	$(CC1) _close.c

$(LIBRARY)(_closedir.o):	_closedir.c
	$(CC1) _closedir.c

$(LIBRARY)(_creat.o):	_creat.c
	$(CC1) _creat.c

$(LIBRARY)(_dup.o):	_dup.c
	$(CC1) _dup.c

$(LIBRARY)(_dup2.o):	_dup2.c
	$(CC1) _dup2.c

$(LIBRARY)(_exec.o):	_exec.c
	$(CC1) _exec.c

$(LIBRARY)(_execn.o):	_execn.c
	$(CC1) _execn.c

$(LIBRARY)(_fcntl.o):	_fcntl.c
	$(CC1) _fcntl.c

$(LIBRARY)(_fork.o):	_fork.c
	$(CC1) _fork.c

$(LIBRARY)(_fpathconf.o):	_fpathconf.c
	$(CC1) _fpathconf.c

$(LIBRARY)(_fstat.o):	_fstat.c
	$(CC1) _fstat.c

$(LIBRARY)(_getcwd.o):	_getcwd.c
	$(CC1) _getcwd.c

$(LIBRARY)(_getegid.o):	_getegid.c
	$(CC1) _getegid.c

$(LIBRARY)(_geteuid.o):	_geteuid.c
	$(CC1) _geteuid.c

$(LIBRARY)(_getgid.o):	_getgid.c
	$(CC1) _getgid.c

$(LIBRARY)(_getgroups.o):	_getgroups.c
	$(CC1) _getgroups.c

$(LIBRARY)(_getpid.o):	_getpid.c
	$(CC1) _getpid.c

$(LIBRARY)(_getppid.o):	_getppid.c
	$(CC1) _getppid.c

$(LIBRARY)(_getuid.o):	_getuid.c
	$(CC1) _getuid.c

$(LIBRARY)(_gtty.o):	_gtty.c
	$(CC1) _gtty.c

$(LIBRARY)(_ioctl.o):	_ioctl.c
	$(CC1) _ioctl.c

$(LIBRARY)(_isatty.o):	_isatty.c
	$(CC1) _isatty.c

$(LIBRARY)(_kill.o):	_kill.c
	$(CC1) _kill.c

$(LIBRARY)(_link.o):	_link.c
	$(CC1) _link.c

$(LIBRARY)(_lseek.o):	_lseek.c
	$(CC1) _lseek.c

$(LIBRARY)(_mkdir.o):	_mkdir.c
	$(CC1) _mkdir.c

$(LIBRARY)(_mkfifo.o):	_mkfifo.c
	$(CC1) _mkfifo.c

$(LIBRARY)(_mknod.o):	_mknod.c
	$(CC1) _mknod.c

$(LIBRARY)(_mktemp.o):	_mktemp.c
	$(CC1) _mktemp.c

$(LIBRARY)(_mount.o):	_mount.c
	$(CC1) _mount.c

$(LIBRARY)(_open.o):	_open.c
	$(CC1) _open.c

$(LIBRARY)(_opendir.o):	_opendir.c
	$(CC1) _opendir.c

$(LIBRARY)(_pathconf.o):	_pathconf.c
	$(CC1) _pathconf.c

$(LIBRARY)(_pause.o):	_pause.c
	$(CC1) _pause.c

$(LIBRARY)(_pipe.o):	_pipe.c
	$(CC1) _pipe.c

$(LIBRARY)(_ptrace.o):	_ptrace.c
	$(CC1) _ptrace.c

$(LIBRARY)(_read.o):	_read.c
	$(CC1) _read.c

$(LIBRARY)(_readdir.o):	_readdir.c
	$(CC1) _readdir.c

$(LIBRARY)(_rename.o):	_rename.c
	$(CC1) _rename.c

$(LIBRARY)(_rewinddir.o):	_rewinddir.c
	$(CC1) _rewinddir.c

$(LIBRARY)(_rmdir.o):	_rmdir.c
	$(CC1) _rmdir.c

$(LIBRARY)(_setgid.o):	_setgid.c
	$(CC1) _setgid.c

$(LIBRARY)(_setuid.o):	_setuid.c
	$(CC1) _setuid.c

$(LIBRARY)(_sigaction.o):	_sigaction.c
	$(CC1) _sigaction.c

$(LIBRARY)(_sigpending.o):	_sigpending.c
	$(CC1) _sigpending.c

$(LIBRARY)(_sigprocmask.o):	_sigprocmask.c
	$(CC1) _sigprocmask.c

$(LIBRARY)(_sigreturn.o):	_sigreturn.c
	$(CC1) _sigreturn.c

$(LIBRARY)(_sigset.o):	_sigset.c
	$(CC1) _sigset.c

$(LIBRARY)(_sigsetjmp.o):	_sigsetjmp.c
	$(CC1) _sigsetjmp.c

$(LIBRARY)(_sigsuspend.o):	_sigsuspend.c
	$(CC1) _sigsuspend.c

$(LIBRARY)(_sleep.o):	_sleep.c
	$(CC1) _sleep.c

$(LIBRARY)(_stat.o):	_stat.c
	$(CC1) _stat.c

$(LIBRARY)(_stime.o):	_stime.c
	$(CC1) _stime.c

$(LIBRARY)(_stty.o):	_stty.c
	$(CC1) _stty.c

$(LIBRARY)(_sync.o):	_sync.c
	$(CC1) _sync.c

$(LIBRARY)(_time.o):	_time.c
	$(CC1) _time.c

$(LIBRARY)(_times.o):	_times.c
	$(CC1) _times.c

$(LIBRARY)(_umask.o):	_umask.c
	$(CC1) _umask.c

$(LIBRARY)(_umount.o):	_umount.c
	$(CC1) _umount.c

$(LIBRARY)(_uname.o):	_uname.c /usr/include/minix/config.h
	$(CC1) _uname.c

$(LIBRARY)(_unlink.o):	_unlink.c
	$(CC1) _unlink.c

$(LIBRARY)(_utime.o):	_utime.c
	$(CC1) _utime.c

$(LIBRARY)(_wait.o):	_wait.c
	$(CC1) _wait.c

$(LIBRARY)(_waitpid.o):	_waitpid.c
	$(CC1) _waitpid.c

$(LIBRARY)(_write.o):	_write.c
	$(CC1) _write.c
