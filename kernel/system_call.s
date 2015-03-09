#
# Small/kernel/system_call.s
# 
# (C) 2012-2013 Yafei Zheng <e9999e@163.com>
#

#
# 此文件包含部分系统调用以及中断处理程序。
#

SYS_CALL_COUNT		= 1			# 系统调用数

.globl interrupt_entry, ret_from_interrupt, system_call, keyboard_int, 
.globl timer_int, hd_int
.globl debug_call


#
# 以下是中断入口函数和中断出口函数。每次中断或系统调用开始或结束时，
# 都要调用。进入中断时，eax、ebx、ecx、edx可能保存着函数参数，esi可能
# 保存着系统调用号，退出中断时，eax可能保存着返回值。故，我们将eax保存
# 在栈顶，ebx、ecx、edx则依次次之。若eax保存有我们需要的返回值时，那么，
# 我们只需要在调用中断退出函数之前，改变栈顶的eax值即可，我们可以使用 
# movl %eax,%ss:(%esp) 实现。
#
# 在中断入口函数中，我们将fs指向用户地址空间，gs指向显存基址。在内核中，
# 很多地方的数据传送，我们使fs指向源地址段或者目的地址段。若需要让fs
# 临时指向内核地址空间，就修改它吧，但别忘了再恢复它 :-)
#
# NOTE!!! 中断入口函数若修改，请特别留意。你不仅需要相应地修改中断出口函数，
# 而且需要修改与压栈顺序息息相关的 sys_fork() 等函数参数顺序以及声明等等。
#

# 中断入口函数。NOTE! 函数开始处将返回地址保存，在函数结束时重新压栈。
.align 4
interrupt_entry:
	xchgl	%edi,%ss:(%esp)		# edi压栈，并换出返回地址
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
	pushl	%edi				# 返回地址压栈
	movw	$0x10,%di			# 全局数据段选择符
	movw	%di,%ds
	movw	%di,%es
	movw	$0x17,%di			# 局部数据段选择符,指向用户地址空间
	movw	%di,%fs
	movw	$0x18,%di			# 显存数据段选择符,指向显存地址空间
	movw	%di,%gs
	movl	%ss:44(%esp),%edi	# 恢复edi
	ret

# 中断出口函数。
.align 4
ret_from_interrupt:
	popl	%edi				# 保存返回地址
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
	xchgl	%edi,%ss:(%esp)		# 返回地址入栈，edi弹出
	ret


msg1:
	.ascii "system_call: nr of system call out of the limit!"
	.byte	0

.align 4
bad_system_call:
	lea		msg1,%eax
	pushl	%eax
	call	panic
	addl	$4,%esp				# NOTE! 实际上不可能执行到这里，but...
	iret

.align 4
system_call:
# [??] 还未限制系统调用数
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
	
# 键盘中断处理程序
.align 4
keyboard_int:
	call	interrupt_entry
	xorb	%al,%al
	# 8042，测试64h端口位0，查看输出缓冲是否满，若满则读出一个字符
	inb		$0x64,%al
	andb	$0x01,%al
	cmpb	$0, %al
	je		1f
	# 输出缓冲满，则调用键盘处理程序读出一个字符
	call	keyboard_hander
# 以下注释的代码是屏蔽键盘输入，然后再允许，
# 用于复位键盘输入。在8042中也可以不用。
#	inb		$0x61,%al
#	orb		$0x80,%al
#	.word	0x00eb,0x00eb	# 2条jmp $+2，$为当前指令地址，起延时作用
#	outb	%al,$0x61
#	andb	$0x7f,%al
#	.word	0x00eb,0x00eb
#	outb	%al,$0x61
1:	movb	$0x20,%al	# 向8259A主芯片发送EOI命令,将其ISR中的相应位清零
	outb	%al,$0x20
	call	ret_from_interrupt
	iret

.align 4
timer_int:
	call	interrupt_entry
	movb	$0x20,%al	# 向8259A主芯片发送EOI命令,将其ISR中的相应位清零。
						# NOTE! 须在do_timer前EOI
	outb	%al,$0x20
	movl	%ss:48(%esp),%eax	# 获取cs寄存器的值，以判断特权级
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
	movb	$0x20,%al	# 向8259A从芯片发送EOI命令,将其ISR中的相应位清零。
	outb	%al,$0xa0
	movb	$0x20,%al	# 向8259A主芯片发送EOI命令,将其ISR中的相应位清零。
						# NOTE! 此处同时起到延时作用。
	outb	%al,$0x20
	movl	do_hd,%eax
	cmpl	$0,%eax
	jne		1f
	leal	hd_default_int,%eax
	movl	%eax,do_hd	
1:	call	*do_hd		# NOTE!!! 此处call的是绝对地址，需加星号!!!
	call	ret_from_interrupt
	iret
