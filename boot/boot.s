!
! Small/boot/boot.s
!
! (C) 2012-2013 Yafei Zheng <e9999e@163.com>
!

! ********************************************************************
! 编译器：as86 0.16.18
!
! boot.s的主要工作如下：
!	1.将内核加载到0x10000，然后将其移动到0x0处
!	2.设置好临时的GDT，，选通A20地址线，切换到保护模式
!	3.跳转到head模块执行
!
! 注意：因为时间问题，此启动代码不检测磁盘类型，而默认使用1.44M软盘。
! 1.44M软盘有2个面（磁头号0和1），每面80个磁道（磁道号0 ~ 79），每道18
! 个扇区（1 ~ 18，没错，是1 ~ 18）。2 * 80 * 18 * 512B = 1440KB，并没有
! 1.44MB。
! ********************************************************************

! 设置以下值时，注意：内存0x10000以前为中断服务程序、中断表、BIOS数据区，
! 内存640KB 以后为BIOS和显存区域（ROM），这些区域起初均不能被占用。
BOOTSEG		= 0x07c0				! 启动代码被载入的位置
INITSEG		= 0x9000				! 启动代码被移动到的位置
SYSSEG		= 0x1000				! 系统被载入位置
TEMPSTK		= 0x9050				! 用于临时堆栈
SYSLEN		= 260					! 内核占用磁盘最大扇区数
! 以下是显示模式，其中前面的4表示使用线性地址
!VESA_MODE	= 0x10E					! 320*200,真彩5:6:5
!VESA_MODE	= 0x111					! 640*480,真彩5:6:5
VESA_MODE	= 0x114					! 800*600,真彩5:6:5
!VESA_MODE	= 0x117					! 1024*768,真彩5:6:5

entry start
start:
! 将启动代码移至 INITSEG:0
	mov		ax,#BOOTSEG
	mov		ds,ax
	mov		ax,#INITSEG
	mov		es,ax
	mov		cx,#256
	sub		si,si
	sub		di,di
	rep
	movw

! 跳转至 INITSEG:go 开始继续执行
	jmpi	go,#INITSEG
go:	mov		ax,cs
	mov		ds,ax
	mov		es,ax
	mov		ax,#TEMPSTK
	mov		ss,ax
	mov		sp,#0x0400				! 设置临时堆栈

! 显示载入提示信息
	mov		bp,#msg1
	mov		cx,#21
	call	showmsg
	jmp		load_system				! right! 跳过下面定义的变量

! 加载内核代码到 SYSSEG:0，共 SYSLEN 个扇区。加载时，须防止"DMA超过
! 64KB界限"这种错误发生。
! INT 13h 的使用方法如下：
!	ah = 02h - 读磁盘扇区到内存；	al = 需要读出的扇区数量；
!	ch = 磁道(柱面)号的低8位；		cl = 开始扇区(0－5位)，磁道号高2位(6－7)；
!	dh = 磁头号；					dl = 驱动器号(如果是硬盘则要置为7)；
!	es:bx ->指向数据缓冲区；
!	如果出错则CF标志置位。 

! 注意：须将以下变量跳过，他们不能被执行!
head:	.word 0			! 当前磁头号
track:	.word 0			! 当前磁道号
sread:	.word 1			! 当前磁道已读扇区数
canread:.word 128		! 为防止DMA超过64KB界限，最大可读取扇区数。该变量递减，
						! 若为0，则已读取128扇区（64KB），就重新从128开始。
left:	.word SYSLEN	! 还需读取扇区数
loadpos:.word SYSSEG	! 被载入的位置，这里是 es 的值。

load_system:
	mov		ax,loadpos
	mov		es,ax
	mov		ax,#18
	sub		ax,sread
	cmp		ax,left
	jna		go1
	mov		ax,left
go1:
	cmp		canread,ax
	ja		go2
	mov		ax,canread
	mov		canread,#128
	jmp		go3
go2:
	sub		canread,ax
go3:
	push	ax						! 将准备读取的扇区数保存于栈中
	mov		ch,track
	mov		cl,sread
	add		cl,#1
	mov		dh,head
	xor		bx,bx
	mov		ah,#0x02
	mov		dl,#0x00
	int		0x13
	pop		ax						! 栈中保存着每次读取的扇区数
	jc		read_error				! 若读取出错，则显示提示信息之后，死机
	add		sread,ax
	sub		left,ax
	cmp		left,#0
	je		ok_load
	mov		cx,#0x0020
	mul		cx
	add		loadpos,ax	! 将载入位置加上 (ax * 0x20)，其中0x20 = (512/16)
	cmp		sread,#18
	jne		load_system
	mov		sread,#0x0000
	cmp		head,#0x0000			! 不能是 test
	je		head_to_1
	mov		head,#0x0000
	inc		track
	jmp		load_system
head_to_1:
	mov		head,#0x0001
	jmpi	load_system,#INITSEG	! 超出了短跳转，故用长跳转
