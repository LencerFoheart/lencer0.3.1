/*
 * Small/mm/memory.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * 此文件用于对内存进行管理。支持的物理内存大小参见头文件。
 * 此文件也对高速缓冲区的开始和结束地址进行了初始化。
 *
 * 内存管理允许进程共享页目录、二级页表、普通页面，因此它们都依赖于
 * 页面引用计数。
 *
 * 进程0的代码混在内核代码中(main()中)，这就导致它的用户态页表会映射
 * 部分内核代码，若此时创建进程，将导致对内核代码也进行写保护。同时也
 * 会因为用户态页表映射了页表本身，从而导致共享内存时页表引用数的增加
 * 量不正确。它们是很恐怖的。我们的做法是刚开始执行进程0，马上execve()，
 * 替换掉用户空间，从此进程0的代码就与内核代码划清了界限，用户态页表也
 * 不再映射本身。接下来就可以放心创建进程了。所有进程也都将具有写时复制
 * 机制。
 */

/*
#ifndef __DEBUG__
#define __DEBUG__
#endif
*/

#include "memory.h"
#include "string.h"
#include "kernel.h"
#include "sched.h"
#include "sys_nr.h"
#include "types.h"

/* 以下是链接器的预定义变量 */
extern unsigned long _etext;
extern unsigned long _end;

/* 缓冲区起始地址 */
extern unsigned long buffer_mem_start;
/* 缓冲区末端地址 */
extern unsigned long buffer_mem_end;

/* UNION(初始化进程结构, 进程内核态堆栈) */
extern union proc_union init_proc;

/* 内存高端地址 */
unsigned long high_mem = 0;
/*
 * 内存页面使用情况表。映射内存范围：0MB - 16MB。
 * 其值表示该页引用次数，0表示该页空闲。
 */
unsigned char mem_map[MEM_MAP_SIZE] = {0};

/*
 * 该函数倒序查找内存使用表，取得一个内存空闲页面，并将内存使用表
 * 相应项加1。返回页面起始物理地址，若为NULL，则表示无空闲页面。
 */
unsigned long get_free_page(void)
{
	unsigned long i;
	for(i=M2MAP(high_mem)-1; i>=M2MAP(buffer_mem_end); i--) {	
		if(0 == mem_map[i]) {
			++mem_map[i];
			break;
		}
	}
	return i>=M2MAP(buffer_mem_end) ? MAP2M(i) : NULL;
}

/*
 * 该函数获取指定页面并且清0。返回获取的页面的物理地址。
 */
unsigned long get_clean_page(void)
{
	unsigned long addr = get_free_page();
	if(addr)
		zeromem((char *)addr, PAGE_SIZE);
	return addr;
}

/*
 * 该函数获取一个空闲页面并且放入页表。返回获取的页面的物理地址。
 */
unsigned long get_mapped_free_page(unsigned long vaddr, unsigned long flag)
{
	unsigned long addr = get_free_page();
	if(addr && !put_page(addr, vaddr, flag))
		return NULL;
	else
		return addr;
}

/*
 * 该函数获取一个清0的空闲页面并且放入页表。返回获取的页面的物理地址。
 */
unsigned long get_mapped_clean_page(unsigned long vaddr, unsigned long flag)
{
	unsigned long addr = get_clean_page();
	if(addr && !put_page(addr, vaddr, flag))
		return NULL;
	else
		return addr;
}

/*
 * 该函数申请一个页面作为页目录表，并复制整个页目录表。
 */
BOOL copy_page_dir(long * cr3)
{
	long tmp;
	if(!cr3)
		panic("copy_page_dir: cr3-pointer is NULL!");
	if(NULL == (tmp=get_clean_page())) {
		k_printf("copy_page_dir: have no free-page!");
		return FALSE;
	}
	memcpy((char *)tmp, (char *)current->tss.cr3, PAGE_SIZE);
	*cr3 = tmp;
	return TRUE;
}

/*
 * 该函数申请一个页面作为页目录表，并复制内核态页目录表项。
 */
