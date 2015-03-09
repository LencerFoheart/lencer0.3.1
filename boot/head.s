#
# Small/boot/head.s
#
# (C) 2012-2013 Yafei Zheng <e9999e@163.com>
#

# ********************************************************************
# 编译器：GNU as 2.21.1
#
# head.s的主要工作如下：
#	1.重新设置GDT，IDT，内核堆栈（任务0用户态堆栈）
#	2.设置默认中断
#	3.设置页表，开启分页模式
# ********************************************************************

BOOTPOS		= 0x90000		# 启动扇区被移动到的位置。即 INITSEG * 16
SCRN_SEL	= 0x18			# 屏幕显示内存段选择符
V_KERNEL_ZONE_START	= 0xC0000000	# 内核线性地址空间开始处。NOTE!
									# 内核虚拟地址空间为：3GB - 4GB.
KERNEL_PG_DIR_START	= 768	# 内核的页目录的开始项。

.globl startup_32
.globl pg_dir, idt, gdt

.text

startup_32:
	movl	$0x10,%eax
	mov		%ax,%ds
	mov		%ax,%es
	mov		%ax,%fs
	mov		%ax,%gs
	lss		init_stack,%esp

# 将bss段清零。我们知道未初始化或者初始化为0的全局变量或static变量被
# 链接在bss段，应用程序在运行之前，其bss段会被操作系统清零，但别忘了
# 我们这是OS本身。之前，我还因为bss没有清零导致的奇怪错误而崩溃过呢 :-)
	leal	_edata,%eax
	leal	_end,%ebx
1:	cmpl	%eax,%ebx
	je		2f
	movb	$0,(%eax)
	incl	%eax
	jmp		1b
2:

# 设置进程0的页目录和页表，并开启分页模式。
# NOTE! 打开分页模式之后，由于内核虚拟空间为3GB-4GB，而目前的段基址还是
# 0x00，故对GDT表的重新设置要马上进行! 因为此处还使用着非规格的页目录
# (从0项开始的页目录)。
	call	setup_page
# 在新的位置重新设置GDT,IDT
	call	setup_gdt
	call	setup_idt
	movl	$0x10,%eax
	mov		%ax,%ds
	mov		%ax,%es
	mov		%ax,%fs
# NOTE! 在内核态时，我们默认让gs指向显存段。So, 在内核态别用gs做其他事儿。
	movl	$SCRN_SEL,%eax
	mov		%ax,%gs
	lss		init_stack,%esp

# 对8259A重编程，设置其中断号为int 0x20-0x2F。此代码参照linux-0.11
	mov		$0x11,%al		# initi%alization sequence
	out		%al,$0x20		# send it to 8259A-1
	.word	0x00eb,0x00eb	# jmp $+2, jmp $+2
	out		%al,$0xA0		# and to 8259A-2
	.word	0x00eb,0x00eb
	mov		$0x20,%al		# start of hardware int's (0x20)
	out		%al,$0x21
	.word	0x00eb,0x00eb
	mov		$0x28,%al		# start of hardware int's 2 (0x28)
	out		%al,$0xA1
	.word	0x00eb,0x00eb
	mov		$0x04,%al		# 8259-1 is master
	out		%al,$0x21
	.word	0x00eb,0x00eb
	mov		$0x02,%al		# 8259-2 is slave
	out		%al,$0xA1
	.word	0x00eb,0x00eb
	mov		$0x01,%al		# 8086 mode for both
	out		%al,$0x21
	.word	0x00eb,0x00eb
	out		%al,$0xA1
	.word	0x00eb,0x00eb

# OK! We begin to run the MAIN function now.
	pushl	$die
	pushl	$main
	ret						# 进入main函数执行
die:
	jmp		die

# ----------------------------------------------------

#
# 设置进程0的页目录项和页表项，并开启分页模式。内核默认内存大小为16MB。
#
# NOTE! 进程0页目录项包括：用户态页目录表项、内核态页目录表项。
#
.align 4
setup_page:
	# bss段已被清零，故页表无需清零。
	# 设置用户态页目录表项。NOTE! 在重新设置GDT之前，需要这些页目录项。
	# NOTE: 初始化进程的用户态页目录表初始项数需与内存管理模块保持一致。
	movl	$0*4+pg_dir,%eax					# 用户态的页目录项地址
	movl	$pg0+7,0(%eax)						# U/S - 1, R/W - 1, P - 1
	# 设置内核态页目录表项。内核虚拟空间为3GB-4GB。
	movl	$KERNEL_PG_DIR_START*4+pg_dir,%eax	# 内核态的页目录项地址
	movl	$pg0+3,0(%eax)						# U/S - 0, R/W - 1, P - 1
	movl	$pg1+3,4(%eax)
	movl	$pg2+3,8(%eax)
	movl	$pg3+3,12(%eax)
	movl	$pg4+3,512(%eax)	# 这是显存对应的页表。起始物理地址0xE0000000
	# 倒序设置页表项
	movl	$16*1024*1024-4096+7,%eax	# 最后一页内存的地址。7: U/S - 1, R/W - 1, P - 1
	movl	$pg0+4096*4-4,%ebx			# 最后一个页表项的地址
	movl	$1024*4,%ecx
1:	movl	%eax,(%ebx)
	sub		$4096,%eax
	sub		$4,%ebx
	dec		%ecx
	cmpl	$0,%ecx
	jne		1b
	# 倒序设置VESA显存页表。起始物理地址0xE0000000
	movl	$0xE0000000+4*1024*1024-4096+7,%eax	# 最后一页内存的地址。7: U/S - 1, R/W - 1, P - 1
	movl	$pg4+1024*4-4,%ebx					# 最后一个页表项的地址
	movl	$1024,%ecx
