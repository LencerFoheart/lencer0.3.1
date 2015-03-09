/*
 * Small/include/sys_set.h
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */
/*
 * ��ͷ�ļ�����ϵͳ���õĺ궨��ȡ���Ҫ���������������˿ڲ�����
 * �ж����Ρ�move_to_user_mode�ȵ���ز����ĺ궨�塣
 */

#ifndef _SYS_SET_H_
#define _SYS_SET_H_

/*
#include "sched.h"
*/
/*
 * ����ldt��������NOTE! �˺�����"sched.h"�ж��壬Ϊ�˱���ͷ�ļ�
 * �໥���������ô˷��������Ϊ�һ�û���뵽���õĽ��ͷ�ļ��໥
 * ��������İ취 :-)
 */
#ifndef NR_PROC_LDT
#define NR_PROC_LDT		(3)
#endif

/********************************************************************/

#define move_to_user_mode() \
do { \
	__asm__ __volatile__( \
		"pushfl\n\t" \
		"andl	$0xffffbfff,(%esp)\n\t" /* ��λ���¼Ĵ�����Ƕ�������־λNT */ \
		"popfl\n\t" \
		"movl	$0x17,%eax\n\t" /* 0x17���ֲ����ݶ�ѡ��� */ \
		"mov	%ax,%ds\n\t" \
		"mov	%ax,%es\n\t" \
		"mov	%ax,%fs\n\t" \
		"mov	%ax,%gs\n\t" \
		"mov	%esp,%eax\n\t" \
		"pushl	$0x17\n\t" \
		"pushl	%eax\n\t" \
		"pushfl\n\t" \
		"pushl	$0x0f\n\t" /* 0x0f���ֲ������ѡ��� */ \
		"pushl	$1f\n\t" \
		"iretl\n\t" \
		"1:\n\t" \
	); \
} while(0)

#define cli() \
do { \
	__asm__("cli"); \
} while(0)

#define sti() \
do { \
	__asm__("sti"); \
} while(0)

#define load_cr3(a) ({ \
	__asm__ __volatile__( \
		"movl %%eax,%%cr3\n\t" \
		::"a"((a)&0xfffff000) : \
	); \
})

/* ���ú�ȡ��cr0�е�д����λ */
#define set_wp_bit() ({ \
	__asm__ __volatile__( \
		"movl %cr0,%eax\n\t" \
		"orl $0x00010000,%eax\n\t" \
		"movl %eax,%cr0\n\t" \
	); \
})
#define unset_wp_bit() ({ \
	__asm__ __volatile__( \
		"movl %cr0,%eax\n\t" \
		"andl $0xfffeffff,%eax\n\t" \
		"movl %eax,%cr0\n\t" \
	); \
})

#define get_cs() ({ \
	unsigned short ret; \
	__asm__( \
		"movw %%cs,%%ax\n\t" \
		:"=a"(ret) :: \
	); \
	ret; \
})
#define get_fs() ({ \
	unsigned short ret; \
	__asm__( \
		"movw %%fs,%%ax\n\t" \
		:"=a"(ret) :: \
	); \
	ret; \
})
#define set_fs(a) ({ \
	unsigned short var; \
	__asm__( \
		"movw %%fs,%%bx\n\t" \
		"movw %%ax,%%fs\n\t" \
		:"=b"(var) :"a"(a) : \
	); \
	var&0xffff; \
})
/* ����fsָ���ں˵�ַ�ռ� */
#define set_fs_kernel() ({ \
	unsigned short var; \
	__asm__( \
		"movw %%fs,%%bx\n\t" \
		"movw %%ds,%%ax\n\t" \
		"movw %%ax,%%fs\n\t" \
		:"=b"(var) :: \
	); \
	var&0xffff; \
})
/* ����fsָ���û���ַ�ռ� */
#define set_fs_user() ({ \
	unsigned short var; \
	__asm__( \
		"movw %%fs,%%bx\n\t" \
		"movw $0x17,%%ax\n\t" \
		"movw %%ax,%%fs\n\t" \
		:"=b"(var) :: \
	); \
	var&0xffff; \
})
#define set_gs(a) ({ \
	unsigned short var; \
	__asm__( \
		"movw %%gs,%%bx\n\t" \
		"movw %%ax,%%gs\n\t" \
		:"=b"(var) :"a"(a) : \
	); \
	var&0xffff; \
})

/* ��ָ���˿ڶ������� */
#define in_b(src) ({ \
	unsigned char var; \
	__asm__( \
		"inb	%%dx,%%al\n\t" \
		"jmp	1f\n\t" \
		"1:		\n\t" \
		"jmp	1f\n\t" \
		"1:		\n\t" \
		: "=a"(var) : "d"(src) :/*����� dx ��*/ \
	); \
	var; \
})

/* ��ָ���˿�д���� */
#define out_b(des, data) \
do { \
	__asm__( \
		"outb	%%al,%%dx\n\t" \
		"jmp	1f\n\t" \
		"1:		\n\t" \
		"jmp	1f\n\t" \
		"1:		\n\t" \
		: : "a"(data),"d"(des) :/*����� dx ��*/ \
	); \
} while(0)

