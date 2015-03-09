/*
 * Small/fs/buffer.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * 本文件包含磁盘高速缓冲区的相关处理程序。
 *
 * 缓冲区的起始地址是内核代码和数据的结束处。NOTE! 缓冲区的起始地址
 * 和结束地址均在mm中设置。
 *
 * 缓冲区通过缓冲块空闲表和哈希表来管理。空闲表是由头指针指向的双向
 * 循环链表组成，哈希表是由头指针数组以及每个头指针指向的双向循环链
 * 表组成。一个缓冲区在哈希表中，则意味着它有效，在空闲表中，则意味
 * 着它空闲。一个缓冲区可同时位于空闲表和哈希表，此时意味着它空闲且
 * 有效 ...
 *
 * [??] 还没有实现异步读写硬盘，异步读写时getblk里将会有不同。
 */

/*
#ifndef __DEBUG__
#define __DEBUG__
#endif
*/

#include "file_system.h"
#include "harddisk.h"
#include "kernel.h"
#include "string.h"
#include "sched.h"
#include "types.h"
#include "sys_set.h"

/*
 * 缓冲区开始和结束的物理地址。
 * NOTE! 已在mm中初始化，故不能在本文件中再次初始化!!!
 */
unsigned long buffer_mem_start = 0, buffer_mem_end = 0;

/* 缓冲区空闲表头部指针 */
static struct buffer_head * free_head = NULL;
/* 缓冲区哈希表头部指针数组 */
static struct buffer_head * hash_heads[NR_BUFF_HASH] = {0};

/* 指向等待缓冲区的进程 */
struct proc_struct * buff_wait = NULL;


/*
 * 该函数为哈希表散列函数。根据设备号和块号，返回其在哈希表
 * 数组中的索引。NOTE! 该函数并不知道该块是否存在于散列表。
 */
int b_hash_fun(unsigned char dev, unsigned long nrblock)
{
	return (nrblock % NR_BUFF_HASH);
}

/*
 * 该函数检查哈希表，若指定块存在于哈希表，则返回指向该缓冲区的指针，
 * 否则，返回NULL。由于设备号和逻辑硬盘块号从0开始，而初始化缓冲区头
 * 部时将它们也初始化为0，因此判断缓冲区是否在哈希表，要判断其有效位。
 */
struct buffer_head * b_scan_hash(unsigned char dev, unsigned long nrblock)
{
	int index = b_hash_fun(dev, nrblock);
	struct buffer_head *p = hash_heads[index];

	while(p) {
		if((p->state&BUFF_VAL) && p->dev==dev && p->nrblock==nrblock)
			return p;
		p = p->p_hash_next;
		if(p == hash_heads[index])
			break;
	}
	return NULL;
}

/*
 * 该函数从空闲表中摘取一个空闲缓冲区。输入若为空，
 * 则从头部摘取一个，否则根据输入的缓冲区指针摘取。
 */
struct buffer_head * b_get_from_free(struct buffer_head *p)
{
	if(p) {				/* 从表中摘下指定的空闲缓冲区 */
		if(p->p_free_next == p) {/* 表中只有一个缓冲区 */
			free_head = NULL;
		} else {
			p->p_free_pre->p_free_next = p->p_free_next;
			p->p_free_next->p_free_pre = p->p_free_pre;
			if(p == free_head) {		/* p指向表头 */
				free_head = p->p_free_next;
			}
		}
	} else {
		if(NULL == free_head) {	/* 表空 */
			p = NULL;
		} else if(free_head->p_free_next == free_head) {/* 表中只有一个缓冲区 */
			p = free_head;
			free_head = NULL;
		} else {					/* 表中有多个缓冲区 */
			p = free_head;
			p->p_free_pre->p_free_next = p->p_free_next;
			p->p_free_next->p_free_pre = p->p_free_pre;
			free_head = p->p_free_next;
		}
	}
	return p;
}

/*
 * 该函数将空闲缓冲区放入空闲表中。若其含有有效数据，则将其
 * 放入空闲表尾部，否则放入空闲表头部，以便下次直接使用它。
 */