1:	movl	%eax,(%ebx)
	sub		$4096,%eax
	sub		$4,%ebx
	dec		%ecx
	cmpl	$0,%ecx
	jne		1b
	# 设置代码空间只读，然后设置cr0中的写保护位。以防止内核向用户级只读
	# 空间写入，这是为了写时复制(参见内存管理)。同时防止内核写入内核代码
	# 空间，方便debug内核代码。因此需读写的全局变量应定义在data段或者bss段。
	leal	_etext,%eax
	shrl	$12,%eax
	xorl	%ecx,%ecx
	movl	$0+5,%ebx					# 只读，U/S - 1, R/W - 0, P - 1
	movl	$pg0,%edx
1:	movl	%ebx,(%edx)
	addl	$4096,%ebx
	addl	$4,%edx
	inc		%ecx
	cmpl	%ecx,%eax
	jne		1b
	movl	%cr0,%eax
	orl		$0x00010000,%eax	# 设置写保护
	movl	%eax,%cr0
	# 开启分页模式。NOTE! CR3需要的是物理地址。
	movl	$pg_dir,%eax
	movl	%eax,%cr3			# 将CR3(3号32位控制寄存器,高20位存页目录基址)指向页目录表
	movl	%cr0,%eax
	orl		$0x80000000,%eax	# 设置启动使用分页处理(cr0的PG标志,位31)
	movl	%eax,%cr0			# 设置CR0中分页标志位PG为1
	ret							# NOTE! ret刷新指令缓冲流

.align 4
setup_gdt:
	lgdt	gdt_new_48
	ret

.align 4
setup_idt:
	pushl	%edx
	pushl	%eax
	pushl	%ecx
	pushl	%edi
	lea		ignore_int,%edx
	movl	$0x00080000,%eax
	mov		%dx,%ax
	movw	$0x8e00,%dx			# 中断门类型，特权级0
	lea		idt,%edi
	movl	$256,%ecx
rp:	movl	%eax,(%edi)
	movl	%edx,4(%edi)
	addl	$8,%edi
	dec		%ecx
	cmpl	$0,%ecx
	jne		rp
	lidt	idt_new_48
	popl	%edi
	popl	%ecx
	popl	%eax
	popl	%edx
	ret

.align 4
ignore_int:
	pushl	%eax	
# 下面注释的代码是屏蔽键盘输入，然后再允许，用于复位键盘输入。在8042中不需要
#	inb		$0x61,%al
#	orb		$0x80,%al
#	.word	0x00eb,0x00eb	# 此处是2条jmp $+2，$为当前指令地址，起延时作用
#	outb	%al,$0x61
#	andb	$0x7f,%al
#	.word	0x00eb,0x00eb
#	outb	%al,$0x61
	movb	$0x20,%al		# 向8259A主芯片发送EOI命令,将其ISR中的相应位清零
	outb	%al,$0x20
	popl	%eax
	iret

# ----------------------------------------------------

.data

# GDT,IDT限长和线性地址。NOTE: GDT、IDT、LDT、TSS段均属于系统段，
# 其描述符均是线性地址，我们将其全部放入内核地址空间，以便只有内
# 核可以操作它。之前，我错误的将其放入用户地址空间(没有加上内核
# 地址空间基址)，导致了CPU三重错误(Triple Fault)，这算是见识到了 :-)
.align 4
gdt_new_48:
	.word 256*8-1
	.long gdt+V_KERNEL_ZONE_START

idt_new_48:
	.word 256*8-1
	.long idt+V_KERNEL_ZONE_START

init_stack:
	.long init_stack_bottom
	.word 0x10

#
# NOTE! 从0.3版开始，内核开启分页模式，内核虚拟地址空间在3GB-4GB，
# 用户虚拟地址空间为0GB-3GB，因此我们需要调整一下GDT :-)
#
.align 8
gdt:							# 段类型	选择符	线性基址	限长	特权级
	.quad 0x0000000000000000	# none		-		-			-		-
	.quad 0xc0c39a000000ffff	# text		0x08	0xC0000000	1GB		0
	.quad 0xc0c392000000ffff	# data		0x10	0xC0000000	1GB		0
	.quad 0xc0c0920b80000007	# VGA-mem	0x18	0xC00B8000	32KB	0
	.fill 252,8,0
# 0.3版以前的GDT
#gdt:							# 段类型	选择符	线性基址	限长	特权级
#	.quad 0x0000000000000000	# none		-		-			-		-
#	.quad 0x00c09a0000000fff	# text		0x08	0			16MB	0
#	.quad 0x00c0920000000fff	# data		0x10	0			16MB	0
#	.quad 0x00c0920b80000007	# VGA-mem	0x18	0xB8000		32KB	0
#	.fill 252,8,0


.bss

.align 8
idt:
	.fill 256,8,0

# 内核初始化堆栈，也是后来任务0的用户栈。
.align 4
	.fill 512,4,0
init_stack_bottom:

.align 0x1000
# 以下是进程0的页目录和二级页表。目前只支持16MB内存，故页表只有4页。
pg_dir:
	.fill 0x1000,1,0
pg0:
	.fill 0x1000,1,0
pg1:
	.fill 0x1000,1,0
pg2:
	.fill 0x1000,1,0
pg3:
	.fill 0x1000,1,0
# 以下是起始物理地址 0xE0000000 对应的页表，其线性地址也为 0xE0000000 。
# 该地址空间对应于VESA显存地址，暂且只设置4MB
pg4:
	.fill 0x1000,1,0
