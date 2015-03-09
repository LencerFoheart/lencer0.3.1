#
# Small/kernel/system_call.s
# 
# (C) 2012-2013 Yafei Zheng <e9999e@163.com>
#

#
# ���ļ���������ϵͳ�����Լ��жϴ������
#

SYS_CALL_COUNT		= 1			# ϵͳ������

.globl interrupt_entry, ret_from_interrupt, system_call, keyboard_int, 
.globl timer_int, hd_int
.globl debug_call


#
# �������ж���ں������жϳ��ں�����ÿ���жϻ�ϵͳ���ÿ�ʼ�����ʱ��
# ��Ҫ���á������ж�ʱ��eax��ebx��ecx��edx���ܱ����ź���������esi����
# ������ϵͳ���úţ��˳��ж�ʱ��eax���ܱ����ŷ���ֵ���ʣ����ǽ�eax����
# ��ջ����ebx��ecx��edx�����δ�֮����eax������������Ҫ�ķ���ֵʱ����ô��
# ����ֻ��Ҫ�ڵ����ж��˳�����֮ǰ���ı�ջ����eaxֵ���ɣ����ǿ���ʹ�� 
# movl %eax,%ss:(%esp) ʵ�֡�
#
# ���ж���ں����У����ǽ�fsָ���û���ַ�ռ䣬gsָ���Դ��ַ�����ں��У�
# �ܶ�ط������ݴ��ͣ�����ʹfsָ��Դ��ַ�λ���Ŀ�ĵ�ַ�Ρ�����Ҫ��fs
# ��ʱָ���ں˵�ַ�ռ䣬���޸����ɣ����������ٻָ��� :-)
#
# NOTE!!! �ж���ں������޸ģ����ر����⡣�㲻����Ҫ��Ӧ���޸��жϳ��ں�����
# ������Ҫ�޸���ѹջ˳��ϢϢ��ص� sys_fork() �Ⱥ�������˳���Լ������ȵȡ�
#

# �ж���ں�����NOTE! ������ʼ�������ص�ַ���棬�ں�������ʱ����ѹջ��
.align 4
interrupt_entry:
	xchgl	%edi,%ss:(%esp)		# ediѹջ�����������ص�ַ
	pushl	%esi
	pushl	%ebp
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	pushl	%edi				# ���ص�ַѹջ
	movw	$0x10,%di			# ȫ�����ݶ�ѡ���
	movw	%di,%ds
	movw	%di,%es
	movw	$0x17,%di			# �ֲ����ݶ�ѡ���,ָ���û���ַ�ռ�
	movw	%di,%fs
	movw	$0x18,%di			# �Դ����ݶ�ѡ���,ָ���Դ��ַ�ռ�
	movw	%di,%gs
	movl	%ss:44(%esp),%edi	# �ָ�edi
	ret

# �жϳ��ں�����
.align 4
ret_from_interrupt:
	popl	%edi				# ���淵�ص�ַ
	popl	%eax
	popl	%ebx
	popl	%ecx
	popl	%edx
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popl	%ebp
	popl	%esi
	xchgl	%edi,%ss:(%esp)		# ���ص�ַ��ջ��edi����
	ret


msg1:
	.ascii "system_call: nr of system call out of the limit!"
	.byte	0

.align 4
bad_system_call:
	lea		msg1,%eax
	pushl	%eax
	call	panic
	addl	$4,%esp				# NOTE! ʵ���ϲ�����ִ�е����but...
	iret

.align 4
system_call:
# [??] ��δ����ϵͳ������
#	cmpl	$SYS_CALL_COUNT,%esi
#	jna		bad_system_call
	call	interrupt_entry
	leal	sys_call_table,%eax
	call	*(%eax,%esi,4)
	movl	%eax,%ss:(%esp)
	call	ret_from_interrupt
	iret

.align 4
debug_call:
	call	interrupt_entry
	call	debug
	call	ret_from_interrupt
	iret
	
# �����жϴ������
.align 4
keyboard_int:
	call	interrupt_entry
	xorb	%al,%al
	# 8042������64h�˿�λ0���鿴��������Ƿ��������������һ���ַ�
	inb		$0x64,%al
	andb	$0x01,%al
	cmpb	$0, %al
	je		1f
	# ���������������ü��̴���������һ���ַ�
	call	keyboard_hander
# ����ע�͵Ĵ��������μ������룬Ȼ��������
# ���ڸ�λ�������롣��8042��Ҳ���Բ��á�
#	inb		$0x61,%al
#	orb		$0x80,%al
#	.word	0x00eb,0x00eb	# 2��jmp $+2��$Ϊ��ǰָ���ַ������ʱ����
#	outb	%al,$0x61
#	andb	$0x7f,%al
#	.word	0x00eb,0x00eb
#	outb	%al,$0x61
1:	movb	$0x20,%al	# ��8259A��оƬ����EOI����,����ISR�е���Ӧλ����
	outb	%al,$0x20
	call	ret_from_interrupt
	iret

.align 4
timer_int:
	call	interrupt_entry
	movb	$0x20,%al	# ��8259A��оƬ����EOI����,����ISR�е���Ӧλ���㡣
						# NOTE! ����do_timerǰEOI
	outb	%al,$0x20
	movl	%ss:48(%esp),%eax	# ��ȡcs�Ĵ�����ֵ�����ж���Ȩ��
	pushl	%eax	
	call	do_timer
	addl	$4,%esp
	call	ret_from_interrupt
	iret

#
# OK,we handle the slave '8259A' now. NOTE!!! The difference
# between the slave and master '8259A' isthat the interrupt 
# of slave '8259A' need EOI slave and master. But the interrupt
# of master is ONLYEOI master.
#
.align 4
hd_int:
	call	interrupt_entry
	movb	$0x20,%al	# ��8259A��оƬ����EOI����,����ISR�е���Ӧλ���㡣
	outb	%al,$0xa0
	movb	$0x20,%al	# ��8259A��оƬ����EOI����,����ISR�е���Ӧλ���㡣
						# NOTE! �˴�ͬʱ����ʱ���á�
	outb	%al,$0x20
	movl	do_hd,%eax
	cmpl	$0,%eax
	jne		1f
	leal	hd_default_int,%eax
	movl	%eax,do_hd	
1:	call	*do_hd		# NOTE!!! �˴�call���Ǿ��Ե�ַ������Ǻ�!!!
	call	ret_from_interrupt
	iret
