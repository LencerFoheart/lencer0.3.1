/*
 * Small/include/types.h
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

#ifndef _TYPES_H_
#define _TYPES_H_
/********************************************************************/
typedef unsigned char BOOL;
typedef long FILEDESC;	/* 文件描述符，出错时返回-1 */

/*
 * NOTE! TRUE & FALSE & NULL can't be altered!
 */
#define FALSE	0
#define TRUE	1
#ifndef __GCC_LIBRARY__
#define NULL	0
#endif
/********************************************************************/
#endif /* _TYPES_H_ */