BOOL copy_kernel_page_dir(long * cr3)
{
	long tmp;
	if(!cr3)
		panic("copy_kernel_page_dir: cr3-pointer is NULL!");
	if(NULL == (tmp=get_clean_page())) {
		k_printf("copy_kernel_page_dir: have no free-page!");
		return FALSE;
	}
	memcpy((char *)(tmp+PG_DIR_KERNEL_START),
		(char *)(current->tss.cr3+PG_DIR_KERNEL_START),
		PAGE_SIZE-PG_DIR_KERNEL_START);
	*cr3 = tmp;
	return TRUE;
}

/*
 * 该函数将页表与内存页面挂接。
 * I: page - 页面起始物理地址;
 *	  vaddr - 页面映射的虚拟线性地址;
 * 	  flag - 页面属性标志.
 * O: 若错误，返回FALSE.
 */
BOOL put_page(unsigned long page, unsigned long vaddr, unsigned long flag)
{
	if(page<buffer_mem_end || page>(high_mem-PAGE_SIZE))
		panic("put_page: page address out of the limit.");
	if(vaddr > (V_USER_ZONE_END-PAGE_SIZE))
		panic("put_page: vaddr out of the limit.");
	if(page & (PAGE_SIZE-1))	/* 内存页面起始地址不是PAGE_SIZE对齐 */
		panic("put_page: page address with wrong alignment.");
	if(vaddr & (PAGE_SIZE-1))	/* 虚拟线性地址不是PAGE_SIZE对齐 */
		panic("put_page: vaddr with wrong alignment.");
	if(flag!=P_URP && flag!=P_UWP && flag!=P_SRP && flag!=P_SWP)
		panic("put_page: ERROR page-flag.");

	/* 将要放置的页目录表项的地址 */
	unsigned long put_addr = ADDR_IN_1PT(current->tss.cr3, vaddr);
	/* 页目录表项中P位是否置位 */
	if(!(*(unsigned long *)put_addr & 1)) {
		unsigned long tmp;
		if(NULL == (tmp = get_free_page()))
			return FALSE;
		*(unsigned long *)put_addr = tmp + P_UWP; /* 用户态，读写 */
	}
	/* 得到二级页表所在页面的首地址 */
	put_addr = ALIGN_PAGE(*(unsigned long *)put_addr);
	/* 得到二级页表项的地址 */
	put_addr = ADDR_IN_2PT(put_addr, vaddr);
	*(unsigned long *)put_addr = page + flag;

	return TRUE;
}

/*
 * 该函数将指定用户态线性地址空间对应的页面以及
 * 二级页表使用次数加1，并设置页面属性为只读。
 * I: from - 起始线性地址; size - 内存空间大小.
 */
void set_shared_pages(unsigned long from, unsigned long size)
{
	if(from & (PAGE_SIZE-1))
		panic("set_shared_pages: start address with wrong alignment.");
	size = ALIGN_PAGE(size+PAGE_SIZE-1);
	if(from + size > V_USER_ZONE_END)
		panic("set_shared_pages: trying to set kernel address space.");

	/* 处理二级页表项指向的页面 */
	unsigned long to, addr, tmp;
	for(to=from+size; from<to; from+=PAGE_SIZE) {
		addr = ADDR_IN_1PT(current->tss.cr3, from);
		if(*(unsigned long *)addr & 1) {
			addr = ALIGN_PAGE(*(unsigned long *)addr);
			addr = ADDR_IN_2PT(addr, from);
			if(*(unsigned long *)addr & 1) {
				tmp = ALIGN_PAGE(*(unsigned long *)addr);
				++mem_map[M2MAP(tmp)];
				*(unsigned long *)addr = tmp + P_URP;
			}
		}
	}
	
	/* 处理二级页表页面, from 4MB向下取整, to 4MB向上取整 */
	from = (to-size) & 0xffc00000;
	to = (to+PAGE_SIZE/4*PAGE_SIZE-1) & 0xffc00000;
	for(; from<to; from+=PAGE_SIZE/4*PAGE_SIZE) {
		addr = ADDR_IN_1PT(current->tss.cr3, from);
		if(*(unsigned long *)addr & 1) {
			++mem_map[M2MAP(ALIGN_PAGE(*(unsigned long *)addr))];
		}
	}
}

