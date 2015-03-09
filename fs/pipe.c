/*
 * Small/fs/pipe.c
 *
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * 该文件包含管道部分的实现，包括无名管道和有名管道。因为本版内核
 * 默认只有一个文件系统，因此我们就在这个文件系统上创建管道，而不
 * 单独再划分管道文件系统。
 *
 * [??] 管道部分目前只实现了创建无名管道，关于有名管道以及无名管道
 * 的读写、关闭等，目前还没实现，因为还没考虑清楚，暂且搁置吧 :-)
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
	/* 分配索引节点和文件表项，失败则退回 */
	if(!(p=ialloc(0)))
		goto pipe_false_ret_1;
	if(-1 == (tmp.rdesc = find_file_table(p, O_RONLY)))
		goto pipe_false_ret_2;
	if(-1 == (tmp.wdesc = find_file_table(p, O_WONLY)))
		goto pipe_false_ret_3;
	/*
	 * 对于管道文件，我们将其内存节点引用计数设置为2。
	 * 实际上这是修改，因为在iget()中，我们已经设置过了。
	 */
	p->count = 2;
	/*
	 * 将索引节点解锁。注意再次使用时，要先加锁，使用完解锁。
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
