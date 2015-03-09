/*
 * Small/include/sys_call_table.h
 * 
 * (C) 2014 Yafei Zheng <e9999e@163.com>
 */

#ifndef _SYS_CALL_TABLE_H_
#define _SYS_CALL_TABLE_H_

#include "kernel.h"
#include "sched.h"
#include "file_system.h"
#include "exec.h"
#include "console.h"

/********************************************************************/
/* 以下定义系统调用函数入口表。此处需与系统调用对应函数标号同步!!! */
void * sys_call_table[] = {
	sys_fork,		/* 0 */
	sys_pause,		/* 1 */
	sys_exit,		/* 2 */
	sys_execve,		/* 3 */
	sys_open,		/* 4 */
	sys_creat,		/* 5 */
	sys_read,		/* 6 */
	sys_write,		/* 7 */
	sys_lseek,		/* 8 */
	sys_close,		/* 9 */
	sys_dup,		/* 10 */
	sys_fstat,		/* 11 */
	sys_stat,		/* 12 */
	sys_chdir,		/* 13 */
	sys_chroot,		/* 14 */
	sys_link,		/* 15 */
	sys_unlink,		/* 16 */
	sys_pipe,		/* 17 */
	sys_sync,		/* 18 */
	sys_shutdown,	/* 19 */
	sys_restart,	/* 20 */
	console_write,	/* 21 */
};
/********************************************************************/

#endif /* _SYS_CALL_TABLE_H_ */
