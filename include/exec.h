/*
 * Small/include/exec.h
 * 
 * (C) 2014 Yafei Zheng <e9999e@163.com>
 */

#ifndef _EXEC_H_
#define _EXEC_H_

#include "types.h"

/********************************************************************/

#define MAX_USER_STACK_PAGES	(1)	/* 用户程序栈占用的最大页数 */

/* 保存可执行文件的信息 */
struct exec_info {
	/* [??] 最好复制到内核空间，放在exec_info页面吧，因为用户空间一旦替换，就不见了 */
	char * filename;		/* 可执行文件名，指向用户空间 */
	FILEDESC desc;			/* 可执行文件描述符 */
	unsigned long ventry;	/* 程序虚拟入口地址 */
};

extern BOOL sys_execve(char * file);

/********************************************************************/

#endif /* _EXEC_H_ */
