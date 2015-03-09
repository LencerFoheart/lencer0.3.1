#
# Small/Makefile
#
# (C) 2012-2013 Yafei Zheng <e9999e@163.com>
#

# --------------------------------------------------------------------
# 变量定义
AS86 		=	as86
LD86 		= 	ld86
AS86EFLAGES	=	-0 -a
LD86EFLAGES	=	-0 -s

AS		= 	as
LD		=	ld
ASEFLAGES	=
# -M选项用于生成map文件
# -N选项取消数据段页对齐，否则会出错（初始化的全局变量访问错误）。
LDEFLAGES	=	-N -s -x -Ttext 0 -e startup_32 -M -nostdlib

CC		=	gcc
# 不使用gcc自带标准头文件，故此编译配置不适用于tools目录
# c99标准支持在for循环初始化语句中定义变量
# -fno-builtin 选项，关闭GCC的内置函数替换功能，比如禁止将 printf("abc\n") 替换为 puts("abc")
# NOTE! 选项： -fomit-frame-pointer --- ERROR: can't find a register in class 'GENERAL_REGS' while reloading 'asm'.
# >> Warning.log 2>&1 将gcc的标准错误输出和标准输出重定向追加到Warning.log
CEFLAGES		=	-Wall -O0 -std=c99 -Iinclude -nostdinc -fno-builtin -fomit-frame-pointer >> Warning.log 2>&1
# NOTE：tools目录使用gcc库文件，故不使用include目录作为头文件目录
TOOLS_CEFLAGES	=	-Wall -O0 -std=c99 >> Warning.log 2>&1


KERNEL = kernel/kernel.o
LIBS = lib/lib.a
FS = fs/fs.o
MM = mm/mm.o
GRAPH = graph/graph.o
OTHERS =  kernel/kernel.o  lib/lib.a  fs/fs.o  mm/mm.o  graph/graph.o
# kernel, lib, fs, mm, graph, include
__OTHERS =  kernel/system_call.s kernel/keyboard.c kernel/console.c kernel/kernel_printf.c kernel/panic.c \
			kernel/sched.c kernel/fork.c kernel/harddisk.c kernel/debug.c kernel/traps.c kernel/traps.s \
			kernel/exit.c kernel/shutdown.c \
			lib/stdio.c lib/string.c lib/libs.c \
			fs/buffer.c fs/inode.c fs/alloc.c fs/read_write.c fs/exec.c \
			fs/chdir.c fs/link_mount.c fs/pipe.c fs/stat.c fs/sync.c \
			mm/memory.c \
			graph/g_simple.c \
			include/console.h include/elf.h include/file_system.h include/graph.h include/harddisk.h \
			include/kernel.h include/keyboard.h include/keymap.h include/memory.h include/sched.h \
			include/stdarg.h include/stdio.h include/string.h include/sys_call_table.h include/sys_nr.h \
			include/sys_set.h include/types.h include/exec.h include/libs.h
# --------------------------------------------------------------------

# --------------------------------------------------------------------
# Image为OS镜像，rootImage为根文件系统镜像
all: Image rootImage Codelines.log

Image: boot/boot tools/system tools/build
	tools/build

rootImage: tools/mkfs tools/add_data/test tools/add_data/init
	tools/mkfs

Codelines.log:
	wc -l `find . -name *.c; find . -name *.h; find . -name *.s` > Codelines.log
	
tools/build: tools/build.c
	$(CC) $(TOOLS_CEFLAGES) -o $@ $<
tools/mkfs: tools/mkfs.c include/file_system.h include/kernel.h
	$(CC) $(TOOLS_CEFLAGES) -o $@ $<

tools/add_data/test tools/add_data/init: $(LIBS) tools/add_data/test.c tools/add_data/init.c \
	include/libs.h include/stdio.h include/string.h
	(cd tools/add_data; make)

boot/boot: boot/boot.s
	$(AS86) $(AS86EFLAGES) -o boot/boot.o boot/boot.s
	$(LD86) $(LD86EFLAGES) -o boot/boot boot/boot.o

# 以下使用 LD 连接，而不用 CC。命令中各个模块出现的顺序若不正确，则会导致连接错误。
tools/system: boot/head.o init/main.o $(OTHERS)
	$(LD) $(LDEFLAGES) \
	boot/head.o init/main.o $(OTHERS) \
	-o tools/system > System.map

$(KERNEL): $(__OTHERS)
	(cd kernel; make)
	
$(LIBS): $(__OTHERS)
	(cd lib; make)
	
$(FS): $(__OTHERS)
	(cd fs; make)
	
$(MM): $(__OTHERS)
	(cd mm; make)
	
$(GRAPH): $(__OTHERS)
	(cd graph; make)

init/main.o: init/main.c include/sys_set.h include/console.h include/keyboard.h \
	 include/sched.h include/kernel.h include/memory.h include/harddisk.h \
	 include/file_system.h include/graph.h include/exec.h include/sys_call_table.h \
	 include/libs.h include/stdio.h
	$(CC) $(CEFLAGES) -c $< -o $@

boot/head.o: boot/head.s
	$(AS) $(ASEFLAGES) -o boot/head.o boot/head.s
# --------------------------------------------------------------------

# --------------------------------------------------------------------
# 清除
.PHONY: clean

clean:
	-rm Image System.map rootImage Warning.log Codelines.log \
	boot/boot boot/*.o \
	tools/build tools/system tools/mkfs \
	init/main.o
	(cd kernel; make clean)
	(cd lib; make clean)
	(cd mm; make clean)
	(cd fs; make clean)
	(cd graph; make clean)
	(cd tools/add_data; make clean)
	(cd TEST_RUN/Bochs; make clean)
	(cd TEST_RUN/VMware; make clean)
# --------------------------------------------------------------------