void b_put_to_free(struct buffer_head *p, BOOL ishead)
{
	if(!p)
		panic("b_put_to_free: pointer is NULL.");

	if(NULL == free_head) {	/* 空闲表空 */
		free_head = p;
		p->p_free_next = p;
		p->p_free_pre = p;
	} else {
		if(ishead) {			/* 放在头部 */
			p->p_free_next = free_head;
			p->p_free_pre = free_head->p_free_pre;
			free_head->p_free_pre->p_free_next = p;
			free_head->p_free_pre = p;
			free_head = p;
		} else {				/* 放在末尾 */
			p->p_free_pre = free_head->p_free_pre;
			free_head->p_free_pre->p_free_next = p;
			p->p_free_next = free_head;
			free_head->p_free_pre = p;
		}
	}
}

/*
 * 从哈希表移除。我们不能将设备号等置为NULL，因为设备号
 * 等可能为0，实际上只要其有效位清除即可，参见scan_hash().
 */
void b_remove_from_hash(struct buffer_head *p)
{
	if(!p)
		panic("b_remove_from_hash: pointer is NULL.");
	if(!b_scan_hash(p->dev, p->nrblock))
		panic("b_remove_from_hash: not in hash-table.");

	int index = b_hash_fun(p->dev, p->nrblock);

	if(p->p_hash_next == p) {	/* 表中只有一个缓冲区 */
		hash_heads[index] = NULL;
	} else {
		p->p_hash_pre->p_hash_next = p->p_hash_next;
		p->p_hash_next->p_hash_pre = p->p_hash_pre;
		if(p == hash_heads[index]) {		/* p指向表头 */
			hash_heads[index] = p->p_hash_next;
		}
	}
}

/* * 该函数将指定缓冲区放入到哈希表中，同时会设置缓冲区的
 * 设备号以及磁盘块号。 */
void b_put_to_hash(struct buffer_head *p, 
		unsigned char newdev, unsigned long newnrblock)
{
	if(!p)
		panic("b_put_to_hash: pointer is NULL.");

	int index = b_hash_fun(newdev, newnrblock);

	p->dev = newdev;
	p->nrblock = newnrblock;
	if(NULL == hash_heads[index]) {
		hash_heads[index] = p;
		p->p_hash_next = p;
		p->p_hash_pre = p;
	} else {
		p->p_hash_next = hash_heads[index];
		p->p_hash_pre = hash_heads[index]->p_hash_pre;
		hash_heads[index]->p_hash_pre->p_hash_next = p;
		hash_heads[index]->p_hash_pre = p;
		hash_heads[index] = p;
	}
}

/* * 该函数根据设备号和硬盘块号获取指定缓冲区。它返回含有有效数据的缓冲
 * 区指针。注意：缓冲区释放之后就进入空闲表，但若数据有效，也会同时存
 * 在于哈希表中。
 */
struct buffer_head * getblk(unsigned char dev, unsigned long nrblock)
{
	struct buffer_head * p = NULL;

	while(1) {
		if((p = b_scan_hash(dev, nrblock))) {/* 在散列表 */
			/* 在散列表，但上锁 */
			if(p->state & BUFF_LOCK) {
				sleep_on(&buff_wait, INTERRUPTIBLE);
				continue;		/* 回到while循环 */
			}
			/* 在散列表，且空闲 */
			LOCK_BUFF(p);
			b_get_from_free(p);
			return p;
		} else {/* 不在散列表 */
			/* 不在散列表，且空闲表空 */
			if(NULL == free_head) {
				sleep_on(&buff_wait, INTERRUPTIBLE);
				continue;		/* 回到while循环 */
			}
			/* 不在散列表，在空闲表找到一个缓冲区，但标记了延迟写 */
			p = b_get_from_free(NULL);
			if(p->state & BUFF_DELAY_W) {
				/*
				 * 不需要考虑缓冲是否加锁，因为空闲表中的缓冲区是不可能
				 * 处于加锁状态的。
				 *
				 * 此处应该是异步写，但异步写还没实现呢，暂且同步写吧。
				 * 同步写的话，我们将其写入磁盘之后，就拿它直接去用了，
				 * 只是效率不够好。异步写的话，情况就不一样了，我们需要
				 * 继续回到while循环，而不能直接使用该缓冲，同时该缓冲
				 * 被我们从空闲表上摘下来了，因此还需考虑要不要把它放回
				 * 空闲表呢...或许情况很复杂 :-)
				 */
				bwrite(p);
			}
			/* 不在散列表，在空闲表找到一个缓冲区 */
			LOCK_BUFF(p);
			if(p->state & BUFF_VAL) {
				b_remove_from_hash(p);
				p->state &= (~BUFF_VAL) & 0xff;
			}
			b_put_to_hash(p, dev, nrblock);
			return p;
		}
	}
}

