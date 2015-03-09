#
# Small/mm/Makefile
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
all: mm.o

OBJS = memory.o

mm.o: $(OBJS)
	$(LD) -r -o $@ $(OBJS)

memory.o: memory.c ../include/memory.h ../include/kernel.h ../include/string.h \
	../include/types.h ../include/sched.h ../include/sys_nr.h
	$(CC) $(CEFLAGES) -c $<
# --------------------------------------------------------------------

# --------------------------------------------------------------------
# 清除
.PHONY: clean
clean:
	-rm *.a *.o
# --------------------------------------------------------------------