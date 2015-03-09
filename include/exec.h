/*
 * Small/include/exec.h
 * 
 * (C) 2014 Yafei Zheng <e9999e@163.com>
 */

#ifndef _EXEC_H_
#define _EXEC_H_

#include "types.h"

/********************************************************************/

#define MAX_USER_STACK_PAGES	(1)	/* �û�����ջռ�õ����ҳ�� */

/* �����ִ���ļ�����Ϣ */
struct exec_info {
	/* [??] ��ø��Ƶ��ں˿ռ䣬����exec_infoҳ��ɣ���Ϊ�û��ռ�һ���滻���Ͳ����� */
	char * filename;		/* ��ִ���ļ�����ָ���û��ռ� */
	FILEDESC desc;			/* ��ִ���ļ������� */
	unsigned long ventry;	/* ����������ڵ�ַ */
};

extern BOOL sys_execve(char * file);

/********************************************************************/

#endif /* _EXEC_H_ */
