/*
 * Small/tools/add_data/test.c
 *
 * (C) 2014 Yafei Zheng <e9999e@163.com>
 *
 * 此程序使用内核系统调用来打印一串字符，不使用库函数。用于测试内核。
 *
 * 主函数名为“_main”而不是“main”，因为main只不过是C语言里一个重要的
 * 标号而已，它是给编译器看的。实际上一般情况下C程序经过编译链接后，
 * main并不是程序的真正入口，在GCC中真正入口为“_start”，_start会做
 * 一些初始化工作，然后调用main函数执行，而main一旦返回，_start就会
 * 将main的返回值作为退出码调用exit函数，最终导致进程的结束执行。
 * 这里我们不打算让GCC做初始化工作，因为我们的内核还不支持运行GCC的
 * 这些初始化例程。区分一般的main，我们用_main代替之。这里_main是程
 * 序的真正入口，由于没有其他函数调用它，我们自己调用exit代替return。
 *
 * 为了不包含GCC的初始化例程，让_main做程序的真正入口，需用到GCC和LD。
 * 命令如下：
 * 		-- 编译:				gcc -c test.c
 *		-- 链接:				ld -e _main test.o -o _test
 * 		-- 减小可执行文件尺寸:	strip -s -o test _test
 * 		-- ELF文件信息读取:		readelf -a test > readelf_test.txt
 *		-- 删除中间文件:		rm _test test.o
 *
 * 以下命令会包含GCC的初始化例程，因此目前不使用它们。
 *
 * 静态编译等命令:
 * 		-- 编译链接:			gcc -static -o _s_test test.c
 * 		-- 减小可执行文件尺寸:	strip -s -o s_test _s_test
 * 		-- ELF文件信息读取:		readelf -a s_test > s_readelf_test.txt
 *		-- 删除中间文件:		rm _s_test
 *
 * 动态编译等命令:
 * 		-- 编译链接:			gcc -o _d_test test.c
 * 		-- 减小可执行文件尺寸:	strip -s -o d_test _d_test
 * 		-- ELF文件信息读取:		readelf -a d_test > d_readelf_test.txt
 *		-- 删除中间文件:		rm _d_test
 */

#include "libs.h"
#include "stdio.h"
#include "string.h"

void _main(void)
{
	char s[] = "\n\tHello world!\n";

	printf(s, strlen(s));

	for(;;)
		;
}
