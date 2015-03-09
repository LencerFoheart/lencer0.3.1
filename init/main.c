/*
 * Small/init/main.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
#ifndef __DEBUG__
#define __DEBUG__
#endif
*/

#include "sys_set.h"
#include "console.h"
#include "keyboard.h"
#include "sched.h"
#include "kernel.h"
#include "memory.h"
#include "harddisk.h"
#include "file_system.h"
#include "graph.h"
#include "libs.h"
#include "stdio.h"

#include "sys_call_table.h"

int main(void)
{
	/* NOTE! ��Щ��ʼ��������λ�ñ����Ķ�! */
	trap_init();	/* �����������ж�֮ǰ */
	console_init();	/* ���ӡ������֮�� */
	keyboard_init();
	hd_init();		/* ������̵ķ���� */
/*	graph_init(); */
	mem_init();
	sched_init();
	buff_init();	/* ���ڴ��ʼ��֮�� */
	inode_init();
	file_table_init();
	debug_init();
	sti();
	super_init(0);	/* ���ж�֮�� */
	files_init();

/*	unsigned short color = rgb_to_565color(255,255,255);
	draw_rect(0, 0, 800, 600, color, 1); */

	move_to_user_mode();

	/*	 * ����0����execve()���滻���û�̬�ռ䣬��������0��
	 * ����дʱ�����ˡ�ע���ʱԭ�����û�̬��ջҲ������	 * �ˣ��������¶�ջ������μ��ڴ����	 */
	if(!execve("/init"))
		printf("main: execve init-process failed.\n");

	return 0;
}
