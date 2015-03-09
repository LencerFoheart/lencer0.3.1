/*
 * Small/fs/sync.c
 * 
 * (C) 2014 Yafei Zheng <e9999e@163.com>
 */

/*
 * 该文件将内存中的文件系统相关数据(超级块、索引节点、高速缓冲)同步
 * 至文件系统。用于正常情况下和非正常情况下(panic)的文件系统同步，
 * 因此对于非正常情况，内核不能关闭系统文件表中的文件。
 *
 * [??] 要不要加锁、加锁的要不要写入、延迟还是立即写、panic下是否其他
 * 进程可以运行，这样就可以等待解锁了......考虑的是如此之多之多，
 * 先放放 :-((((((
 */

/*
#ifndef __DEBUG__
#define __DEBUG__
#endif
*/

#include "file_system.h"

void sys_sync(unsigned char dev)
{
	/* 写超级块 */
	write_super(dev);
	/* 索引节点 */
	
	/* 高速缓冲 */
}
