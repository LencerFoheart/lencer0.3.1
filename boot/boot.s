!
! Small/boot/boot.s
!
! (C) 2012-2013 Yafei Zheng <e9999e@163.com>
!

! ********************************************************************
! ��������as86 0.16.18
!
! boot.s����Ҫ�������£�
!	1.���ں˼��ص�0x10000��Ȼ�����ƶ���0x0��
!	2.���ú���ʱ��GDT����ѡͨA20��ַ�ߣ��л�������ģʽ
!	3.��ת��headģ��ִ��
!
! ע�⣺��Ϊʱ�����⣬���������벻���������ͣ���Ĭ��ʹ��1.44M���̡�
! 1.44M������2���棨��ͷ��0��1����ÿ��80���ŵ����ŵ���0 ~ 79����ÿ��18
! ��������1 ~ 18��û����1 ~ 18����2 * 80 * 18 * 512B = 1440KB����û��
! 1.44MB��
! ********************************************************************

! ��������ֵʱ��ע�⣺�ڴ�0x10000��ǰΪ�жϷ�������жϱ�BIOS��������
! �ڴ�640KB �Ժ�ΪBIOS���Դ�����ROM������Щ������������ܱ�ռ�á�
BOOTSEG		= 0x07c0				! �������뱻�����λ��
INITSEG		= 0x9000				! �������뱻�ƶ�����λ��
SYSSEG		= 0x1000				! ϵͳ������λ��
TEMPSTK		= 0x9050				! ������ʱ��ջ
SYSLEN		= 260					! �ں�ռ�ô������������
! ��������ʾģʽ������ǰ���4��ʾʹ�����Ե�ַ
!VESA_MODE	= 0x10E					! 320*200,���5:6:5
!VESA_MODE	= 0x111					! 640*480,���5:6:5
VESA_MODE	= 0x114					! 800*600,���5:6:5
!VESA_MODE	= 0x117					! 1024*768,���5:6:5

entry start
start:
! �������������� INITSEG:0
	mov		ax,#BOOTSEG
	mov		ds,ax
	mov		ax,#INITSEG
	mov		es,ax
	mov		cx,#256
	sub		si,si
	sub		di,di
	rep
	movw

! ��ת�� INITSEG:go ��ʼ����ִ��
	jmpi	go,#INITSEG
go:	mov		ax,cs
	mov		ds,ax
	mov		es,ax
	mov		ax,#TEMPSTK
	mov		ss,ax
	mov		sp,#0x0400				! ������ʱ��ջ

! ��ʾ������ʾ��Ϣ
	mov		bp,#msg1
	mov		cx,#21
	call	showmsg
	jmp		load_system				! right! �������涨��ı���

! �����ں˴��뵽 SYSSEG:0���� SYSLEN ������������ʱ�����ֹ"DMA����
! 64KB����"���ִ�������
! INT 13h ��ʹ�÷������£�
!	ah = 02h - �������������ڴ棻	al = ��Ҫ����������������
!	ch = �ŵ�(����)�ŵĵ�8λ��		cl = ��ʼ����(0��5λ)���ŵ��Ÿ�2λ(6��7)��
!	dh = ��ͷ�ţ�					dl = ��������(�����Ӳ����Ҫ��Ϊ7)��
!	es:bx ->ָ�����ݻ�������
!	���������CF��־��λ�� 

! ע�⣺�뽫���±������������ǲ��ܱ�ִ��!
head:	.word 0			! ��ǰ��ͷ��
track:	.word 0			! ��ǰ�ŵ���
sread:	.word 1			! ��ǰ�ŵ��Ѷ�������
canread:.word 128		! Ϊ��ֹDMA����64KB���ޣ����ɶ�ȡ���������ñ����ݼ���
						! ��Ϊ0�����Ѷ�ȡ128������64KB���������´�128��ʼ��
left:	.word SYSLEN	! �����ȡ������
loadpos:.word SYSSEG	! �������λ�ã������� es ��ֵ��

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
	push	ax						! ��׼����ȡ��������������ջ��
	mov		ch,track
	mov		cl,sread
	add		cl,#1
	mov		dh,head
	xor		bx,bx
	mov		ah,#0x02
	mov		dl,#0x00
	int		0x13
	pop		ax						! ջ�б�����ÿ�ζ�ȡ��������
	jc		read_error				! ����ȡ��������ʾ��ʾ��Ϣ֮������
	add		sread,ax
	sub		left,ax
	cmp		left,#0
	je		ok_load
	mov		cx,#0x0020
	mul		cx
	add		loadpos,ax	! ������λ�ü��� (ax * 0x20)������0x20 = (512/16)
	cmp		sread,#18
	jne		load_system
	mov		sread,#0x0000
	cmp		head,#0x0000			! ������ test
	je		head_to_1
	mov		head,#0x0000
	inc		track
	jmp		load_system