/* * 该函数将指定缓冲区释放。 */
void brelse(struct buffer_head *p)
{
	if(!p)
		panic("brelse: the m-inode-pointer can't be NULL.");
	
	if(p->state & BUFF_VAL) {
		b_put_to_free(p, 0);
	} else {
		/*		 * “从哈希表移除”函数会检查其是否在哈希表，而有效性是确定其在
		 * 哈希表的因素之一，因此我们暂时让它有效，移除之后再使之无效。		 */
		p->state |= BUFF_VAL;
		b_remove_from_hash(p);
		p->state &= (~BUFF_VAL) & 0xff;
		b_put_to_free(p, 1);
	}
	UNLOCK_BUFF(p);
}

/* * 该函数读取指定磁盘块数据。 */
struct buffer_head * bread(unsigned char dev, unsigned long nrblock)
{
	struct buffer_head *p = getblk(dev, nrblock);
	if(!(p->state&BUFF_VAL)) {	/* 无效，则启动磁盘读 */
		insert_hd_request(p->p_data, WIN_READ, dev, nrblock, 1);
		p->state |= BUFF_VAL;
	}
	return p;
}

/* * 该函数将数据写入指定磁盘块。 */
void bwrite(struct buffer_head *p)
{
	if(!p)
		panic("bwrite: the m-inode-pointer can't be NULL.");
	
	if(!(p->state&BUFF_LOCK))
		LOCK_BUFF(p);
	/* 启动磁盘写 */
	insert_hd_request(p->p_data, WIN_WRITE, p->dev, p->nrblock, 1);
	if(p->state & BUFF_DELAY_W) {
		p->state &= (~BUFF_DELAY_W) & 0xff;
		UNLOCK_BUFF(p);
	} else {
		brelse(p);
	}
}

/*
 * 缓冲区初始化需在内存初始化之后被调用。
 * [??] buffer-struct等在内存中字节对齐处理。
 */
void buff_init(void)
{
	int buffers;				/* 缓冲区数 */
	struct buffer_head * p;		/* 当前要处理缓冲区 */
	struct buffer_head * plast;	/* 前一个缓冲区 */
	unsigned long tmp;

	/* 最后减1是为了避免，在640kb处无法存储完整的缓冲头部或缓冲数据区域 */
	buffers = (buffer_mem_end - buffer_mem_start - (1*1024*1024-640*1024))
			/ (BUFFER_SIZE + sizeof(struct buffer_head)) - 1;
	p = (struct buffer_head *)buffer_mem_start;
	plast = NULL;
	free_head = p;
	buff_wait = NULL;

	d_printf("buff_head+BUFFER_SIZE = %d\n buffers = %d\n",
		sizeof(struct buffer_head)+BUFFER_SIZE, buffers);

	/* 建立双向循环空闲缓冲区链表 */
	while(buffers--) {
		/*
		 * 存在这样一种可能性：当p未达到640KB，而p->p_free_next 
		 * 已超过了，因此我们要探测这种情况，并做出反应。
		 */
		tmp = (unsigned long)p + sizeof(struct buffer_head) + BUFFER_SIZE;
		/* 当前缓冲区进入[640KB,1MB]区域 */
		if(tmp>640*1024 && tmp<1*1024*1024) {
			p = (struct buffer_head *)(1*1024*1024);
			/*
			 * Well! Here, we need to change the pointer of
			 * last-buffer. I debug it for a long time :-)
			 */
			NULL==plast ? 0 : (plast->p_free_next=p);
			NULL==plast ? (free_head=p) : 0;
		}

		d_printf("[1] p = %d, p->p_free_next = %d\n",
			(unsigned long)p, (unsigned long)p->p_free_next);

		zeromem(p, sizeof(struct buffer_head));
		p->p_free_next = (struct buffer_head *)((unsigned long)p 
			+ sizeof(struct buffer_head) + BUFFER_SIZE);
		p->p_data = (unsigned char *)((unsigned long)p 
			+ sizeof(struct buffer_head));
		p->p_free_pre = plast;
		
		plast = p;
		p = p->p_free_next;

		d_printf("[2] p = %d, p->p_free_next = %d\n", 
			(unsigned long)p, (unsigned long)p->p_free_next);

		d_printf("next_free_buff_head_addr = %d\n", p->p_free_next);
		d_delay(1000);
	}
	plast->p_free_next = free_head;
	free_head->p_free_pre = plast;

	/* 初始化哈希链表 */
	/* nothing.在bss段,已被清零 */
}
