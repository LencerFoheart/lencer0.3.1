/*
 * Small/include/stdio.h
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

#ifndef _STDIO_H_
#define _STDIO_H_
/********************************************************************/
#include "stdarg.h"

extern int itoa(char *buff, int value, int base, unsigned int flag);
extern int vsprintf(char *buff, char *fmt, va_list args);
extern int sprintf(char *buff, char *fmt, ...);
extern int printf(char *fmt, ...);

/********************************************************************/
#endif /* _STDIO_H_ */
