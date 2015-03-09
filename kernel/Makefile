#
# Small/kernel/Makefile
#
# (C) 2012-2013 Yafei Zheng <e9999e@163.com>
#

# --------------------------------------------------------------------
# 变量定义
AS		= 	as
LD		=	ld
ASEFLAGES	=
LDEFLAGES	=	-Ttext 0 -nostdlib

CC		=	gcc
# 注意：console.c编译时，若优化则会出错!!! 即使是 -O
# NOTE! 选项： -fomit-frame-pointer --- ERROR: can't find a register in class 'GENERAL_REGS' while reloading 'asm'.
CEFLAGES		=	-Wall -O0 -std=c99 -I../include  -nostdinc -fno-builtin -fomit-frame-pointer >> ../Warning.log 2>&1
# --------------------------------------------------------------------

# --------------------------------------------------------------------
all: kernel.o

OBJS  = system_call.o keyboard.o console.o kernel_printf.o panic.o sched.o \
	fork.o harddisk.o debug.o traps_c.o traps_s.o exit.o shutdown.o

kernel.o: $(OBJS)
	$(LD) -r -o $@ $(OBJS)

system_call.o: system_call.s
	$(AS) $(ASEFLAGES) -o $@ $<

keyboard.o: keyboard.c ../include/types.h ../include/keyboard.h ../include/string.h ../include/sys_set.h  \
	../include/console.h ../include/sys_nr.h ../include/keymap.h
	$(CC) $(CEFLAGES) -c $<

# 注意：console.c编译时，若优化则会出错!!! 即使是 -O
console.o: console.c ../include/sys_set.h ../include/console.h ../include/types.h ../include/string.h
	$(CC) $(CEFLAGES) -c $<

kernel_printf.o: kernel_printf.c ../include/stdio.h ../include/stdarg.h ../include/console.h \
	../include/sys_set.h ../include/string.h
	$(CC) $(CEFLAGES) -c $<
	
panic.o: panic.c ../include/kernel.h
	$(CC) $(CEFLAGES) -c $<

sched.o: sched.c ../include/sched.h ../include/sys_set.h ../include/types.h ../include/kernel.h ../include/sys_nr.h
	$(CC) $(CEFLAGES) -c $<

fork.o: fork.c ../include/sched.h ../include/memory.h ../include/string.h \
	../include/types.h ../include/kernel.h
	$(CC) $(CEFLAGES) -c $<

harddisk.o: harddisk.c ../include/harddisk.h ../include/sys_set.h ../include/sys_nr.h ../include/sched.h \
	../include/kernel.h ../include/types.h ../include/string.h
	$(CC) $(CEFLAGES) -c $<

debug.o: debug.c ../include/kernel.h ../include/file_system.h ../include/sched.h ../include/stdio.h \
	../include/string.h ../include/sys_set.h ../include/exec.h
	$(CC) $(CEFLAGES) -c $<

traps_c.o: traps.c ../include/sys_set.h ../include/sys_nr.h ../include/kernel.h ../include/sched.h \
	../include/string.h
	$(CC) $(CEFLAGES) -c -o $@ $<

traps_s.o: traps.s
	$(AS) $(ASEFLAGES) -o $@ $<
	
exit.o: exit.c
	$(CC) $(CEFLAGES) -c $<
	
shutdown.o: shutdown.c ../include/sys_set.h
	$(CC) $(CEFLAGES) -c $<

# --------------------------------------------------------------------

# --------------------------------------------------------------------
# 清除
.PHONY: clean
clean:
	-rm *.a *.o
# --------------------------------------------------------------------