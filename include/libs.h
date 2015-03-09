/*
 * Small/include/libs.h
 *
 * (C) 2014 Yafei Zheng <e9999e@163.com>
 */

#ifndef _LIBS_H_
#define _LIBS_H_

#include "types.h"

/********************************************************************/

/* 没有参数的 */
extern int fork(void);
extern void pause(void);
extern void shutdown(void);
extern void restart(void);


/* 1个参数的 */
extern BOOL execve(char * file);
extern long close(long desc);


/* 2个参数的 */
extern long open(char * pathname, unsigned short mode);
extern long creat(char * pathname, unsigned short mode);
extern void _console_write(char *buff, long count);


/* 3个参数的 */
extern long read(long desc, char * buff, unsigned long count);
extern long write(long desc, char * buff, unsigned long count);
extern long lseek(long desc, long offset, unsigned char reference);

/********************************************************************/
#endif /* _LIBS_H_ */