/********************************************************************/
/* �������������������� */

/* �������ṹ�� */
typedef struct desc_struct {
	unsigned long a,b;
}desc_table[256];

extern desc_table idt,gdt;

/*
 * NOTE! �ж��ź������ŵ�DPL���ڷ��ʱ����������DPL֮�󣬾��з���Ȩ�ޣ�
 * ��CPU�Զ�ȡ���������еĶ�ѡ�������CS��
 *
 * ��ˣ�����ϵͳ���ã��ں˽�����Ȩ������ΪRING3��ʹ�û�������Է��ʡ�
 * ������ϵͳ����֮��CS����Ȩ��ΪRING0����Ϊ���������еĶ�ѡ���Ϊ0x8��
 */
#define INT_R0	(0x8e00)		/* �ж������ͣ���Ȩ��0 */
#define INT_R3	(0xee00)		/* �ж������ͣ���Ȩ��3 */
#define TRAP_R0	(0x8f00)		/* ���������ͣ���Ȩ��0 */
#define TRAP_R3	(0xef00)		/* ���������ͣ���Ȩ��3 */

#define lldt(a) \
do { \
	__asm__("lldt %%ax" ::"a"(a) :); \
} while(0)

#define ltr(a) \
do { \
	__asm__("ltr %%ax" ::"a"(a) :); \
} while(0)

/* �����ж��������� */
#define set_idt(FLAG,FUNC_ADDR,NR_INT) \
do { \
	__asm__( \
		"movl	$0x00080000,%%eax\n\t"/* ע�⣺��ѡ���Ϊ 0x00080000 */ \
		"movw	%%dx,%%ax\n\t" \
		"movw	%%cx,%%dx\n\t" \
		"movl	%%eax,(%%edi)\n\t" \
		"movl	%%edx,4(%%edi)\n\t" \
		: : "c"(FLAG),"d"(FUNC_ADDR),"D"(&idt[NR_INT]) : /* idt�ǽṹ�����飬�ʲ����ٳ���8������ע����ȡ��ַ */ \
	); \
} while(0)


/* ldt��tss��ѡ�������Ȩ��0 */
#define FIRST_LDT_ENTRY	(4)
#define FIRST_TSS_ENTRY	((FIRST_LDT_ENTRY)+1)
#define _LDT(nr) ((unsigned long)((FIRST_LDT_ENTRY)<<3) + ((nr)<<4))
#define _TSS(nr) ((unsigned long)((FIRST_TSS_ENTRY)<<3) + ((nr)<<4))

/*
 * ����GDT�ϵ�ldt��tss��������
 *
 * NOTE! ldt��tss�ο�����Ϊ1byte��
 * ldt����������Ϊ��0x82 (P - 1, DPL - 00, 0, TYPE - 0010)
 * tss����������Ϊ��0x89 (P - 1, DPL - 00, 0, TYPE - 1001)
 * ldt���޳���NR_PROC_LDT * 8 - 1
 * tss���޳���sizeof(struct tss_struct) - 1
 *
 * NOTE! GCC����ѡ� -fomit-frame-pointer --- ����ʱ��ERROR: 
 * can't find a register in class 'GENERAL_REGS' while reloading 'asm'.
 */
#define set_ldt_desc(nr,ldt_addr) \
do { \
	unsigned long desc_addr = (unsigned long)&gdt + ((FIRST_LDT_ENTRY+((nr)<<1)) << 3); /* gdt��ȡ��ַ */ \
	__asm__( \
		"movw	%%ax,%2\n\t" \
		"movl	%%eax,%4\n\t" \
		"movl	%%ebx,%3\n\t" \
		"movb	%5,%%al\n\t" \
		"movb	%%al,%6\n\t" \
		"movb	$0x82,%5\n\t" \
		::"a"(NR_PROC_LDT * 8 - 1), "b"(ldt_addr), "m"(*(char*)(desc_addr+0)), "m"(*(char*)(desc_addr+2)), \
		"m"(*(char*)(desc_addr+4)), "m"(*(char*)(desc_addr+5)), "m"(*(char*)(desc_addr+7)) : \
	); \
} while(0)

#define set_tss_desc(nr,tss_addr) \
do { \
	unsigned long desc_addr = (unsigned long)&gdt + ((FIRST_TSS_ENTRY+((nr)<<1)) << 3); /* gdt��ȡ��ַ */ \
	__asm__( \
		"movw	%%ax,%2\n\t" \
		"movl	%%eax,%4\n\t" \
		"movl	%%ebx,%3\n\t" \
		"movb	%5,%%al\n\t" \
		"movb	%%al,%6\n\t" \
		"movb	$0x89,%5\n\t" \
		::"a"(sizeof(struct tss_struct) - 1), "b"(tss_addr), "m"(*(char*)(desc_addr+0)), "m"(*(char*)(desc_addr+2)), \
		"m"(*(char*)(desc_addr+4)), "m"(*(char*)(desc_addr+5)), "m"(*(char*)(desc_addr+7)) : \
	); \
} while(0)

/********************************************************************/
#endif /* _SYS_SET_H_ */
