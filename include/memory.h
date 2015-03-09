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
#define MAX_MEM_SIZE	(16*1024*1024)	/* ���֧�ֵ������ڴ��С */
#define INIT_UPDE_COUNT	(1)	/* ��ʼ������0���û�̬ҳĿ¼��������head.s�б���һ�� */
#define MEM_MAP_SIZE	(MAX_MEM_SIZE/PAGE_SIZE)
#define USED			(100)					/* ҳ����ʹ�� */

#define M2MAP(paddr) ((paddr)/PAGE_SIZE)	/* �����ַתΪmem_map���� */
#define MAP2M(i) ((i)*PAGE_SIZE)	/* mem_map����תΪ�����ַ */
#define ALIGN_PAGE(vaddr) ((vaddr)&0xfffff000)	/* ʹ���Ե�ַҳ���� */
#define ADDR_IN_1PT(base_1pt,vaddr) ((base_1pt)+(vaddr)/(PAGE_SIZE/4*PAGE_SIZE)*4)/* ���Ե�ַ��һ��ҳ���еĵ�ַ */
#define ADDR_IN_2PT(base_2pt,vaddr) ((base_2pt)+((vaddr)%(PAGE_SIZE/4*PAGE_SIZE))/PAGE_SIZE*4)/* ���Ե�ַ�ڶ���ҳ���еĵ�ַ */

/*
 * �������û������ַ�ռ䷶Χ(0GB - 3GB)���ں������ַ�ռ䷶Χ(3GB - 4GB)��
 * NOTE! �������������ڴ��������GDT,LDT�������ط�����һ�¡�
 */
#define V_USER_ZONE_START		(0x0)
#define V_USER_ZONE_END			(0xC0000000)
#define V_KERNEL_ZONE_START		(0xC0000000)
#define V_KERNEL_ZONE_END		(0x100000000)
#define PG_DIR_KERNEL_START		(768*4)	/* ��head.sͬ�� */

/* ҳ�����Ա�־����������ҳ��� */
#define P_UR	(4)	/* �û�̬��ֻ���������� */
#define P_UW	(6)	/* �û�̬����д�������� */
#define P_SR	(0)	/* �ں�̬��ֻ���������� */
#define P_SW	(2)	/* �ں�̬����д�������� */
#define P_URP	(5)	/* �û�̬��ֻ�������� */
#define P_UWP	(7)	/* �û�̬����д������ */
#define P_SRP	(1)	/* �ں�̬��ֻ�������� */
#define P_SWP	(3)	/* �ں�̬����д������ */

/* ʹTLB��Ч */
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