read_error:
	mov		bp,#msg2
	mov		cx,#11
	call	showmsg
die:
	jmp		die

! 以下开始获取并保存系统相关信息，注意，这里将覆盖前面执行过的代码，
! 希望不要超出范围 :-)

! 获取光标位置并保存，注意此处保存的是转换之后的线性值
! BIOS 中断10h 的读光标功能号ah = 03
! 输入：bh = 页号
! 返回：ch = 扫描开始线，cl = 扫描结束线，
!		dh = 行号(00 是顶端)，dl = 列号(00 是左边)。	
ok_load:
	mov		ax,#INITSEG				! Oh，这里不应再是 BOOTSEG
	mov		ds,ax
	mov		ah,#0x03
	xor		bh,bh
	int		0x10
	mov		ax,#80					! 默认的80 * 25显示模式
	mul		dh
	mov		dh,#0
	add		ax,dx
	mov		[0],ax					! ds = INITSEG， 从 INITSEG 开始

! 获取扩展内存大小（KB），多于1mb的内存
! 调用中断15h，功能号ah = 88
! 返回：ax = 从100000（1M）处开始的扩展内存大小(KB)。
! 		若出错则CF 置位，ax = 出错码。
	mov		ah,#0x88
	int		0x15
	mov		[2],ax

! 保存第一个硬盘参数表。0x41中断向量处存放的不是中断程序地址，而是
! 第一个硬盘参数表的地址。
	cld
	mov		cx,#16/2
	mov		ax,#INITSEG
	mov		es,ax
	mov		di,#4
	xor		ax,ax
	mov		ds,ax
	lds		si,[4*0x41]		! NOTE! ds = 0
	rep
	movw
	
! 设置显卡显示模式
!	mov		ax,#0x4f02
!	mov		bx,#0x4000+VESA_MODE
!	int		0x10
! 获取显示模式对应信息.返回的信息是一个256字节的，很庞大的结构.存储在es:di
!	mov		ax,#INITSEG
!	mov		es,ax	
!	mov		di,#20
!	mov		ax,#0x4f01
!	mov		cx,#VESA_MODE
!	int		0x10

! 将内核移动到0x0处
	cli						! 关中断
	xor		bx,bx
rp_move:
	mov		cx,#SYSSEG
	mov		ax,#0x0020		! 每移动 512 字节，段寄存器加 0x20
	mul		bx
	add		cx,ax
	mov		ds,cx
	mov		es,ax
	mov		cx,#256			! 每次移动 512 字节，共需移动 SYSLEN 次
	sub		si,si
	sub		di,di
	rep
	movw
	inc		bx
	cmp		bx,#SYSLEN		! 注意 test 与 cmp 的区别 。在此不能用 test
	jne		rp_move

! 装载GDTR,IDTR
	mov		ax,#INITSEG		! Oh，这里不应再是 BOOTSEG
	mov		ds,ax
	mov		es,ax
	lgdt	gdt_48
	lidt	idt_48

! 打开A20地址线
	call	empty_8042
	mov		al,#0xd1
	out		#0x64,al
	call	empty_8042
	mov		al,#0xdf
	out		#0x60,al
	call	empty_8042

! 置CR0的PE位进入保护模式，并跳转至head模块执行
	mov		ax,#0x0001
	lmsw	ax
	jmpi	0,8
! debug:
!	jmp		debug

! -----------------------------------------------
empty_8042:
	.word 0x00eb,0x00eb
	in		al,#0x64
	test	al,#0x02
	jnz		empty_8042
	ret

showmsg:
	push	cx				! 需保存cx、bp的值
	push	bp
	mov		ah,#0x03
	xor		bh,bh
	int		0x10			! 获取光标位置,保存于dx中
	pop		bp
	pop		cx
	mov		ax,cs
	mov		es,ax
	mov		ax,#0x1301
	mov		bx,#0x0007		! 0页，属性7（normal）
	int		0x10
	ret

! -----------------------------------------------
! 提示信息定义。msg1为载入提示信息，共 21 个字符；
! msg2为载入失败提示信息，共11个字符
msg1:
	.byte 13,10
	.ascii "Loading System..."
	.byte 13,10

msg2:
	.ascii "Load Error!"

! GDT,IDT定义
gdt:
	.word 0,0,0,0			! 不用

	.word 0x07ff			! 代码段描述符
	.word 0x0000
	.word 0x9a00
	.word 0x00c0

	.word 0x07ff			! 数据段描述符
	.word 0x0000
	.word 0x9200
	.word 0x00c0

gdt_48:
	.word 0x07ff	! 段限长
	.word gdt,9		! 注意!段基址不再是0x7c00,0. 嗯,花了半天功夫才发现 :-(

idt_48:
	.word 0
	.word 0,0

! 引导扇区有效标志，必须位于引导扇区最后2个字节
.org 510
	.word 0xaa55
