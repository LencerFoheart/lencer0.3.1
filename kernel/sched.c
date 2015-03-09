/*
 * Small/kernel/sched.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * ���ļ��������̵��ȵ���س���
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

/* ����� */
long ticks = 0;
/* ���̲� */
struct proc_struct * proc[NR_PROC] = {0};
/* ��ǰ���̽ṹָ�� */
struct proc_struct * current = NULL;

/* UNION(��ʼ�����̽ṹ, �����ں�̬��ջ) */
union proc_union init_proc = {INIT_PROC, };

/*
 * �л����̡��˴����ж��Ƿ� current==proc[next]��
 * ��Ϊ�ڵ��ȳ������ѽ������жϡ�
 */
#define switch_to(next) \
do \
{ \
	__asm__ __volatile__( \
		"movl	%%ebx,current\n\t" \
		"pushl	%%ecx\n\t" /* �ߵ�ַ���TSS��ѡ��� */ \
		"pushl	$0\n\t" \
		"ljmp	*%%ss:(%%esp)\n\t" \
		"addl	$8,%%esp\n\t" \
		"clts\n\t" /*��CR0��TS��־��ÿ����������ת��ʱ��TS=1������ת����ϣ�TS=0��TS=1ʱ������Э������������*/ \
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
 * �ں˵��ȳ���NOTE! ����0�ĵ��Ȳ�������state�ֶκ�counter�ֶΡ�
 * ��������0û��ʱ��Ƭ����0�Ŀ�ִ�н��̣����������ó�����0�������
 * ���̵�ʱ��Ƭ�����ڿ��ܰ�������ִ�н��̣���counter����С��0(��Ϊ
 * �ں�Ĭ���ڷ���ʱ���ж�ʱ�������ں�̬�Ľ��̲����е���)����
 * counter = priority + counter��
 */
BOOL schedule(void)
{
	/* NOTE! ��Ҫ�Ķ�next, counter�ĳ�ʼֵ!!! */
	int next = -1;		/* ��һ����Ҫִ�еĽ����ڽ��̲��е����� */
	long counter = 0;	/* ����ʣ��ʱ��Ƭ�� */

	while(1) {
		/* ����Ѱ�����ȼ���ߵĿ�ִ�н��̡�NOTE! ���²��������0 */
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
		if(-1 == next)	/* ���˽���0��û��������ִ�н��� */
			break;
	}

	-1==next ? next=0 : 0;			/* ��ֻ�н���0����nextָ�����0 */
	if(current != proc[next]) {		/* ���ǵ�ǰ���̣����л� */
		switch_to(next);
		return TRUE;
	}

	return FALSE;
}

/* * ���̹���ϵͳ���á� */
void sys_pause(void)
{
	current->state = INTERRUPTIBLE;
	schedule();
}

/*
 * sleep_on ����: ���ж�˯�� �� �����ж�˯�ߡ�
 * ���ڿ��ж�˯�ߣ������յ��źŲ�תΪ��ִ��״̬�Ľ��̲�һ��
 * ��˯�߽��̶��е�ͷ�����̣����軽�ѿ��ж�˯�߽��̶��е�ͷ��
 * ���̣��Ա��������κ�һ���յ��źź󣬶��������еĽ��̾�תΪ
 * ��ִ��̬��
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
	if(0 == (cs & 0x3)) {	/* �����ں�̬�������� */
		++current->ktime;
		return;
	} else {
		++current->utime;
	}
	schedule();
}

void sched_init(void)
{
	/* ��ʼ�����̿��ƿ�ָ������ */
	/* nothing.��bss��,�ѱ����� */
	
	proc[0] = &init_proc.proc;
	current = proc[0];
	ticks = 0;

	/* ���ý���0��ldt��tss������ */
	set_ldt_desc(0, V_KERNEL_ZONE_START+(unsigned long)&(proc[0]->ldt));
	set_tss_desc(0, V_KERNEL_ZONE_START+(unsigned long)&(proc[0]->tss));
	/* װ�ؽ���0��ldt��tssѡ�������һ�������������� */
	lldt(_LDT(0));
	ltr(_TSS(0));
	/* ����ʱ���жϴ������ */
	set_idt(INT_R0, timer_int, NR_TIMER_INT);
	/* ����8253��ʱ��оƬ */
	out_b(0x43, 0x36);
	out_b(0x40, (LATCH & 0xff));
	out_b(0x40, ((LATCH>>8) & 0xff));
	/* ����ϵͳ���ô������ */
	set_idt(TRAP_R3, system_call, NR_SYS_CALL);
}