head_to_1:
	mov		head,#0x0001
	jmpi	load_system,#INITSEG	! �����˶���ת�����ó���ת
read_error:
	mov		bp,#msg2
	mov		cx,#11
	call	showmsg
die:
	jmp		die

! ���¿�ʼ��ȡ������ϵͳ�����Ϣ��ע�⣬���ｫ����ǰ��ִ�й��Ĵ��룬
! ϣ����Ҫ������Χ :-)

! ��ȡ���λ�ò����棬ע��˴��������ת��֮�������ֵ
! BIOS �ж�10h �Ķ���깦�ܺ�ah = 03
! ���룺bh = ҳ��
! ���أ�ch = ɨ�迪ʼ�ߣ�cl = ɨ������ߣ�
!		dh = �к�(00 �Ƕ���)��dl = �к�(00 �����)��	
ok_load:
	mov		ax,#INITSEG				! Oh�����ﲻӦ���� BOOTSEG
	mov		ds,ax
	mov		ah,#0x03
	xor		bh,bh
	int		0x10
	mov		ax,#80					! Ĭ�ϵ�80 * 25��ʾģʽ
	mul		dh
	mov		dh,#0
	add		ax,dx
	mov		[0],ax					! ds = INITSEG�� �� INITSEG ��ʼ

! ��ȡ��չ�ڴ��С��KB��������1mb���ڴ�
! �����ж�15h�����ܺ�ah = 88
! ���أ�ax = ��100000��1M������ʼ����չ�ڴ��С(KB)��
! 		��������CF ��λ��ax = �����롣
	mov		ah,#0x88
	int		0x15
	mov		[2],ax

! �����һ��Ӳ�̲�����0x41�ж���������ŵĲ����жϳ����ַ������
! ��һ��Ӳ�̲�����ĵ�ַ��
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
	
! �����Կ���ʾģʽ
!	mov		ax,#0x4f02
!	mov		bx,#0x4000+VESA_MODE
!	int		0x10
! ��ȡ��ʾģʽ��Ӧ��Ϣ.���ص���Ϣ��һ��256�ֽڵģ����Ӵ�Ľṹ.�洢��es:di
!	mov		ax,#INITSEG
!	mov		es,ax	
!	mov		di,#20
!	mov		ax,#0x4f01
!	mov		cx,#VESA_MODE
!	int		0x10

! ���ں��ƶ���0x0��
	cli						! ���ж�
	xor		bx,bx
rp_move:
	mov		cx,#SYSSEG
	mov		ax,#0x0020		! ÿ�ƶ� 512 �ֽڣ��μĴ����� 0x20
	mul		bx
	add		cx,ax
	mov		ds,cx
	mov		es,ax
	mov		cx,#256			! ÿ���ƶ� 512 �ֽڣ������ƶ� SYSLEN ��
	sub		si,si
	sub		di,di
	rep
	movw
	inc		bx
	cmp		bx,#SYSLEN		! ע�� test �� cmp ������ ���ڴ˲����� test
	jne		rp_move

! װ��GDTR,IDTR
	mov		ax,#INITSEG		! Oh�����ﲻӦ���� BOOTSEG
	mov		ds,ax
	mov		es,ax
	lgdt	gdt_48
	lidt	idt_48

! ��A20��ַ��
	call	empty_8042
	mov		al,#0xd1
	out		#0x64,al
	call	empty_8042
	mov		al,#0xdf
	out		#0x60,al
	call	empty_8042

! ��CR0��PEλ���뱣��ģʽ������ת��headģ��ִ��
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
	push	cx				! �豣��cx��bp��ֵ
	push	bp
	mov		ah,#0x03
	xor		bh,bh
	int		0x10			! ��ȡ���λ��,������dx��
	pop		bp
	pop		cx
	mov		ax,cs
	mov		es,ax
	mov		ax,#0x1301
	mov		bx,#0x0007		! 0ҳ������7��normal��
	int		0x10
	ret

! -----------------------------------------------
! ��ʾ��Ϣ���塣msg1Ϊ������ʾ��Ϣ���� 21 ���ַ���
! msg2Ϊ����ʧ����ʾ��Ϣ����11���ַ�
msg1:
	.byte 13,10
	.ascii "Loading System..."
	.byte 13,10

msg2:
	.ascii "Load Error!"

! GDT,IDT����
gdt:
	.word 0,0,0,0			! ����

	.word 0x07ff			! �����������
	.word 0x0000
	.word 0x9a00
	.word 0x00c0

	.word 0x07ff			! ���ݶ�������
	.word 0x0000
	.word 0x9200
	.word 0x00c0

gdt_48:
	.word 0x07ff	! ���޳�
	.word gdt,9		! ע��!�λ�ַ������0x7c00,0. ��,���˰��칦��ŷ��� :-(

idt_48:
	.word 0
	.word 0,0

! ����������Ч��־������λ�������������2���ֽ�
.org 510
	.word 0xaa55
