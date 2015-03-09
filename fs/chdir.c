/*
 * Small/fs/chdir.c
 *
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * 该文件用于改变当前进程的“工作目录”或者“根目录”。
 */

/*
#ifndef __DEBUG__
#define __DEBUG__
#endif
*/

#include "file_system.h"
#include "sched.h"
#include "types.h"

BOOL check_change_permit(struct m_inode * p)
{
	return (p->mode&FILE_DIR);
}

/*
 * 改变当前进程的当前工作目录。
 * I: 新目录名, O: true - ret TRUE, false - ret FALSE.
 */
BOOL sys_chdir(char * dirname)
{
	struct m_inode * p = namei(dirname);
	if(!p)
		return FALSE;
	if(!check_change_permit(p)) {
		iput(p);
		return FALSE;
	}
	UNLOCK_INODE(p);
	iput(current->current_dir);
	current->current_dir = p;
	return TRUE;
}

/*
 * 改变当前进程的根目录。
 * I: 新目录名, O: true - ret TRUE, false - ret FALSE.
 */
BOOL sys_chroot(char * dirname)
{
	struct m_inode * p = namei(dirname);
	if(!p)
		return FALSE;
	if(!check_change_permit(p)) {
		iput(p);
		return FALSE;
	}
	UNLOCK_INODE(p);
	iput(current->root_dir);
	current->root_dir = p;
	return TRUE;
}
