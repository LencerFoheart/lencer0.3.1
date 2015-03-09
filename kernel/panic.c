/*
 * Small/kernel/panic.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * 此文件用于内核出现致命错误，其结果是使内核停在此处。
 * NOTE! 死循环，但不是死机!!!
 */

#include "kernel.h"

void panic(const char * str)
{
	_k_printf("Kernel panic: %s\n", str);

	for(;;)
		;
}