/*
 * Small/fs/chdir.c
 *
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * ���ļ����ڸı䵱ǰ���̵ġ�����Ŀ¼�����ߡ���Ŀ¼����
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
 * �ı䵱ǰ���̵ĵ�ǰ����Ŀ¼��
 * I: ��Ŀ¼��, O: true - ret TRUE, false - ret FALSE.
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
 * �ı䵱ǰ���̵ĸ�Ŀ¼��
 * I: ��Ŀ¼��, O: true - ret TRUE, false - ret FALSE.
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
