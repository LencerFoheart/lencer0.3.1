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

/* 最多进程数，内核目前支持的最大进程数 */
#define NR_PROC		(64)

#define LATCH		(11930)		/* 定时器初始计数值，即10毫秒 */

/* 进程ldt表项数 */
#ifndef NR_PROC_LDT
#define NR_PROC_LDT		3
#endif

/* 进程状态。NOTE! 进程0不需要据此进行调度。 */
#define RUNNING			(0)		/* 运行态 */
#define INTERRUPTIBLE	(1)		/* 可中断睡眠态 */
#define UNINTERRUPTIBLE	(2)		/* 不可中断睡眠态 */
#define PAUSED			(3)		/* 暂停态 */
#define ZOMBIE			(-1)	/* 僵死态。该状态表示进程已停止运行，
								等待father进程获取其停止运行的相关信息。 */

/* tss结构 */
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

/* 进程结构。修改此结构时，需修改下面的 INIT_PROC !!! */
struct proc_struct {
	/* 调度控制信息 */
	long state;						/* 进程运行状态，任务0的该字段不用 */
	long counter;					/* 任务运行时间片，递减到0是说明时间片用完。NOTE! counter可以为负 */
	long priority;					/* 任务运行优先数，刚开始是counter==priority，越大运行越长 */
//	long signal;					/* 任务的信号位图,每个比特位代表一种信号,信号值=偏移+1,任务0不处理信号 */
//	struct sigaction sigaction[32];	/* 信号执行属性结构，对应信号将要执行的操作和标志信息 */
//	long blocked;					/* 进程信号屏蔽码（对应信号位图） */
	/* 进程属性信息 */
	long code_start;				/* 代码段起始线性地址 */
	long code_end;					/* 代码段结束线性地址 */
	long data_start;				/* 数据段和堆栈段起始线性地址 */
	long data_end;					/* 数据段和堆栈段结束线性地址 */
	int exit_code;					/* 任务退出码，当任务结束时其父进程会读取 */
	long pid;						/* 进程号 */
	long fpid;						/* 父进程号 */
	long utime;						/* 用户态时间，滴答数 */
	long ktime;						/* 内核态时间，滴答数 */
	long cutime;					/* 子进程用户态时间，滴答数 */
	long cktime;					/* 子进程内核态时间，滴答数 */
	/* 进程文件信息 */
	struct m_inode * root_dir;			/* 进程根目录索引节点 */
	struct m_inode * current_dir;		/* 进程当前工作目录索引节点 */
	struct file_table_struct * file[PROC_MAX_FILES];	/* 进程的文件描述符表 */
	struct exec_info * exec;			/* 可执行文件信息结构指针 */
	/* 进程ldt表 */
	struct desc_struct ldt[NR_PROC_LDT];/* 本任务的ldt表，NR_PROC_LDT=3，0-空，1-代码段，2-数据和堆栈段 */
	/* 进程tss段 */
	struct tss_struct tss;			/* 本任务的tss段 */
};

/* UNION(进程结构, 进程内核态堆栈)  */
union proc_union {
	struct proc_struct proc;
	char stack[PAGE_SIZE];
};

/*
 * 最原始进程的 proc_struct 结构。内核依据此进程结构创建其余进程，
 * 此进程作为0进程，其余都是其子进程。
 *
 * 进程0：基址0，限长3GB，特权级3。NOTE! 进程的虚拟地址空间为3GB-4GB，
 * child进程无需再重新设置LDT描述符，只需从father进程继承下来即可。
 *
 * 注意：进程0的代码段和数据段起止地址，在内存管理里进行初始化。
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