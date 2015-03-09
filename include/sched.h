/*
 * Small/include/sched.h
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

#ifndef _SCHED_H_
#define _SCHED_H_

#include "sys_set.h"
#include "memory.h"
#include "types.h"
#include "file_system.h"

/********************************************************************/

/* �����������ں�Ŀǰ֧�ֵ��������� */
#define NR_PROC		(64)

#define LATCH		(11930)		/* ��ʱ����ʼ����ֵ����10���� */

/* ����ldt������ */
#ifndef NR_PROC_LDT
#define NR_PROC_LDT		3
#endif

/* ����״̬��NOTE! ����0����Ҫ�ݴ˽��е��ȡ� */
#define RUNNING			(0)		/* ����̬ */
#define INTERRUPTIBLE	(1)		/* ���ж�˯��̬ */
#define UNINTERRUPTIBLE	(2)		/* �����ж�˯��̬ */
#define PAUSED			(3)		/* ��̬ͣ */
#define ZOMBIE			(-1)	/* ����̬����״̬��ʾ������ֹͣ���У�
								�ȴ�father���̻�ȡ��ֹͣ���е������Ϣ�� */

/* tss�ṹ */
struct tss_struct {
	long	back_link;		/* 16 high bits zero */
	long	esp0;
	long	ss0;			/* 16 high bits zero */
	long	esp1;
	long	ss1;			/* 16 high bits zero */
	long	esp2;
	long	ss2;			/* 16 high bits zero */
	long	cr3;
	long	eip;
	long	eflags;
	long	eax,ecx,edx,ebx;
	long	esp;
	long	ebp;
	long	esi;
	long	edi;
	long	es;				/* 16 high bits zero */
	long	cs;				/* 16 high bits zero */
	long	ss;				/* 16 high bits zero */
	long	ds;				/* 16 high bits zero */
	long	fs;				/* 16 high bits zero */
	long	gs;				/* 16 high bits zero */
	long	ldt;			/* 16 high bits zero */
	long	trace_bitmap;	/* bits: trace 0, bitmap 16-31 */
};

/* ���̽ṹ���޸Ĵ˽ṹʱ�����޸������ INIT_PROC !!! */
struct proc_struct {
	/* ���ȿ�����Ϣ */
	long state;						/* ��������״̬������0�ĸ��ֶβ��� */
	long counter;					/* ��������ʱ��Ƭ���ݼ���0��˵��ʱ��Ƭ���ꡣNOTE! counter����Ϊ�� */
	long priority;					/* �����������������տ�ʼ��counter==priority��Խ������Խ�� */
//	long signal;					/* ������ź�λͼ,ÿ������λ����һ���ź�,�ź�ֵ=ƫ��+1,����0�������ź� */
//	struct sigaction sigaction[32];	/* �ź�ִ�����Խṹ����Ӧ�źŽ�Ҫִ�еĲ����ͱ�־��Ϣ */
//	long blocked;					/* �����ź������루��Ӧ�ź�λͼ�� */
	/* ����������Ϣ */
	long code_start;				/* �������ʼ���Ե�ַ */
	long code_end;					/* ����ν������Ե�ַ */
	long data_start;				/* ���ݶκͶ�ջ����ʼ���Ե�ַ */
	long data_end;					/* ���ݶκͶ�ջ�ν������Ե�ַ */
	int exit_code;					/* �����˳��룬���������ʱ�丸���̻��ȡ */
	long pid;						/* ���̺� */
	long fpid;						/* �����̺� */
	long utime;						/* �û�̬ʱ�䣬�δ��� */
	long ktime;						/* �ں�̬ʱ�䣬�δ��� */
	long cutime;					/* �ӽ����û�̬ʱ�䣬�δ��� */
	long cktime;					/* �ӽ����ں�̬ʱ�䣬�δ��� */
	/* �����ļ���Ϣ */
	struct m_inode * root_dir;			/* ���̸�Ŀ¼�����ڵ� */
	struct m_inode * current_dir;		/* ���̵�ǰ����Ŀ¼�����ڵ� */
	struct file_table_struct * file[PROC_MAX_FILES];	/* ���̵��ļ��������� */
	struct exec_info * exec;			/* ��ִ���ļ���Ϣ�ṹָ�� */
	/* ����ldt�� */
	struct desc_struct ldt[NR_PROC_LDT];/* �������ldt��NR_PROC_LDT=3��0-�գ�1-����Σ�2-���ݺͶ�ջ�� */
	/* ����tss�� */
	struct tss_struct tss;			/* �������tss�� */
};

/* UNION(���̽ṹ, �����ں�̬��ջ)  */
union proc_union {
	struct proc_struct proc;
	char stack[PAGE_SIZE];
};

/*
 * ��ԭʼ���̵� proc_struct �ṹ���ں����ݴ˽��̽ṹ����������̣�
 * �˽�����Ϊ0���̣����඼�����ӽ��̡�
 *
 * ����0����ַ0���޳�3GB����Ȩ��3��NOTE! ���̵������ַ�ռ�Ϊ3GB-4GB��
 * child������������������LDT��������ֻ���father���̼̳��������ɡ�
 *
 * ע�⣺����0�Ĵ���κ����ݶ���ֹ��ַ�����ڴ��������г�ʼ����
 */
#define INIT_PROC { \
/* state,counter,priority	*/		0, 15, 15, \
/* code_start,end,data... */		0, 0, 0, 0, \
/* exit_code,pid,fpid		*/		0, 0, 0, \
/* utime,kt..,cut..,ckt..	*/		0, 0, 0, 0, \
/* file... */						NULL, NULL, {NULL,}, NULL, \
/* ldt */	{ \
				{0, 0}, \
				{0xffff, 0xcbfa00}, \
				{0xffff, 0xcbf200}, \
			}, \
/* tss */	{ \
				NULL, PAGE_SIZE+(unsigned long)&init_proc, 0x10, 0,0,0,0, (unsigned long)&pg_dir, \
/* 10 */		0,0,0,0,0,0,0,0,0,0, \
				0x17, 0x0f, 0x17, 0x17, 0x17, 0x17, \
				_LDT(0),0x80000000, \
			}, \
}

/********************************************************************/
extern struct proc_struct * proc[NR_PROC];
extern struct proc_struct * current;

extern int get_cur_proc_nr(void);
extern void sys_pause(void);
extern void sleep_on(struct proc_struct ** p, int sleep_state);
extern void wake_up(struct proc_struct ** p);
extern void sched_init(void);
extern BOOL schedule(void);
extern void do_timer(long cs);

/********************************************************************/

#endif /* _SCHED_H_ */