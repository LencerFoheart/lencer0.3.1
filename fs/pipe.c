/*
 * Small/fs/pipe.c
 *
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * ���ļ������ܵ����ֵ�ʵ�֣����������ܵ��������ܵ�����Ϊ�����ں�
 * Ĭ��ֻ��һ���ļ�ϵͳ��������Ǿ�������ļ�ϵͳ�ϴ����ܵ�������
 * �����ٻ��ֹܵ��ļ�ϵͳ��
 *
 * [??] �ܵ�����Ŀǰֻʵ���˴��������ܵ������������ܵ��Լ������ܵ�
 * �Ķ�д���رյȣ�Ŀǰ��ûʵ�֣���Ϊ��û������������Ҹ��ð� :-)
 */

/*
#ifndef __DEBUG__
#define __DEBUG__
#endif
*/

#include "file_system.h"
#include "sched.h"
#include "kernel.h"
#include "string.h"
#include "types.h"

/*
 * Create no-name-pipeline.
 */
BOOL sys_pipe(struct pipe_rw_desc * p_rw)
{
	struct m_inode * p;
	struct pipe_rw_desc tmp;
	
	if(!p_rw) {
		k_printf("pipe: pipe-structure is NULL.");
		return FALSE;
	}
	/* ���������ڵ���ļ����ʧ�����˻� */
	if(!(p=ialloc(0)))
		goto pipe_false_ret_1;
	if(-1 == (tmp.rdesc = find_file_table(p, O_RONLY)))
		goto pipe_false_ret_2;
	if(-1 == (tmp.wdesc = find_file_table(p, O_WONLY)))
		goto pipe_false_ret_3;
	/*
	 * ���ڹܵ��ļ������ǽ����ڴ�ڵ����ü�������Ϊ2��
	 * ʵ���������޸ģ���Ϊ��iget()�У������Ѿ����ù��ˡ�
	 */
	p->count = 2;
	/*
	 * �������ڵ������ע���ٴ�ʹ��ʱ��Ҫ�ȼ�����ʹ���������
	 */
	UNLOCK_INODE(p);
	
	ds2fs_memcpy(p_rw, &tmp, sizeof(struct pipe_rw_desc));
	return TRUE;

pipe_false_ret_3:
	current->file[tmp.rdesc]->count = 0;
	current->file[tmp.rdesc] = NULL;
pipe_false_ret_2:
	ifree(p->dev, p->nrinode);
	iput(p);
pipe_false_ret_1:
	tmp.rdesc = -1;
	tmp.wdesc = -1;
	ds2fs_memcpy(p_rw, &tmp, sizeof(struct pipe_rw_desc));
	return FALSE;
}