/*
 * 该函数共享页目录表，也就是共享了进程全部内存。虽然再次设置代
 * 码段只读是没必要的，但增加代码段内存页面的引用次数却是必要的。
 */
void share_page_dir(long * cr3)
{
	if(!cr3)
		panic("share_page_dir: cr3-pointer is NULL!");
	*cr3 = current->tss.cr3;
	++mem_map[M2MAP(*cr3)];
	set_shared_pages(current->code_start,
		current->data_end-current->code_start);
	invalidate_tlb();
}

/*
 * 该函数将指定用户态线性地址空间对应的页面以及二级页表使用次数减1，
 * 若其上一级页表的引用数为1，则将其在上一级页表中的索引位置置为NULL.
 * I: from - 起始线性地址; size - 内存空间大小.
 */
void free_shared_pages(unsigned long from, unsigned long size)
{
	if(from & (PAGE_SIZE-1))
		panic("free_shared_pages: start address with wrong alignment.");
	size = ALIGN_PAGE(size+PAGE_SIZE-1);
	if(from+size > V_USER_ZONE_END)
		panic("free_shared_pages: trying to free kernel address space.");

	/* 处理二级页表项指向的页面 */
	unsigned long to, addr, tmp;
	for(to=from+size; from<to; from+=PAGE_SIZE) {
		addr = ADDR_IN_1PT(current->tss.cr3, from);
		if(*(unsigned long *)addr & 1) {
			tmp = ALIGN_PAGE(*(unsigned long *)addr);
			addr = ADDR_IN_2PT(tmp, from);
			if(*(unsigned long *)addr & 1) {
				--mem_map[M2MAP(ALIGN_PAGE(*(unsigned long *)addr))];
				if(1 == mem_map[M2MAP(tmp)])
					*(unsigned long *)addr = NULL;
			}
		}
	}

	/* 处理二级页表页面 */
	unsigned long begin, end;
	from = to - size;
	for(; from<to; from+=PAGE_SIZE/4*PAGE_SIZE, from&=0xffc00000) {
		if(!(from&(PAGE_SIZE/4*PAGE_SIZE-1))
		&& from+PAGE_SIZE/4*PAGE_SIZE<=to) { /* 4MB对齐,且不超出范围 */
			begin = from;
		} else {
			if(!(from & (PAGE_SIZE/4*PAGE_SIZE-1))) {
				begin = from & 0xffc00000;
				end = from;
			} else {	/* from+PAGE_SIZE/4*PAGE_SIZE > to */
				begin = to;
				end = (to+PAGE_SIZE/4*PAGE_SIZE) & 0xffc00000;
			}
			/* 遍历其余二级页表项，若全部为空，则需将该页表引用数减1 */
			for(; begin<end; begin+=PAGE_SIZE) {
				addr = ADDR_IN_1PT(current->tss.cr3, begin);
				if(*(unsigned long *)addr & 1) {
					addr = ALIGN_PAGE(*(unsigned long *)addr);
					addr = ADDR_IN_2PT(addr, begin);
					if(*(unsigned long *)addr & 1)
						break;
				}
			}
			if(begin < end)
				continue;
			begin = from & 0xffc00000;
		}
		/* 二级页表页面引用数减1 */
		addr = ADDR_IN_1PT(current->tss.cr3, begin);
		if(*(unsigned long *)addr & 1) {
			--mem_map[M2MAP(ALIGN_PAGE(*(unsigned long *)addr))];
			if(1 == mem_map[M2MAP(current->tss.cr3)])
				*(unsigned long *)addr = NULL;
		}
	}
}

