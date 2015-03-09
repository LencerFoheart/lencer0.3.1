#
# Small/boot/head.s
#
# (C) 2012-2013 Yafei Zheng <e9999e@163.com>
#

# ********************************************************************
# ��������GNU as 2.21.1
#
# head.s����Ҫ�������£�
#	1.��������GDT��IDT���ں˶�ջ������0�û�̬��ջ��
#	2.����Ĭ���ж�
#	3.����ҳ��������ҳģʽ
# ********************************************************************

BOOTPOS		= 0x90000		# �����������ƶ�����λ�á��� INITSEG * 16
SCRN_SEL	= 0x18			# ��Ļ��ʾ�ڴ��ѡ���
V_KERNEL_ZONE_START	= 0xC0000000	# �ں����Ե�ַ�ռ俪ʼ����NOTE!
									# �ں������ַ�ռ�Ϊ��3GB - 4GB.
KERNEL_PG_DIR_START	= 768	# �ں˵�ҳĿ¼�Ŀ�ʼ�

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

# ��bss�����㡣����֪��δ��ʼ�����߳�ʼ��Ϊ0��ȫ�ֱ�����static������
# ������bss�Σ�Ӧ�ó���������֮ǰ����bss�λᱻ����ϵͳ���㣬��������
# ��������OS����֮ǰ���һ���Ϊbssû�����㵼�µ���ִ������������ :-)
	leal	_edata,%eax
	leal	_end,%ebx
1:	cmpl	%eax,%ebx
	je		2f
	movb	$0,(%eax)
	incl	%eax
	jmp		1b
2:

# ���ý���0��ҳĿ¼��ҳ����������ҳģʽ��
# NOTE! �򿪷�ҳģʽ֮�������ں�����ռ�Ϊ3GB-4GB����Ŀǰ�Ķλ�ַ����
# 0x00���ʶ�GDT�����������Ҫ���Ͻ���! ��Ϊ�˴���ʹ���ŷǹ���ҳĿ¼
# (��0�ʼ��ҳĿ¼)��
	call	setup_page
# ���µ�λ����������GDT,IDT
	call	setup_gdt
	call	setup_idt
	movl	$0x10,%eax
	mov		%ax,%ds
	mov		%ax,%es
	mov		%ax,%fs
# NOTE! ���ں�̬ʱ������Ĭ����gsָ���Դ�Ρ�So, ���ں�̬����gs�������¶���
	movl	$SCRN_SEL,%eax
	mov		%ax,%gs
	lss		init_stack,%esp

# ��8259A�ر�̣��������жϺ�Ϊint 0x20-0x2F���˴������linux-0.11
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
	ret						# ����main����ִ��
die:
	jmp		die

# ----------------------------------------------------

#
# ���ý���0��ҳĿ¼���ҳ�����������ҳģʽ���ں�Ĭ���ڴ��СΪ16MB��
#
# NOTE! ����0ҳĿ¼��������û�̬ҳĿ¼����ں�̬ҳĿ¼���
#
.align 4
setup_page:
	# bss���ѱ����㣬��ҳ���������㡣
	# �����û�̬ҳĿ¼���NOTE! ����������GDT֮ǰ����Ҫ��ЩҳĿ¼�
	# NOTE: ��ʼ�����̵��û�̬ҳĿ¼���ʼ���������ڴ����ģ�鱣��һ�¡�
	movl	$0*4+pg_dir,%eax					# �û�̬��ҳĿ¼���ַ
	movl	$pg0+7,0(%eax)						# U/S - 1, R/W - 1, P - 1
	# �����ں�̬ҳĿ¼����ں�����ռ�Ϊ3GB-4GB��
	movl	$KERNEL_PG_DIR_START*4+pg_dir,%eax	# �ں�̬��ҳĿ¼���ַ
	movl	$pg0+3,0(%eax)						# U/S - 0, R/W - 1, P - 1
	movl	$pg1+3,4(%eax)
	movl	$pg2+3,8(%eax)
	movl	$pg3+3,12(%eax)
	movl	$pg4+3,512(%eax)	# �����Դ��Ӧ��ҳ����ʼ�����ַ0xE0000000
	# ��������ҳ����
	movl	$16*1024*1024-4096+7,%eax	# ���һҳ�ڴ�ĵ�ַ��7: U/S - 1, R/W - 1, P - 1
	movl	$pg0+4096*4-4,%ebx			# ���һ��ҳ����ĵ�ַ
	movl	$1024*4,%ecx
1:	movl	%eax,(%ebx)
	sub		$4096,%eax
	sub		$4,%ebx
	dec		%ecx
	cmpl	$0,%ecx
	jne		1b
	# ��������VESA�Դ�ҳ����ʼ�����ַ0xE0000000
	movl	$0xE0000000+4*1024*1024-4096+7,%eax	# ���һҳ�ڴ�ĵ�ַ��7: U/S - 1, R/W - 1, P - 1
	movl	$pg4+1024*4-4,%ebx					# ���һ��ҳ����ĵ�ַ
	movl	$1024,%ecx
