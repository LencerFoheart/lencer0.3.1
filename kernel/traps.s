#
# Small/kernel/traps.s
#
# (C) 2012-2013 Yafei Zheng <e9999e@163.com>
#


.globl page_fault_int, double_fault_int, error_tss_int, no_segment_int, 
.globl error_ss_int, general_protect_int, align_check_int
.globl divide_int, debug_int, nmi_int, break_point_int, overflow_int
.globl bound_int, ud_int, nm_int, mf_int, machine_check_int, xf_int
.globl other_IRQ_int, IRQ7_int

#
# NOTE!!! have_error_code 和 no_error_code 若修改，请特别留意。
# 你同时还需要修改 print_trap_msg() 参数顺序以及其函数声明等等。
#

#
# OK. 这是有出错号的中断。NOTE! 在iret之前，我们需要将出错码
# 从堆栈弹出，以便正确iret。
#
.align 4
page_fault_int:									# 中断号14
	call	interrupt_entry
	leal	do_page_fault,%eax
have_error_code:
	xchgl	%eax,%ss:44(%esp)	# 保存函数地址，并换出出错码
	pushl	%eax				# 出错码入栈
	movl	%cr2,%eax
	pushl	%eax				# cr2入栈
	movl	%ss:56(%esp),%eax
	pushl	%eax				# eip入栈
	movl	%ss:64(%esp),%eax
	pushl	%eax				# cs入栈
	call	print_trap_msg
	addl	$8,%esp
	call	*%ss:52(%esp)
	addl	$8,%esp
	call	ret_from_interrupt
	addl	$4,%esp		# 跳过保存在堆栈中的函数地址，原出错码位置
	iret

.align 4
double_fault_int:								# 中断号8
	call	interrupt_entry
	leal	do_double_fault,%eax
	jmp		have_error_code

.align 4
error_tss_int:									# 中断号10
	call	interrupt_entry
	leal	do_error_tss,%eax
	jmp		have_error_code

.align 4
no_segment_int:									# 中断号11
	call	interrupt_entry
	leal	do_no_segment,%eax
	jmp		have_error_code

.align 4
error_ss_int:									# 中断号12
	call	interrupt_entry
	leal	do_error_ss,%eax
	jmp		have_error_code

.align 4
general_protect_int:							# 中断号13
	call	interrupt_entry
	leal	do_general_protect,%eax
	jmp		have_error_code

.align 4
align_check_int:								# 中断号17
	call	interrupt_entry
	leal	do_align_check,%eax
	jmp		have_error_code



#
# OK. 这是没有出错号的中断。但我们将一个假想的出错号(0)入栈 ...
#
.align 4
divide_int:										# 中断号0
	call	interrupt_entry
	leal	do_divide,%eax
no_error_code:
	pushl	%eax				# 保存函数地址
	pushl	$0					# 将0入栈，作为假想的出错码
	movl	%cr2,%eax
	pushl	%eax				# cr2入栈
	movl	%ss:56(%esp),%eax
	pushl	%eax				# eip入栈
	movl	%ss:64(%esp),%eax
	pushl	%eax				# cs入栈
	call	print_trap_msg
	addl	$16,%esp
	call	*%ss:(%esp)
	addl	$4,%esp				# 跳过保存在堆栈中的函数地址
	call	ret_from_interrupt
	iret

.align 4
debug_int:										# 中断号1
	call	interrupt_entry
	leal	do_debug,%eax
	jmp		no_error_code

.align 4
nmi_int:										# 中断号2
	call	interrupt_entry
	leal	do_nmi,%eax
	jmp		no_error_code

.align 4
break_point_int:								# 中断号3
	call	interrupt_entry
	leal	do_break_point,%eax
	jmp		no_error_code

.align 4
overflow_int:									# 中断号4
	call	interrupt_entry
	leal	do_overflow,%eax
	jmp		no_error_code

.align 4
bound_int:										# 中断号5
	call	interrupt_entry
	leal	do_bound,%eax
	jmp		no_error_code

.align 4
ud_int:											# 中断号6
	call	interrupt_entry
	leal	do_ud,%eax
	jmp		no_error_code

.align 4
nm_int:											# 中断号7
	call	interrupt_entry
	leal	do_nm,%eax
	jmp		no_error_code

.align 4
mf_int:											# 中断号16
	call	interrupt_entry
	leal	do_mf,%eax
	jmp		no_error_code

.align 4
machine_check_int:								# 中断号18
	call	interrupt_entry
	leal	do_machine_check,%eax
	jmp		no_error_code

.align 4
xf_int:											# 中断号19
	call	interrupt_entry
	leal	do_xf,%eax
	jmp		no_error_code

#
# NOTE! this is the default 8259A interrupt handle, so wo don't EOI.
#
.align 4
other_IRQ_int:
	call	interrupt_entry
	leal	do_other_IRQ,%eax
	jmp		no_error_code

.align 4
IRQ7_int:
	pushl	%eax
	movb	$0x20,%al
	outb	%al,$0x20
	popl	%eax
	iret