/*
 * 该函数释放进程用户态内存空间。关于释放进程内存空间，需明确以下几点：
 *
 * 对于进程0，释放用户态内存则意味着其刚开始执行execve()，此时只需将其
 * 用户态页目录项清零即可。不能释放，因为0进程与内核共用页表!
 *
 * 由于各个进程共用一套内核态的二级页表页面，因此不能释放内核空间。
 *
 * 同时，并不释放页目录表所在页面，因为进程当前运行于内核态，依然依赖
 * 于内核地址空间。而在该进程挂掉之后，由其父进程释放页目录表。那么0号
 * 进程的父进程是谁呢？别想它了，除非系统关闭，否则0号进程是不会灭亡滴 :-)
 *
 * 由于可能存在多个进程在使用当前页目录表(实际上在创建新进程时，子进程
 * 默认与父进程具有相同的CR3)，因此释放进程页目录表时，只需将引用数减1。
 *
 * 另外，我们需要使TLB无效，以保持TLB的一致性(幸好Intel CPU会负责维护
 * Cache与内存的一致性)。但有时(比如当前进程退出时)，我们无需使TLB无效，
 * 因为它没有机会再去运行，很快其父进程就会将其页目录回收掉。
 *
 * I：from - 起始线性地址, size - 内存空间大小, invtlb - 是否使TLB无效.
 */
void free_user_pages(unsigned long from, unsigned long size, BOOL invtlb)
{
	if(0 == get_cur_proc_nr())
		zeromem((char *)current->tss.cr3, 4*INIT_UPDE_COUNT);
	else
		free_shared_pages(from, size);
	if(invtlb)
		invalidate_tlb();
}

/* * 该函数为替换用户态空间做准备。首先释放用户态内存，然后检查页目录
 * 使用次数，若是共享的，则分配一个新的页目录，并将原页目录引用数减1。 */
BOOL pre_replace_user_zone(void)
{
	free_user_pages(current->code_start,
		current->data_end-current->code_start, 1);
	if(1 < mem_map[M2MAP(current->tss.cr3)]) {
		unsigned long addr = current->tss.cr3;
		if(!copy_kernel_page_dir(&(current->tss.cr3))) {
			k_printf("free_user_zone: have no free-page!");
			return FALSE;
		}
		load_cr3(current->tss.cr3);
		--mem_map[M2MAP(addr)];
		d_printf("set cr3 finished, cr3 = %x.\n", current->tss.cr3);
	}
	return TRUE;
}

/* * 缺页处理。注意：缺页意味着进程在执行新的可执行文件或者在申请内存空间。
 * 若是申请内存空间，那么它可能没有自己独立的页目录表(因为创建进程时页目
 * 录表也是共享的)，因此，我们还要考虑是否为其中某些进程分配页目录表。
 * 二级页表页面同样需考虑。 */
BOOL do_no_page(unsigned long vaddr)
{
	panic("do_no_page: page fault.");
	return TRUE;
}

/*
 * 该函数取消页面写保护。注意：写保护意味着进程共享了内存空间，故它们
 * 可能没有自己独立的页目录表(因为创建进程时页目录表也是共享的)，因此，
 * 我们还要考虑是否为其中某些进程分配页目录表。二级页表页面同样需考虑。
 * I: vaddr - 发生页保护中断的页面的虚拟地址.
 */
