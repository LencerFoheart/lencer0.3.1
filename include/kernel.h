/*
 * Small/include/kernel.h
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * 此文件包含内核的一些普通函数。
 * 主要包括：kernel_printf, panic, debug, fork ...
 */

#ifndef _KERNEL_H_
#define _KERNEL_H_

/********************************************************************/

/* kernel_printf */
extern int _k_printf(char *fmt, ...);
extern int k_printf(char *fmt, ...);


/* panic */
extern void panic(const char * str);


/* debug */

/*
#ifndef __DEBUG__
#define __DEBUG__
#endif
*/

#define __BOCHS__
//#define __VMWARE__


#if defined(__BOCHS__)
#define DELAY_BASE (10000)
#elif defined(__VMWARE__)
#define DELAY_BASE (150000)
#endif


#ifdef __DEBUG__

#define d_printf _k_printf
#define D_DELAY(DELAY_COUNT) \
do { \
	for(int i=0; i<(DELAY_COUNT); i++) \
		for(int j=0; j<DELAY_BASE; j++) \
			; \
} while(0)
#define d_delay(delay_count) debug_delay(delay_count)

#else	/* __DEBUG__ */

#define d_printf no_printf
#define D_DELAY(DELAY_COUNT)
#define d_delay(delay_count)

#endif	/* __DEBUG__ */

extern void files_init(void);
extern void debug_delay(unsigned long delay_count);
extern void no_printf(char * fmt, ...);
extern void debug_init(void);


/* 文件系统之后的附加数据的格式说明。具体参见mkfs。 */
struct addition {
	unsigned char fileflag;		/* 为0则表示后面没有附加的文件数据了 */
	unsigned char namelen;		/* 文件名长度 */
	unsigned long datalen;		/* 数据长度 */
};


/* fork */
extern int sys_fork(long none, long ebx, long ecx, long edx,
				long gs, long fs, long es, long ds,
				long ebp, long esi, long edi,
				long eip, long cs, long eflags, long esp, long ss);
extern void copy_process(int nr, long gs, long fs, long es, long ds, 
				long edi, long esi, long ebp, long edx, long ecx, long ebx,
				long eip, long cs, long eflags, long esp, long ss);
extern int find_empty_process(void);


/* trap */
extern void trap_init(void);


/* exit */
extern void sys_exit(void);


/* halt */
extern void sys_shutdown(void);
extern void sys_restart(void);

/********************************************************************/

#endif /* _KERNEL_H_ */