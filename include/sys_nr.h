/*
 * Small/include/sys_nr.h
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * ���ļ������жϺţ��Լ�ϵͳ���úŶ���
 */

#ifndef _SYS_NR_H_
#define _SYS_NR_H_

/********************************************************************/
/* 
 * ���¶���8259A�жϴ��������жϺţ������ʼ
 * ��8259A�������ر�̣��������жϺ�Ϊ0x20 - 0x2f.
 */
#define NR_TIMER_INT		(0x20)
#define NR_KEYBOARD_INT		(0x21)
#define NR_HD_INT			(0x2e)

/********************************************************************/
/* ���¶���ϵͳ���ú� */
#define NR_SYS_CALL			(0x80)

/* ���¶���ϵͳ���ö�Ӧ������š��˴�����ϵͳ���ú�����ڱ�ͬ��!!! */
#define NR_fork				(0)
#define NR_pause			(1)
#define NR_exit				(2)
#define NR_execve			(3)
#define NR_open				(4)
#define NR_creat			(5)
#define NR_read				(6)
#define NR_write			(7)
#define NR_lseek			(8)
#define NR_close			(9)
#define NR_dup				(10)
#define NR_fstat			(11)
#define NR_stat				(12)
#define NR_chdir			(13)
#define NR_chroot			(14)
#define NR_link				(15)
#define NR_unlink			(16)
#define NR_pipe				(17)
#define NR_sync				(18)
#define NR_shutdown			(19)
#define NR_restart			(20)
#define NR__console_write	(21)


/* ������ϵͳ������غ궨�庯�� */
#define syscall_0(type, name) \
type name(void) \
{ \
	long ret = -1; \
	__asm__ __volatile__( \
		"int $0x80" \
		:"=a"(ret) \
		:"S"(NR_##name) : \
	); \
	return (type)ret; \
}

#define syscall_1(type, name, atype, a) \
type name(atype a) \
{ \
	long ret = -1; \
	__asm__ __volatile__( \
		"int $0x80" \
		:"=a"(ret) \
		:"S"(NR_##name),"0"(a) : \
	); \
	return (type)ret; \
}

#define syscall_2(type, name, atype, a, btype, b) \
type name(atype a, btype b) \
{ \
	long ret = -1; \
	__asm__ __volatile__( \
		"int $0x80" \
		:"=a"(ret) \
		:"S"(NR_##name),"0"(a),"b"(b) : \
	); \
	return (type)ret; \
}

#define syscall_3(type, name, atype, a, btype, b, ctype, c) \
type name(atype a, btype b, ctype c) \
{ \
	long ret = -1; \
	__asm__ __volatile__( \
		"int $0x80" \
		:"=a"(ret) \
		:"S"(NR_##name),"0"(a),"b"(b),"c"(c) : \
	); \
	return (type)ret; \
}


/********************************************************************/
/* ���¶����ϵͳ����֮��Ĳ��������ж� */
#define NR_DIV_INT			(0)			/*	[�޳�����]	��0 */
#define NR_DEBUG_INT		(1)			/*	[�޳�����]	���� */
#define NR_NMI_INT			(2)			/*	[�޳�����]	�ⲿ���������ж� */
#define NR_BP_INT			(3)			/*	[�޳�����]	�ϵ� */
#define NR_OF_INT			(4)			/*	[�޳�����]	��� */
#define NR_BR_INT			(5)			/*	[�޳�����]	�߽���� */
#define NR_UD_INT			(6)			/*	[�޳�����]	��Ч������ */
#define NR_NM_INT			(7)			/*	[�޳�����]	�豸�����ڣ�����ѧЭ������ */
#define NR_DF_INT			(8)			/*	[�� ������,0]	˫�ش����쳣��ֹ */
#define NR_none1_INT		(9)			/*	[�޳�����]	[����]��386�Ժ��CPU���ٲ������쳣 */
#define NR_TS_INT			(10)		/*	[�� ������]	��Ч��TSS */
#define NR_NP_INT			(11)		/*	[�� ������]	�β����� */
#define NR_SS_INT			(12)		/*	[�� ������]	��ջ�δ��� */
#define NR_GP_INT			(13)		/*	[�� ������]	һ�㱣������ */
#define NR_PF_INT			(14)		/*	[�� ������]	ҳ�쳣 */
#define NR_none2_INT		(15)		/*	[�޳�����]	[����] */
#define NR_MF_INT			(16)		/*	[�޳�����]	x87 FPU������� */
#define NR_AC_INT			(17)		/*	[�� ������,0]	������ */
#define NR_MC_INT			(18)		/*	[�޳�����]	������� */
#define NR_XF_INT			(19)		/*	[�޳�����]	SIMD�����쳣 */
/*
 ...
 20 -- 31, Intel ����
 ...
 */

/********************************************************************/
#endif /* _SYS_NR_H_ */