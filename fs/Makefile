#
# Small/fs/Makefile
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
# c99标准支持在for循环初始化语句中定义变量
# -fno-builtin 选项，关闭GCC的内置函数替换功能，比如禁止将 printf("abc\n") 替换为 puts("abc")
# NOTE! 选项： -fomit-frame-pointer --- ERROR: can't find a register in class 'GENERAL_REGS' while reloading 'asm'.
CEFLAGES		=	-Wall -O0 -std=c99 -I../include -nostdinc -fno-builtin -fomit-frame-pointer >> ../Warning.log 2>&1
# --------------------------------------------------------------------

# --------------------------------------------------------------------
all: fs.o

OBJS = buffer.o inode.o alloc.o read_write.o exec.o chdir.o link_mount.o pipe.o stat.o sync.o

fs.o: $(OBJS)
	$(LD) -r -o $@ $(OBJS)

buffer.o: buffer.c ../include/file_system.h ../include/harddisk.h ../include/kernel.h \
	../include/string.h ../include/sched.h ../include/types.h ../include/sys_set.h
	$(CC) $(CEFLAGES) -c $<

inode.o: inode.c ../include/file_system.h ../include/types.h ../include/string.h ../include/sched.h \
	../include/kernel.h ../include/sys_set.h
	$(CC) $(CEFLAGES) -c $<

alloc.o: alloc.c ../include/file_system.h ../include/kernel.h ../include/string.h \
	../include/types.h ../include/sched.h
	$(CC) $(CEFLAGES) -c $<

read_write.o: read_write.c ../include/file_system.h ../include/kernel.h ../include/string.h \
	../include/types.h ../include/sched.h ../include/sys_set.h
	$(CC) $(CEFLAGES) -c $<

exec.o: exec.c ../include/file_system.h ../include/elf.h ../include/exec.h ../include/sched.h ../include/types.h \
	../include/kernel.h ../include/memory.h ../include/sys_set.h
	$(CC) $(CEFLAGES) -c $<

chdir.o: chdir.c ../include/file_system.h ../include/sched.h ../include/types.h
	$(CC) $(CEFLAGES) -c $<

link_mount.o: link_mount.c ../include/file_system.h ../include/sched.h ../include/kernel.h \
	../include/types.h ../include/string.h ../include/sys_set.h
	$(CC) $(CEFLAGES) -c $<

pipe.o: pipe.c ../include/file_system.h ../include/sched.h ../include/kernel.h ../include/string.h \
	../include/types.h
	$(CC) $(CEFLAGES) -c $<

stat.o: stat.c ../include/file_system.h ../include/sched.h ../include/kernel.h \
	../include/types.h ../include/string.h
	$(CC) $(CEFLAGES) -c $<

sync.o: sync.c ../include/file_system.h
	$(CC) $(CEFLAGES) -c $<
	
# --------------------------------------------------------------------

# --------------------------------------------------------------------
# 清除
.PHONY: clean
clean:
	-rm *.a *.o
# --------------------------------------------------------------------