BOOL un_page_wp(unsigned long vaddr)
{
	unsigned long addr, tmp;

	if(vaddr >= V_KERNEL_ZONE_START)	/* 用于调试内核代码 */
		panic("un_page_wp: BAD, write to kernel address space.");
	if(vaddr < current->code_start || vaddr >= current->data_end
		|| (vaddr >= current->code_end && vaddr < current->data_start))
		panic("un_page_wp: out of code or data space limit.");

	/* 进程还没有自己独立的页目录表，则分配 */
	if(1 < mem_map[M2MAP(current->tss.cr3)]) {
		addr = current->tss.cr3;
		if(!copy_page_dir(&(current->tss.cr3))) {
			k_printf("un_page_wp: have no free-page!");
			return FALSE;
		}
		load_cr3(current->tss.cr3);
		--mem_map[M2MAP(addr)];
	}
	addr = ADDR_IN_1PT(current->tss.cr3, vaddr);

	/* 进程还没有自己独立的二级页表页面，则分配 */
	if(1 < mem_map[M2MAP(ALIGN_PAGE(*(unsigned long *)addr))]) {
		/* get_free_page而不是clean，因为接下来会复制整个页面 */
		if(NULL == (tmp=get_free_page())) {
			k_printf("un_page_wp: have no free-page!");
			return FALSE;
		}
		memcpy((char *)tmp, (char *)ALIGN_PAGE(*(unsigned long *)addr),
			PAGE_SIZE);
		--mem_map[M2MAP(ALIGN_PAGE(*(unsigned long *)addr))];
		*(unsigned long *)addr = tmp + P_UWP;
	}
	addr = ALIGN_PAGE(*(unsigned long *)addr);
	addr = ADDR_IN_2PT(addr, vaddr);

	/* 最终页面引用次数为1则设置其可写，否则分配独立页面 */
	if(1 == mem_map[M2MAP(ALIGN_PAGE(*(unsigned long *)addr))]) {
		*(unsigned long *)addr |= 0x00000002;	/* 设置页面可写 */
	} else {
		if(NULL == (tmp=get_free_page())) {
			k_printf("un_page_wp: have no enough memory!");
			return FALSE;
		}
		memcpy((char *)tmp, (char *)ALIGN_PAGE(*(unsigned long *)addr),
			PAGE_SIZE);
		--mem_map[M2MAP(ALIGN_PAGE(*(unsigned long *)addr))];
		*(unsigned long *)addr = tmp + P_UWP;
	}

	/* 刷新TLB */
	invalidate_tlb();

	return TRUE;
}

/*
 * 该函数处理页保护中断。包括页写保护以及缺页。
 * I: vaddr - 发生页保护中断的页面的虚拟地址; error - 错误码.
 *
 * 错误码各位的含义：
 * 	位2(U/S) - 0 表示在超级用户模式下执行，1 表示在用户模式下执行；
 * 	位1(W/R) - 0 表示读操作，1 表示写操作；
 * 	位0(P) - 0 表示页不存在，1 表示页级保护。
 */
void do_page_fault(unsigned long vaddr, unsigned long error)
{
	if(!(error & 1)) {	/* 页面不存在 */
		if(!do_no_page(vaddr)) {
			/* [??] nothing at present */
		}
	} else if(error & 2) {	/* 页面存在但不可写 */
		if(!un_page_wp(vaddr)) {
			/* [??] nothing at present */
		}
	} else {
		panic("do_page_fault: unknown page fault.");
	}
}

void mem_init(void)
{
	/* 初始化进程0的代码段和数据段起止地址 */
	init_proc.proc.code_start = 0;
	init_proc.proc.code_end = (unsigned long)&_etext;
	init_proc.proc.data_start = (unsigned long)&_etext;
	init_proc.proc.data_end = (unsigned long)&_end;

	/* 内存布局 */	
	/* 0x90002地址处保存着扩展内存大小 */
	high_mem = (*(unsigned short *)0x90002+1024)*1024;
	d_printf("memory-size: %d Bytes.\n", high_mem);
	high_mem = ALIGN_PAGE(high_mem);
	if(high_mem >= 16*1024*1024)
		high_mem = 16*1024*1024;
	if(high_mem < 10*1024*1024)
		panic("the size of memory can't below 10MB.");
	buffer_mem_end = 3*1024*1024;
	buffer_mem_start = (unsigned long)&_end;
	
	/* 初始化mem_map */
	memset(mem_map, (unsigned char)USED, MEM_MAP_SIZE);
	/* NOTE! (&mem_map)+M2MAP(buffer_mem_end) 是错误的，翻翻C语法吧 */
	zeromem(&mem_map[M2MAP(buffer_mem_end)],
		(high_mem-buffer_mem_end)/PAGE_SIZE);
	/*
	 * 将init进程(进程0)页目录表使用次数设置为1，因为
	 * 页写保护等内存管理机制依赖于页目录等的使用次数。
	 */
	mem_map[M2MAP(init_proc.proc.tss.cr3)] = 1;
}
