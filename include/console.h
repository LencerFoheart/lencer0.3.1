/*
 * Small/include/console.h
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "types.h"

/********************************************************************/

/* ����̨�ṹ��������ʾ�ַ� */
typedef struct console_struct {
	unsigned long org;		/* ��ǰ����̨���Դ��еĿ�ʼλ�ã�������Դ濪ʼ�� (bytes) */
	unsigned long size;		/* ��ǰ����̨��С (bytes) */
	unsigned long scr_org;	/* ��Ļ��ʾ��ʼλ�ã�������Դ濪ʼ�� (bytes) */
	unsigned long cur_pos;	/* ���λ�ã�������Դ濪ʼ�� (bytes)��ע�⣺���������Դ�ģ���������������Ļ�� */
}CONSOLE;

#define CON_LINES	(25)				/* ����̨��ʾ���� */
#define CON_COLS	(80)				/* ����̨��ʾ���� */
#define SHOW_MODE	(0x02)				/* ����̨�ַ���ʾ���� */
#define CLEAR_CHAR	(0x20)				/* ��������ַ����ַ������ո� */
#define LINE_BYTES	(CON_COLS*2)
#define SCR_BYTES	(CON_LINES*CON_COLS*2)/* ��Ļ��ռ�ڴ��ֽ��� */

#define ALIGN_LINE(pos) ((pos)/LINE_BYTES*LINE_BYTES)

/* ���λ�ó�����Χ */
#define cur_oo_zone(cons, curpos) ((curpos)<(cons)->scr_org || (curpos)>=(cons)->scr_org+SCR_BYTES)
/* ��Ļλ�ó�����Χ */
#define scr_oo_zone(cons, scrorg) ((scrorg)<(cons)->org || ((scrorg)+SCR_BYTES)>((cons)->org+(cons)->size))
/* �Ƿ��ж��� */
#define is_line_align(pos) (0==((pos)%LINE_BYTES))


/* VGA�Կ���ؼĴ������˴��ǿ���̨��ص� */
#define CRT_ADDR_REG		(0x3D4)		/* ��ַ�˿� */
#define CRT_DATA_REG		(0x3D5)		/* ���ݶ˿� */
#define CRT_CUR_HIGH_REG	(0x0E)		/* ���λ�ø�λ�Ĵ��� */
#define CRT_CUR_LOW_REG		(0x0F)		/* ���λ�õ�λ�Ĵ��� */
#define CRT_START_HIGH_REG	(0x0C)		/* ��Ļ��ʾ��ʼλ�ø�λ�Ĵ��� */
#define CRT_START_LOW_REG	(0x0D)		/* ��Ļ��ʾ��ʼλ�õ�λ�Ĵ��� */

/********************************************************************/
extern void set_cur_pos(CONSOLE * cons, unsigned long curpos);
extern void move_cursor(CONSOLE * cons, long offset);
extern void set_scr_org(CONSOLE * cons, unsigned long org);
extern void move_scr_org(CONSOLE * cons, long offset);
extern void write_char(CONSOLE * cons, char c);
extern void clean_char(CONSOLE * cons);
extern void clean_line(unsigned long start);
extern void screen_down(CONSOLE * cons);
extern void console_write(char *buff, long count);
extern void _console_init(CONSOLE * cons, unsigned long org,
			unsigned long size, unsigned long curpos);
extern void console_init(void);

/********************************************************************/
#endif /* _CONSOLE_H_ */