1:	movl	%eax,(%ebx)
	sub		$4096,%eax
	sub		$4,%ebx
	dec		%ecx
	cmpl	$0,%ecx
	jne		1b
	# ���ô���ռ�ֻ����Ȼ������cr0�е�д����λ���Է�ֹ�ں����û���ֻ��
	# �ռ�д�룬����Ϊ��дʱ����(�μ��ڴ����)��ͬʱ��ֹ�ں�д���ں˴���
	# �ռ䣬����debug�ں˴��롣������д��ȫ�ֱ���Ӧ������data�λ���bss�Ρ�
	leal	_etext,%eax
	shrl	$12,%eax
	xorl	%ecx,%ecx
	movl	$0+5,%ebx					# ֻ����U/S - 1, R/W - 0, P - 1
	movl	$pg0,%edx
1:	movl	%ebx,(%edx)
	addl	$4096,%ebx
	addl	$4,%edx
	inc		%ecx
	cmpl	%ecx,%eax
	jne		1b
	movl	%cr0,%eax
	orl		$0x00010000,%eax	# ����д����
	movl	%eax,%cr0
	# ������ҳģʽ��NOTE! CR3��Ҫ���������ַ��
	movl	$pg_dir,%eax
	movl	%eax,%cr3			# ��CR3(3��32λ���ƼĴ���,��20λ��ҳĿ¼��ַ)ָ��ҳĿ¼��
	movl	%cr0,%eax
	orl		$0x80000000,%eax	# ��������ʹ�÷�ҳ����(cr0��PG��־,λ31)
	movl	%eax,%cr0			# ����CR0�з�ҳ��־λPGΪ1
	ret							# NOTE! retˢ��ָ�����

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
	movw	$0x8e00,%dx			# �ж������ͣ���Ȩ��0
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
# ����ע�͵Ĵ��������μ������룬Ȼ�����������ڸ�λ�������롣��8042�в���Ҫ
#	inb		$0x61,%al
#	orb		$0x80,%al
#	.word	0x00eb,0x00eb	# �˴���2��jmp $+2��$Ϊ��ǰָ���ַ������ʱ����
#	outb	%al,$0x61
#	andb	$0x7f,%al
#	.word	0x00eb,0x00eb
#	outb	%al,$0x61
	movb	$0x20,%al		# ��8259A��оƬ����EOI����,����ISR�е���Ӧλ����
	outb	%al,$0x20
	popl	%eax
	iret

# ----------------------------------------------------

.data

# GDT,IDT�޳������Ե�ַ��NOTE: GDT��IDT��LDT��TSS�ξ�����ϵͳ�Σ�
# ���������������Ե�ַ�����ǽ���ȫ�������ں˵�ַ�ռ䣬�Ա�ֻ����
# �˿��Բ�������֮ǰ���Ҵ���Ľ�������û���ַ�ռ�(û�м����ں�
# ��ַ�ռ��ַ)��������CPU���ش���(Triple Fault)�������Ǽ�ʶ���� :-)
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
# NOTE! ��0.3�濪ʼ���ں˿�����ҳģʽ���ں������ַ�ռ���3GB-4GB��
# �û������ַ�ռ�Ϊ0GB-3GB�����������Ҫ����һ��GDT :-)
#
.align 8
gdt:							# ������	ѡ���	���Ի�ַ	�޳�	��Ȩ��
	.quad 0x0000000000000000	# none		-		-			-		-
	.quad 0xc0c39a000000ffff	# text		0x08	0xC0000000	1GB		0
	.quad 0xc0c392000000ffff	# data		0x10	0xC0000000	1GB		0
	.quad 0xc0c0920b80000007	# VGA-mem	0x18	0xC00B8000	32KB	0
	.fill 252,8,0
# 0.3����ǰ��GDT
#gdt:							# ������	ѡ���	���Ի�ַ	�޳�	��Ȩ��
#	.quad 0x0000000000000000	# none		-		-			-		-
#	.quad 0x00c09a0000000fff	# text		0x08	0			16MB	0
#	.quad 0x00c0920000000fff	# data		0x10	0			16MB	0
#	.quad 0x00c0920b80000007	# VGA-mem	0x18	0xB8000		32KB	0
#	.fill 252,8,0


.bss

.align 8
idt:
	.fill 256,8,0

# �ں˳�ʼ����ջ��Ҳ�Ǻ�������0���û�ջ��
.align 4
	.fill 512,4,0
init_stack_bottom:

.align 0x1000
# �����ǽ���0��ҳĿ¼�Ͷ���ҳ��Ŀǰֻ֧��16MB�ڴ棬��ҳ��ֻ��4ҳ��
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
# ��������ʼ�����ַ 0xE0000000 ��Ӧ��ҳ�������Ե�ַҲΪ 0xE0000000 ��
# �õ�ַ�ռ��Ӧ��VESA�Դ��ַ������ֻ����4MB
pg4:
	.fill 0x1000,1,0
