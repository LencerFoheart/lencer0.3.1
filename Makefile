#
# Small/Makefile
#
# (C) 2012-2013 Yafei Zheng <e9999e@163.com>
#

# --------------------------------------------------------------------
# ��������
AS86 		=	as86
LD86 		= 	ld86
AS86EFLAGES	=	-0 -a
LD86EFLAGES	=	-0 -s

AS		= 	as
LD		=	ld
ASEFLAGES	=
# -Mѡ����������map�ļ�
# -Nѡ��ȡ�����ݶ�ҳ���룬����������ʼ����ȫ�ֱ������ʴ��󣩡�
LDEFLAGES	=	-N -s -x -Ttext 0 -e startup_32 -M -nostdlib

CC		=	gcc
# ��ʹ��gcc�Դ���׼ͷ�ļ����ʴ˱������ò�������toolsĿ¼
# c99��׼֧����forѭ����ʼ������ж������
# -fno-builtin ѡ��ر�GCC�����ú����滻���ܣ������ֹ�� printf("abc\n") �滻Ϊ puts("abc")
# NOTE! ѡ� -fomit-frame-pointer --- ERROR: can't find a register in class 'GENERAL_REGS' while reloading 'asm'.
# >> Warning.log 2>&1 ��gcc�ı�׼��������ͱ�׼����ض���׷�ӵ�Warning.log
CEFLAGES		=	-Wall -O0 -std=c99 -Iinclude -nostdinc -fno-builtin -fomit-frame-pointer >> Warning.log 2>&1
# NOTE��toolsĿ¼ʹ��gcc���ļ����ʲ�ʹ��includeĿ¼��Ϊͷ�ļ�Ŀ¼
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
# ImageΪOS����rootImageΪ���ļ�ϵͳ����
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

# ����ʹ�� LD ���ӣ������� CC�������и���ģ����ֵ�˳��������ȷ����ᵼ�����Ӵ���
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
# ���
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