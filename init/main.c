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
	/* NOTE! 这些初始化函数的位置别随便改动! */
	trap_init();	/* 在设置其他中断之前 */
	console_init();	/* 需打印的在这之后 */
	keyboard_init();
	hd_init();		/* 需读磁盘的放其后 */
/*	graph_init(); */
	mem_init();
	sched_init();
	buff_init();	/* 在内存初始化之后 */
	inode_init();
	file_table_init();
	debug_init();
	sti();
	super_init(0);	/* 开中断之后 */
	files_init();

/*	unsigned short color = rgb_to_565color(255,255,255);
	draw_rect(0, 0, 800, 600, color, 1); */

	move_to_user_mode();

	/*	 * 进程0马上execve()，替换掉用户态空间，这样进程0就
	 * 可以写时复制了。注意此时原来的用户态堆栈也被丢弃	 * 了，换成了新堆栈。更多参见内存管理。	 */
	if(!execve("/init"))
		printf("main: execve init-process failed.\n");

	return 0;
}
