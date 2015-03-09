/*
 * Small/fs/stat.c
 *
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */
/*
 * 该文件根据文件描述符或者文件路径名获取文件相关信息。
 */

/*
#ifndef __DEBUG__
#define __DEBUG__
#endif
*/

#include "file_system.h"
#include "sched.h"
#include "kernel.h"
#include "types.h"
#include "string.h"

void copy_file_info(struct file_info * f_info, struct m_inode * p)
{
	struct file_info tmp;
	
	tmp.mode		= p->mode;
	tmp.create_t	= p->create_t;
	tmp.use_t		= p->use_t;
	tmp.alter_t		= p->alter_t;
	tmp.i_alter_t	= p->i_alter_t;
	tmp.nlinks		= p->nlinks;
	tmp.size		= p->size;
	
	ds2fs_memcpy(f_info, &tmp, sizeof(struct file_info));
}

/*
 * 根据用户文件描述符获取文件的相关信息。
 */
BOOL sys_fstat(long desc, struct file_info * f_info)
{
	if(!f_info) {
		k_printf("fstat: information-buffer is NULL.");
		return FALSE;
	}
	struct file_table_struct * p_file = current->file[desc];
	if(!p_file) {
		k_printf("fstat: trying to get unknow-file state.");
		return FALSE;
	}
	struct m_inode * p = p_file->inode;
	if(!p)
		panic("fstat: BAD BAD, file-desc is OK, but m-inode-pointer is NULL!");
	/* 此处无需加锁，因为它不会引起睡眠，而内核态不允许调度 */
	copy_file_info(f_info, p);
	return TRUE;
}

/*
 * 根据文件路径名获取文件的相关信息。
 */
BOOL sys_stat(char * name, struct file_info * f_info)
{
	if(!f_info) {
		k_printf("stat: information-buffer is NULL.");
		return FALSE;
	}
	struct m_inode * p = namei(name);
	if(!p) {
		k_printf("stat: trying to get unknow-file state.");
		return FALSE;
	}
	copy_file_info(f_info, p);
	iput(p);
	return TRUE;
}
