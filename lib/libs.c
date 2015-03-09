/*
 * Small/lib/libs.c
 *
 * (C) 2014 Yafei Zheng <e9999e@163.com>
 */
/*
 * 此文件包含一些用户态使用的库函数。
 */

#include "types.h"
#include "sys_nr.h"


/* 没有参数的 */
syscall_0(int, fork)
syscall_0(void, pause)
syscall_0(void, shutdown)
syscall_0(void, restart)


/* 1个参数的 */
syscall_1(BOOL, execve, char *, file)
syscall_1(long, close, long, desc)


/* 2个参数的 */
syscall_2(long, open, char *, pathname, unsigned short, mode)
syscall_2(long, creat, char *, pathname, unsigned short, mode)
syscall_2(void, _console_write, char *, buff, long, count)


/* 3个参数的 */
syscall_3(long, read, long, desc, char *, buff, unsigned long, count)
syscall_3(long, write, long, desc, char *, buff, unsigned long, count)
syscall_3(long, lseek, long, desc, long, offset, unsigned char, reference)
