/*
 * Small/include/stdarg.h
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

/*
 * ���ļ����ڿɱ����������ע�������� VC6.0 ��
 */

#ifndef _STDARG_H_
#define _STDARG_H_
/********************************************************************/

#define _INTSIZEOF(n) ( (sizeof(n)+sizeof(int)-1) & ~(sizeof(int) - 1) )	/* �����ֽڶ��� */

typedef char * va_list;
#define va_start(ap,v) ( ap = (va_list)&(v) + _INTSIZEOF(v) )
#define va_arg(ap,t) ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap) ( ap = (va_list)0 )

/********************************************************************/
#endif /* _STDARG_H_ */