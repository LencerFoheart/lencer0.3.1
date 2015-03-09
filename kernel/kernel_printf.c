/*
 * Small/kernel/kernel_printf.c
 *
 * (C) 2014 Yafei Zheng <e9999e@163.com>
 */
/*
 * 此文件包含了仅限于内核态使用的打印函数。注意：在内核中不要试图用指
 * 向用户地址空间的指针变量作为参数去打印用户空间的字符串，因为段不同。
 */

#include "stdio.h"
#include "stdarg.h"
#include "console.h"
#include "sys_set.h"
#include "string.h"

 
char k_printbuff[1024] = {0};	/* 内核态打印缓冲 */

/*
 * _k_printf(). 内核态打印函数,返回输出字符的个数.
 */
int _k_printf(char *fmt, ...)
{
	int count = 0;
	va_list vaptr = 0;
	unsigned short oldfs;

	va_start(vaptr, fmt);
	oldfs = set_fs_kernel();
	console_write(k_printbuff, count=vsprintf(k_printbuff, fmt, vaptr));
	set_fs(oldfs);
	va_end(vaptr);

	return count;
}

/*
 * k_printf(). 内核态打印函数,将打印出“Kernel ...: ”提示字样,
 * 返回输出字符的个数.
 */
int k_printf(char *fmt, ...)
{
	int count = 0;
	va_list vaptr = 0;
	unsigned short oldfs;
	char startmsg[] = "Kernel message: ";

	strcpy(k_printbuff, startmsg);
	va_start(vaptr, fmt);
	count = vsprintf(&k_printbuff[sizeof(startmsg)-1], fmt, vaptr);
	k_printbuff[count+sizeof(startmsg)-1] = '\n';
	oldfs = set_fs_kernel();
	console_write(k_printbuff, count+sizeof(startmsg));
	set_fs(oldfs);
	va_end(vaptr);

	return count;
}
