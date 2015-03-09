/*
 * Small/kernel/sched.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * 此文件包含进程调度的相关程序。
 */


#ifndef __DEBUG__
#define __DEBUG__
#endif


#include "sys_set.h"
#include "sys_nr.h"
#include "kernel.h"
#include "types.h"
#include "sched.h"

extern int timer_int(void);
extern int system_call(void);

/* 嘀嗒数 */
long ticks = 0;
/* 进程槽 */
struct proc_struct * proc[NR_PROC] = {0};
/* 当前进程结构指针 */
struct proc_struct * current = NULL;

/* UNION(初始化进程结构, 进程内核态堆栈) */
union proc_union init_proc = {INIT_PROC, };

/*
 * 切换进程。此处不判断是否 current==proc[next]，
 * 因为在调度程序里已进行了判断。
 */
#define switch_to(next) \
do \
{ \
	__asm__ __volatile__( \
		"movl	%%ebx,current\n\t" \
		"pushl	%%ecx\n\t" /* 高地址存放TSS段选择符 */ \
		"pushl	$0\n\t" \
		"ljmp	*%%ss:(%%esp)\n\t" \
		"addl	$8,%%esp\n\t" \
		"clts\n\t" /*清CR0中TS标志。每当进行任务转换时，TS=1，任务转换完毕，TS=0。TS=1时不允许协处理器工作。*/ \
		::"b"(proc[next]), "c"(_TSS(next)) : \
	); \
}while(0)

int get_cur_proc_nr(void)
{
	for(int i=0; i<NR_PROC; i++)
		if(current == proc[i])
			return i;
	panic("get_cur_proc_nr: unknown process.");
	return -1;
}

/*
 * 内核调度程序。NOTE! 进程0的调度不依赖于state字段和counter字段。
 * 若除进程0没有时间片大于0的可执行进程，则重新设置除进程0外的所有
 * 进程的时间片，由于可能包含不可执行进程，且counter可能小于0(因为
 * 内核默认在发生时钟中断时，处于内核态的进程不进行调度)，故
 * counter = priority + counter。
 */
BOOL schedule(void)
{
	/* NOTE! 不要改动next, counter的初始值!!! */
	int next = -1;		/* 下一个将要执行的进程在进程槽中的索引 */
	long counter = 0;	/* 进程剩余时间片数 */

	while(1) {
		/* 倒序寻找优先级最高的可执行进程。NOTE! 以下不处理进程0 */
		for(int i=NR_PROC-1; i; i--) {
			if((NULL != proc[i]) && (RUNNING == proc[i]->state)
			&& (counter < proc[i]->counter)) {
				next = i;
				counter = proc[i]->counter;
			}
		}
		if(counter > 0)
			break;
		for(int i=NR_PROC-1; i; i--) {
			if((NULL != proc[i])) {
				(RUNNING == proc[i]->state) ? next = i : 0;
				proc[i]->counter = proc[i]->priority + proc[i]->counter;
			}
		}
		if(-1 == next)	/* 除了进程0，没有其他可执行进程 */
			break;
	}

	-1==next ? next=0 : 0;			/* 若只有进程0，则next指向进程0 */
	if(current != proc[next]) {		/* 若是当前进程，则不切换 */
		switch_to(next);
		return TRUE;
	}

	return FALSE;
}

/* * 进程挂起系统调用。 */
void sys_pause(void)
{
	current->state = INTERRUPTIBLE;
	schedule();
}

/*
 * sleep_on 包含: 可中断睡眠 和 不可中断睡眠。
 * 对于可中断睡眠，由于收到信号并转为可执行状态的进程不一定
 * 是睡眠进程队列的头部进程，故需唤醒可中断睡眠进程队列的头部
 * 进程，以便在其中任何一个收到信号后，队列中所有的进程均转为
 * 可执行态。
 */
void sleep_on(struct proc_struct ** p, int sleep_state)
{
	if(NULL == p)
		return;
	if(sleep_state!=UNINTERRUPTIBLE && sleep_state!=INTERRUPTIBLE)
		panic("sleep_on: bad sleep-state.");

	struct proc_struct *tmp = *p;
	*p = current;
	current->state = sleep_state;
	schedule();

	if((INTERRUPTIBLE == sleep_state) && (NULL != *p)) {
		if(*p != current) {
			(*p)->state = RUNNING;
			*p = NULL;
			schedule();
		} else {
			*p = NULL;
		}
	}

	if(tmp) {
		tmp->state = RUNNING;
		schedule();
	}
}

void wake_up(struct proc_struct ** p)
{
	if(p!=NULL && *p!=NULL) {
		(*p)->state = RUNNING;
		*p = NULL;
	}
}

void do_timer(long cs)
{
	++ticks;

	if(--current->counter > 0)
		return;
	if(0 == (cs & 0x3)) {	/* 处于内核态，不调度 */
		++current->ktime;
		return;
	} else {
		++current->utime;
	}
	schedule();
}

void sched_init(void)
{
	/* 初始化进程控制块指针数组 */
	/* nothing.在bss段,已被清零 */
	
	proc[0] = &init_proc.proc;
	current = proc[0];
	ticks = 0;

	/* 设置进程0的ldt和tss描述符 */
	set_ldt_desc(0, V_KERNEL_ZONE_START+(unsigned long)&(proc[0]->ldt));
	set_tss_desc(0, V_KERNEL_ZONE_START+(unsigned long)&(proc[0]->tss));
	/* 装载进程0的ldt和tss选择符，第一次需我们来加载 */
	lldt(_LDT(0));
	ltr(_TSS(0));
	/* 挂载时钟中断处理程序 */
	set_idt(INT_R0, timer_int, NR_TIMER_INT);
	/* 设置8253定时器芯片 */
	out_b(0x43, 0x36);
	out_b(0x40, (LATCH & 0xff));
	out_b(0x40, ((LATCH>>8) & 0xff));
	/* 挂载系统调用处理程序 */
	set_idt(TRAP_R3, system_call, NR_SYS_CALL);
}