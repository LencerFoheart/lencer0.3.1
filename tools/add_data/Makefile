#
# Small/tools/add_data/Makefile
#
# (C) 2014 Yafei Zheng <e9999e@163.com>
#
# This is the Makefile for user-test-files.
#

CC			=	gcc
CEFLAGES	=	-Wall -O0 -I../../include -nostdinc -fno-builtin >> ../../Warning.log 2>&1
LD			=	ld
LDEFLAGES	=	-s -x -nostdlib -e _main -M

all: test init

test: test.c ../../include/libs.h ../../include/stdio.h ../../include/string.h
	$(CC) $(CEFLAGES) -c test.c
	$(LD) $(LDEFLAGES) test.o ../../lib/lib.a -o _test > test.map
	strip -s -o test _test
	readelf -a test > readelf_test.txt
	rm _test test.o

init: init.c ../../include/libs.h
	$(CC) $(CEFLAGES) -c init.c
	$(LD) $(LDEFLAGES) init.o ../../lib/lib.a -o _init > init.map
	strip -s -o init _init
	readelf -a init > readelf_init.txt
	rm _init init.o

clean:
	-rm test readelf_test.txt test.map \
		init readelf_init.txt init.map
