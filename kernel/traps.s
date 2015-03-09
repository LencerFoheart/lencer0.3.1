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
# NOTE!!! have_error_code �� no_error_code ���޸ģ����ر����⡣
# ��ͬʱ����Ҫ�޸� print_trap_msg() ����˳���Լ��亯�������ȵȡ�
#

#
# OK. �����г���ŵ��жϡ�NOTE! ��iret֮ǰ��������Ҫ��������
# �Ӷ�ջ�������Ա���ȷiret��
#
.align 4
page_fault_int:									# �жϺ�14
	call	interrupt_entry
	leal	do_page_fault,%eax
have_error_code:
	xchgl	%eax,%ss:44(%esp)	# ���溯����ַ��������������
	pushl	%eax				# ��������ջ
	movl	%cr2,%eax
	pushl	%eax				# cr2��ջ
	movl	%ss:56(%esp),%eax
	pushl	%eax				# eip��ջ
	movl	%ss:64(%esp),%eax
	pushl	%eax				# cs��ջ
	call	print_trap_msg
	addl	$8,%esp
	call	*%ss:52(%esp)
	addl	$8,%esp
	call	ret_from_interrupt
	addl	$4,%esp		# ���������ڶ�ջ�еĺ�����ַ��ԭ������λ��
	iret

.align 4
double_fault_int:								# �жϺ�8
	call	interrupt_entry
	leal	do_double_fault,%eax
	jmp		have_error_code

.align 4
error_tss_int:									# �жϺ�10
	call	interrupt_entry
	leal	do_error_tss,%eax
	jmp		have_error_code

.align 4
no_segment_int:									# �жϺ�11
	call	interrupt_entry
	leal	do_no_segment,%eax
	jmp		have_error_code

.align 4
error_ss_int:									# �жϺ�12
	call	interrupt_entry
	leal	do_error_ss,%eax
	jmp		have_error_code

.align 4
general_protect_int:							# �жϺ�13
	call	interrupt_entry
	leal	do_general_protect,%eax
	jmp		have_error_code

.align 4
align_check_int:								# �жϺ�17
	call	interrupt_entry
	leal	do_align_check,%eax
	jmp		have_error_code



#
# OK. ����û�г���ŵ��жϡ������ǽ�һ������ĳ����(0)��ջ ...
#
.align 4
divide_int:										# �жϺ�0
	call	interrupt_entry
	leal	do_divide,%eax
no_error_code:
	pushl	%eax				# ���溯����ַ
	pushl	$0					# ��0��ջ����Ϊ����ĳ�����
	movl	%cr2,%eax
	pushl	%eax				# cr2��ջ
	movl	%ss:56(%esp),%eax
	pushl	%eax				# eip��ջ
	movl	%ss:64(%esp),%eax
	pushl	%eax				# cs��ջ
	call	print_trap_msg
	addl	$16,%esp
	call	*%ss:(%esp)
	addl	$4,%esp				# ���������ڶ�ջ�еĺ�����ַ
	call	ret_from_interrupt
	iret

.align 4
debug_int:										# �жϺ�1
	call	interrupt_entry
	leal	do_debug,%eax
	jmp		no_error_code

.align 4
nmi_int:										# �жϺ�2
	call	interrupt_entry
	leal	do_nmi,%eax
	jmp		no_error_code

.align 4
break_point_int:								# �жϺ�3
	call	interrupt_entry
	leal	do_break_point,%eax
	jmp		no_error_code

.align 4
overflow_int:									# �жϺ�4
	call	interrupt_entry
	leal	do_overflow,%eax
	jmp		no_error_code

.align 4
bound_int:										# �жϺ�5
	call	interrupt_entry
	leal	do_bound,%eax
	jmp		no_error_code

.align 4
ud_int:											# �жϺ�6
	call	interrupt_entry
	leal	do_ud,%eax
	jmp		no_error_code

.align 4
nm_int:											# �жϺ�7
	call	interrupt_entry
	leal	do_nm,%eax
	jmp		no_error_code

.align 4
mf_int:											# �жϺ�16
	call	interrupt_entry
	leal	do_mf,%eax
	jmp		no_error_code

.align 4
machine_check_int:								# �жϺ�18
	call	interrupt_entry
	leal	do_machine_check,%eax
	jmp		no_error_code

.align 4
xf_int:											# �жϺ�19
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
