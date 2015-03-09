/*
 * Small/include/sys_nr.h
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * 此文件包含中断号，以及系统调用号定义
 */

#ifndef _SYS_NR_H_
#define _SYS_NR_H_

/********************************************************************/
/* 
 * 以下定义8259A中断处理程序的中断号，程序最开始
 * 对8259A进行了重编程，设置其中断号为0x20 - 0x2f.
 */
#define NR_TIMER_INT		(0x20)
#define NR_KEYBOARD_INT		(0x21)
#define NR_HD_INT			(0x2e)

/********************************************************************/
/* 以下定义系统调用号 */
#define NR_SYS_CALL			(0x80)

/* 以下定义系统调用对应函数标号。此处需与系统调用函数入口表同步!!! */
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


/* 以下是系统调用相关宏定义函数 */
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
/* 以下定义除系统调用之外的不可屏蔽中断 */
#define NR_DIV_INT			(0)			/*	[无出错码]	除0 */
#define NR_DEBUG_INT		(1)			/*	[无出错码]	调试 */
#define NR_NMI_INT			(2)			/*	[无出错码]	外部不可屏蔽中断 */
#define NR_BP_INT			(3)			/*	[无出错码]	断点 */
#define NR_OF_INT			(4)			/*	[无出错码]	溢出 */
#define NR_BR_INT			(5)			/*	[无出错码]	边界溢出 */
#define NR_UD_INT			(6)			/*	[无出错码]	无效操作码 */
#define NR_NM_INT			(7)			/*	[无出错码]	设备不存在，无数学协处理器 */
#define NR_DF_INT			(8)			/*	[有 出错码,0]	双重错误，异常终止 */
#define NR_none1_INT		(9)			/*	[无出错码]	[保留]，386以后的CPU不再产生此异常 */
#define NR_TS_INT			(10)		/*	[有 出错码]	无效的TSS */
#define NR_NP_INT			(11)		/*	[有 出错码]	段不存在 */
#define NR_SS_INT			(12)		/*	[有 出错码]	堆栈段错误 */
#define NR_GP_INT			(13)		/*	[有 出错码]	一般保护错误 */
#define NR_PF_INT			(14)		/*	[有 出错码]	页异常 */
#define NR_none2_INT		(15)		/*	[无出错码]	[保留] */
#define NR_MF_INT			(16)		/*	[无出错码]	x87 FPU浮点错误 */
#define NR_AC_INT			(17)		/*	[有 出错码,0]	对齐检查 */
#define NR_MC_INT			(18)		/*	[无出错码]	机器检查 */
#define NR_XF_INT			(19)		/*	[无出错码]	SIMD浮点异常 */
/*
 ...
 20 -- 31, Intel 保留
 ...
 */

/********************************************************************/
#endif /* _SYS_NR_H_ */