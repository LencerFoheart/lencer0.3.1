/*
 * Small/kernel/panic.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * ���ļ������ں˳�����������������ʹ�ں�ͣ�ڴ˴���
 * NOTE! ��ѭ��������������!!!
 */

#include "kernel.h"

void panic(const char * str)
{
	_k_printf("Kernel panic: %s\n", str);

	for(;;)
		;
}