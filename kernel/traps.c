/*
 * Small/kernel/traps.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * NOTE! 由于我们在本文件中，又设置了8259A的所有中断的默认程序。
 * 在其他文件中会进一步设置，所以，trap_init()需先被调用。
 */

#include "sys_nr.h"
#include "sys_set.h"
#include "sched.h"
#include "kernel.h"
#include "string.h"

extern int page_fault_int(void);
extern int double_fault_int(void);
extern int error_tss_int(void);
extern int no_segment_int(void); 
extern int error_ss_int(void);
extern int general_protect_int(void);
extern int align_check_int(void);
extern int divide_int(void);
extern int debug_int(void);
extern int nmi_int(void);
extern int break_point_int(void);
extern int overflow_int(void);
extern int bound_int(void);
extern int ud_int(void);
extern int nm_int(void);
extern int mf_int(void);
extern int machine_check_int(void);
extern int xf_int(void);
extern int other_IRQ_int(void);
extern int IRQ7_int(void);

void print_trap_msg(unsigned long cs, unsigned long eip,
			unsigned long cr2, unsigned long error_code)
{
	_k_printf("[PROC-NR]%u [PID]%u [RING]%u [EIP]%x [CR2]%x [E-CODE]%x\n",
		get_cur_proc_nr(), current->pid, (cs & 0x3), eip, cr2, error_code);	
}

/*
 * 在本版内核中，为了调试的方便，对这些中断做简明处理。
 * 只显示一些信息，然后panic. 其中 do_no_page()函数在mm中。
 */

void do_double_fault(void)
{
	panic("do_double_fault: double_fault.");
}

void do_error_tss(void)
{
	panic("do_error_tss: error_tss.");
}

void do_no_segment(void)
{
	panic("do_no_segment: no_segment.");
} 

void do_error_ss(void)
{
	panic("do_error_ss: error_ss.");
}

void do_general_protect(void)
{
	panic("do_general_protect: general_protect.");
}

void do_align_check(void)
{
	panic("do_align_check: align_check.");
}

void do_divide(void)
{
	panic("do_divide: divide 0.");
}

void do_debug(void)
{
	panic("do_debug: debug.");
}

void do_nmi(void)
{
	panic("do_nmi: nmi.");
}

void do_break_point(void)
{
	panic("do_break_point: break_point.");
}

void do_overflow(void)
{
	panic("do_overflow: overflow.");
}

void do_bound(void)
{
	panic("do_bound: bound.");
}

void do_ud(void)
{
	panic("do_ud: ud.");
}

void do_nm(void)
{
	panic("do_nm: nm.");
}

void do_mf(void)
{
	panic("do_mf: mf.");
}

void do_machine_check(void)
{
	panic("do_machine_check: machine_check.");
}

void do_xf(void)
{
	panic("do_xf: xf.");
}

void do_other_IRQ(void)
{
	k_printf("do_others: other-IRQ-interrupt.");
}

void trap_init(void)
{
	/* TRAP, TRAP, not INTR */
	set_idt(TRAP_R0, page_fault_int, NR_PF_INT);
	set_idt(TRAP_R0, double_fault_int, NR_DF_INT);
	set_idt(TRAP_R0, error_tss_int, NR_TS_INT);
	set_idt(TRAP_R0, no_segment_int, NR_NP_INT);
	set_idt(TRAP_R0, error_ss_int, NR_SS_INT);
	set_idt(TRAP_R0, general_protect_int, NR_GP_INT);
	set_idt(TRAP_R0, align_check_int, NR_AC_INT);
	set_idt(TRAP_R0, divide_int, NR_DIV_INT);
	set_idt(TRAP_R0, debug_int, NR_DEBUG_INT);
	set_idt(TRAP_R0, nmi_int, NR_NMI_INT);
	set_idt(TRAP_R0, break_point_int, NR_BP_INT);
	set_idt(TRAP_R0, overflow_int, NR_OF_INT);
	set_idt(TRAP_R0, bound_int, NR_BR_INT);
	set_idt(TRAP_R0, ud_int, NR_UD_INT);
	set_idt(TRAP_R0, nm_int, NR_NM_INT);
	set_idt(TRAP_R0, mf_int, NR_MF_INT);
	set_idt(TRAP_R0, machine_check_int, NR_MC_INT);
	set_idt(TRAP_R0, xf_int, NR_XF_INT);

	/* INTR */
	for(int i=0x20; i<0x20+16; i++)
		set_idt(INT_R0, other_IRQ_int, i);
	/*
	 * Well, I'm very surprising! 在我初始化一段比较大的内存为0时，
	 * 竟引发了IRQ7中断。不知道原因，于是百度之，话说是有这样一种
	 * 可能性：“当中断请求持续时间太短，当CPU反过头来让8259A反馈时，
	 * 没有了中断请求，于是8259A就会把IRQ7填入data bus”。
	 *
	 * 所以，我们对IRQ7采取特殊处理，遇到它，就iret之。
	 *
	 * [??] 以上只是推测，实际原因是啥，我尚未知否 :-((((
	 * 参考网址：http://blog.csdn.net/lightseed/article/details/6074014
	 */
	set_idt(INT_R0, IRQ7_int, 0x27);
}
