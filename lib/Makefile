#
# Small/lib/Makefile
#
# (C) 2012-2013 Yafei Zheng <e9999e@163.com>
#

# --------------------------------------------------------------------
# 变量定义

AR		=	ar
CC		=	gcc
# NOTE! 选项： -fomit-frame-pointer --- ERROR: can't find a register in class 'GENERAL_REGS' while reloading 'asm'.
CEFLAGES		=	-Wall -O0 -std=c99 -I../include  -nostdinc -fno-builtin -fomit-frame-pointer >> ../Warning.log 2>&1
# --------------------------------------------------------------------

# --------------------------------------------------------------------
all: lib.a

OBJS = string.o stdio.o libs.o

lib.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

string.o: string.c ../include/string.h
	$(CC) $(CEFLAGES) -c $<

stdio.o: stdio.c ../include/stdarg.h ../include/libs.h
	$(CC) $(CEFLAGES) -c $<
	
libs.o: libs.c ../include/types.h ../include/sys_nr.h
	$(CC) $(CEFLAGES) -c $<
# --------------------------------------------------------------------

# --------------------------------------------------------------------
.PHONY: clean

clean:
	-rm *.o *.a
# --------------------------------------------------------------------