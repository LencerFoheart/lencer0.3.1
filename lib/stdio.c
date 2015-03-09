/*
 * Small/lib/stdio.c
 *
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */
/*
 * 该文件包含了一些内核态和用户态都可以使用的例程以及仅限于用户态使
 * 用的例程 - printf()。在使用该文件中的程序时，需注意：使用那些可
 * 用于内核态，也可用于用户态的程序时，必须确保不同时使用内核地址空
 * 间和用户地址空间，因为它们默认指针参数都指向同一段中。
 */

#include "stdarg.h"
#include "libs.h"

#define CAPS		(1<<0)	/* 大写 */
#define SIGNED		(1<<1)	/* 有符号 */
#define PERCENT		(1<<7)	/* 百分号标志 */

/*
 * 将整数转换为字符串。I：buff - 转换之后缓冲区指针; value - 欲转换整数;
 * base - 进制; flag - 转换标志。O：转换之后的字符个数
 */
int itoa(char *buff, int value, int base, unsigned int flag)
{
	char dicta[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	char dictA[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int index = 0;			/* buff 的索引 */
	char stack[20] = {0};	/* 用于10进制数的解析 */

	if(16 == base) {	/* 16进制 */
		index = 0;
		buff[index++] = '0';
		buff[index++] = ((flag & CAPS) ? 'X' : 'x');
		for(int i=sizeof(int)*2; i>0; i--) {	/* 需倒着 */
			/*
			 * 将其右移，将得到的低4位的数作为字典数组的索引，
			 * 得到4位二进制对应的十六进制数.
			 */
			buff[index++] = ((flag&CAPS) 
				? dictA[(value>>((i-1)*4)) & 0xf] 
				: dicta[(value>>((i-1)*4)) & 0xf]);
		}
	} else if(10 == base) {	/* 10进制 */
		index = 0;
		if(flag & SIGNED) {	/* 有符号，则判断是否需加负号“-” */
			(value & (1 << (sizeof(int)*8-1))) ? (buff[index++] = '-') : 0;
		}

		int j = 0;
		while(0 != value) {
			/*
			 * 注意：若是负数，则将其转为正数进行取余，因负数取余会
			 * 得到意想不到的结果。但除法运算正负数通用。
			 */
			/* 另外，注意 & 比 % 的优先级低!!! */
			if(value & (1 << (sizeof(int)*8-1))) {	/* 最高位为1 */
				stack[j++] = dicta[(flag & SIGNED) ? (((~((value&0x7fff)-1))&0x7fff) % 10) : (((unsigned int)value) % 10)];
			} else {	/* 最高位为0，不可能是负数 */
				stack[j++] = dicta[(flag & SIGNED) ? ((value) % 10) : (((unsigned int)value) % 10)];
			}
			value = (flag & SIGNED) ? ((value) / 10) : (((unsigned int)value) / 10);
		}
		if(0 == j)
			buff[index++] = '0';
		while(0 != j)
			buff[index++] = stack[--j];
	} else {
		index = 0;
		/* 没做任何事! */
	}
	buff[index] = 0;

	return index;
}

/*
 * F: 格式化字符串。目前支持：
 *		-- %d(有符号10进制)
 *		-- %u(无符号10进制)
 *		-- %x(无符号小写16进制)
 *		-- %X(无符号大写16进制)
 *		-- %c(单字符)
 *		-- %s(字符串)
 *		-- %%(%本身)
 * I: buff - 存储缓冲区指针; fmt - 欲格式化字符串指针; args - ...
 * O: 格式化之后的缓冲区字符数
 */
int vsprintf(char *buff, char *fmt, va_list args)
{
	int count = 0;			/* 格式化之后的缓冲区字符数 */
	unsigned short flag = 0;	/* 转换标志 */
	char *p = fmt;
	char *tmp = 0;			/* 用于 %s */

	while(*p) {
		switch(*p) {
		case '%':	/* % 和 %% */
			(flag & PERCENT) 
				? (buff[count++] = *p, flag &= (0xffff ^ PERCENT))
				: (flag |= PERCENT);
			break;
		case 'd':	/* d 和 %d */
			if(!(flag & PERCENT)) {
				buff[count++] = *p;
				break;
			}
			flag |= SIGNED;
			count += itoa(&buff[count], va_arg(args, int), 10, flag);
			flag &= (0xffff ^ SIGNED);
			flag &= (0xffff ^ PERCENT);	/* 注意：每次都要取消 %号 标志 */
			break;
		case 'u':	/* u 和 %u */
			if(!(flag & PERCENT)) {
				buff[count++] = *p;
				break;
			}
			count += itoa(&buff[count], va_arg(args, int), 10, flag);
			flag &= (0xffff ^ PERCENT);
			break;
		case 'x':	/* x 和 %x */
			if(!(flag & PERCENT)) {
				buff[count++] = *p;
				break;
			}
			count += itoa(&buff[count], va_arg(args, int), 16, flag);
			flag &= (0xffff ^ PERCENT);
			break;
		case 'X':	/* X 和 %X */
			if(!(flag & PERCENT)) {
				buff[count++] = *p;
				break;
			}
			flag |= CAPS;
			count += itoa(&buff[count], va_arg(args, int), 16, flag);
			flag &= (0xffff ^ CAPS);
			flag &= (0xffff ^ PERCENT);
			break;
		case 'c':	/* c 和 %c */
			if(!(flag & PERCENT)) {
				buff[count++] = *p;
				break;
			}
			buff[count++] = va_arg(args, char);
			flag &= (0xffff ^ PERCENT);
			break;
		case 's':	/* s 和 %s */
			if(!(flag & PERCENT)) {
				buff[count++] = *p;
				break;
			}
			tmp = va_arg(args, char *);
			while(*tmp) {
				buff[count++] = *tmp;
				tmp++;
			}
			flag &= (0xffff ^ PERCENT);
			break;
		default:	/* 其他 */
			buff[count++] = *p;
			break;
		}
		p++;
	}
	buff[count] = 0;

	return count;
}

/*
 * sprintf(). 返回格式化字符的个数.
 */
int sprintf(char *buff, char *fmt, ...)
{
	int count = 0;
	va_list vaptr = 0;

	va_start(vaptr, fmt);
	count=vsprintf(buff, fmt, vaptr);
	va_end(vaptr);

	return count;
}

/*
 * 用户态打印缓冲。用户程序使用它，编译链接后就会存在于用户程序中。
 */
char printbuff[1024] = {0};
/*
 * printf(). 用户态打印函数,返回输出字符的个数.
 */
int printf(char *fmt, ...)
{
	int count = 0;
	va_list vaptr = 0;

	va_start(vaptr, fmt);
	/* 这是个用户态程序，在libs中定义 */
	_console_write(printbuff, count=vsprintf(printbuff, fmt, vaptr));
	va_end(vaptr);

	return count;
}
