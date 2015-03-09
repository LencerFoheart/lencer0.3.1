/*
 * Small/include/console.h
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "types.h"

/********************************************************************/

/* 控制台结构，用于显示字符 */
typedef struct console_struct {
	unsigned long org;		/* 当前控制台在显存中的开始位置，相对于显存开始处 (bytes) */
	unsigned long size;		/* 当前控制台大小 (bytes) */
	unsigned long scr_org;	/* 屏幕显示开始位置，相对于显存开始处 (bytes) */
	unsigned long cur_pos;	/* 光标位置，相对于显存开始处 (bytes)。注意：是依据于显存的，并不是依据于屏幕的 */
}CONSOLE;

#define CON_LINES	(25)				/* 控制台显示行数 */
#define CON_COLS	(80)				/* 控制台显示列数 */
#define SHOW_MODE	(0x02)				/* 控制台字符显示属性 */
#define CLEAR_CHAR	(0x20)				/* 用于清除字符的字符，即空格 */
#define LINE_BYTES	(CON_COLS*2)
#define SCR_BYTES	(CON_LINES*CON_COLS*2)/* 屏幕所占内存字节数 */

#define ALIGN_LINE(pos) ((pos)/LINE_BYTES*LINE_BYTES)

/* 光标位置超出范围 */
#define cur_oo_zone(cons, curpos) ((curpos)<(cons)->scr_org || (curpos)>=(cons)->scr_org+SCR_BYTES)
/* 屏幕位置超出范围 */
#define scr_oo_zone(cons, scrorg) ((scrorg)<(cons)->org || ((scrorg)+SCR_BYTES)>((cons)->org+(cons)->size))
/* 是否行对齐 */
#define is_line_align(pos) (0==((pos)%LINE_BYTES))


/* VGA显卡相关寄存器，此处是控制台相关的 */
#define CRT_ADDR_REG		(0x3D4)		/* 地址端口 */
#define CRT_DATA_REG		(0x3D5)		/* 数据端口 */
#define CRT_CUR_HIGH_REG	(0x0E)		/* 光标位置高位寄存器 */
#define CRT_CUR_LOW_REG		(0x0F)		/* 光标位置低位寄存器 */
#define CRT_START_HIGH_REG	(0x0C)		/* 屏幕显示开始位置高位寄存器 */
#define CRT_START_LOW_REG	(0x0D)		/* 屏幕显示开始位置低位寄存器 */

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