/*
 * Small/kernel/kernel_printf.c
 *
 * (C) 2014 Yafei Zheng <e9999e@163.com>
 */
/*
 * ���ļ������˽������ں�̬ʹ�õĴ�ӡ������ע�⣺���ں��в�Ҫ��ͼ��ָ
 * ���û���ַ�ռ��ָ�������Ϊ����ȥ��ӡ�û��ռ���ַ�������Ϊ�β�ͬ��
 */

#include "stdio.h"
#include "stdarg.h"
#include "console.h"
#include "sys_set.h"
#include "string.h"

 
char k_printbuff[1024] = {0};	/* �ں�̬��ӡ���� */

/*
 * _k_printf(). �ں�̬��ӡ����,��������ַ��ĸ���.
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
 * k_printf(). �ں�̬��ӡ����,����ӡ����Kernel ...: ����ʾ����,
 * ��������ַ��ĸ���.
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
