/*
 * Small/include/memory.h
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "types.h"

/********************************************************************/

#define PAGE_SIZE		(4096)			/* 4KB */
#define MAX_MEM_SIZE	(16*1024*1024)	/* 最大支持的物理内存大小 */
#define INIT_UPDE_COUNT	(1)	/* 初始化进程0的用户态页目录项数，与head.s中保持一致 */
#define MEM_MAP_SIZE	(MAX_MEM_SIZE/PAGE_SIZE)
#define USED			(100)					/* 页面已使用 */

#define M2MAP(paddr) ((paddr)/PAGE_SIZE)	/* 物理地址转为mem_map索引 */
#define MAP2M(i) ((i)*PAGE_SIZE)	/* mem_map索引转为物理地址 */
#define ALIGN_PAGE(vaddr) ((vaddr)&0xfffff000)	/* 使线性地址页对齐 */
#define ADDR_IN_1PT(base_1pt,vaddr) ((base_1pt)+(vaddr)/(PAGE_SIZE/4*PAGE_SIZE)*4)/* 线性地址在一级页表中的地址 */
#define ADDR_IN_2PT(base_2pt,vaddr) ((base_2pt)+((vaddr)%(PAGE_SIZE/4*PAGE_SIZE))/PAGE_SIZE*4)/* 线性地址在二级页表中的地址 */

/*
 * 以下是用户虚拟地址空间范围(0GB - 3GB)和内核虚拟地址空间范围(3GB - 4GB)。
 * NOTE! 以下声明用于内存管理，需与GDT,LDT等其他地方保持一致。
 */
#define V_USER_ZONE_START		(0x0)
#define V_USER_ZONE_END			(0xC0000000)
#define V_KERNEL_ZONE_START		(0xC0000000)
#define V_KERNEL_ZONE_END		(0x100000000)
#define PG_DIR_KERNEL_START		(768*4)	/* 与head.s同步 */

/* 页面属性标志，用于设置页表项。 */
#define P_UR	(4)	/* 用户态，只读，不存在 */
#define P_UW	(6)	/* 用户态，读写，不存在 */
#define P_SR	(0)	/* 内核态，只读，不存在 */
#define P_SW	(2)	/* 内核态，读写，不存在 */
#define P_URP	(5)	/* 用户态，只读，存在 */
#define P_UWP	(7)	/* 用户态，读写，存在 */
#define P_SRP	(1)	/* 内核态，只读，存在 */
#define P_SWP	(3)	/* 内核态，读写，存在 */

/* 使TLB无效 */
#define invalidate_tlb() ({ \
	__asm__( \
		"movl %cr3,%eax\n\t" \
		"movl %eax,%cr3\n\t" \
	); \
})

/********************************************************************/
extern unsigned long pg_dir[PAGE_SIZE/4];

extern unsigned long get_free_page(void);
extern unsigned long get_clean_page(void);
extern unsigned long get_mapped_free_page(unsigned long vaddr, unsigned long flag);
extern unsigned long get_mapped_clean_page(unsigned long vaddr, unsigned long flag);
extern BOOL copy_page_dir(long * cr3);
extern BOOL copy_kernel_page_dir(long * cr3);
extern BOOL put_page(unsigned long page, unsigned long vaddr, unsigned long flag);
extern void set_shared_pages(unsigned long from, unsigned long size);
extern void share_page_dir(long * cr3);
extern void free_shared_pages(unsigned long from, unsigned long size);
extern void free_user_pages(unsigned long from, unsigned long size, BOOL invtlb);
extern BOOL pre_replace_user_zone(void);
extern BOOL do_no_page(unsigned long vaddr);
extern BOOL un_page_wp(unsigned long vaddr);
extern void do_page_fault(unsigned long vaddr, unsigned long error);
extern void mem_init(void);

/********************************************************************/

#endif /* _MEMORY_H_ */
