/*
 * Small/kernel/console.c
 * 
 * (C) 2012-2013 Yafei Zheng <e9999e@163.com>
 */
/*
 * 该文件包含控制台处理程序。往控制台写的数据在fs段中。另外，通过
 * 寄存器设置"屏幕起始位置"、"光标位置"时，请记住它们都是相对于显存
 * 开始处的，并不是相对于当前控制台来说的。
 *
 * [??] NOTE! 可以有多个控制台，因此console_write()参数中应指定使用
 * 哪个控制台，但目前还不完善。
 */

/*
#ifndef __DEBUG__
#define __DEBUG__
#endif
*/

#include "types.h"
#include "sys_set.h"
#include "console.h"
#include "string.h"

CONSOLE	console;

/*
 * 设置光标位置。光标位置是依据于显存的，并不是依据于屏幕的。
 */
void set_cur_pos(CONSOLE * cons, unsigned long curpos)
{
	/* 是不可显示处，则返回 */
	if(cur_oo_zone(cons, curpos))
		return;
	cons->cur_pos = curpos;
	curpos /= 2;	/* curpos是光标的内存地址，除以2则可用于设置 */
	out_b(CRT_ADDR_REG, CRT_CUR_HIGH_REG);
	out_b(CRT_DATA_REG, (curpos >> 8) & 0xff);
	out_b(CRT_ADDR_REG, CRT_CUR_LOW_REG);
	out_b(CRT_DATA_REG, curpos & 0xff);
}

/*
 * 根据当前光标位置相对移动光标。offset可正可负可零。
 */
void move_cursor(CONSOLE * cons, long offset)
{
	return set_cur_pos(cons, cons->cur_pos+offset);
}

/*
 * 设置屏幕显示开始地址，相对于显存开始处。
 */
void set_scr_org(CONSOLE * cons, unsigned long org)
{
	org = ALIGN_LINE(org);	/* 行对齐 */
	if(scr_oo_zone(cons, org))
		return;
	cons->scr_org = org;
	org /= 2;	/* org是显示的内存开始地址，除以2则可用于设置 */
	out_b(CRT_ADDR_REG, CRT_START_HIGH_REG);
	out_b(CRT_DATA_REG, (org >> 8) & 0xff);
	out_b(CRT_ADDR_REG, CRT_START_LOW_REG);
	out_b(CRT_DATA_REG, org & 0xff);
}

/*
 * 根据当前屏幕显示开始处，进行偏移调整。offset可正可负可零。
 */
void move_scr_org(CONSOLE * cons, long offset)
{
	return set_scr_org(cons, cons->scr_org+offset);
}

/*
 * 控制台写字符函数，同时移动光标。
 */
void write_char(CONSOLE * cons, char c)
{
	set_gs_word(cons->cur_pos, (SHOW_MODE<<8)|c);
	cons->cur_pos += 2;
	/*
	 * 光标位置超出屏幕范围，则屏幕下滚。由于可能从
	 * cons->org 重新开始，故需调整光标。
	 */
	if(cur_oo_zone(cons, cons->cur_pos)) {
		screen_down(cons);
		set_cur_pos(cons, cons->scr_org+SCR_BYTES-LINE_BYTES);
	} else {
		set_cur_pos(cons, cons->cur_pos);
	}
}

/*
 * 清除一个字符。只清除一行中的字符。
 */
void clean_char(CONSOLE * cons)
{
	if(is_line_align(cons->cur_pos))/* 处于一行的最开始 */
		return;
	move_cursor(cons, -2);
	write_char(cons, CLEAR_CHAR);
	move_cursor(cons, -2);
}

/*
 * 清除一行字符。start - 要清除的那行字符的开始地址.
 */
void clean_line(unsigned long start)
{
	start = ALIGN_LINE(start);	/* 行对齐 */
	for(int i=0; i<CON_COLS; i++) {
		set_gs_word(start+i*2, (SHOW_MODE<<8)|CLEAR_CHAR);
	}
}

/*
 * 屏幕下移一行。也即字符上滚。在这里并没有对光标进行设置。
 */
void screen_down(CONSOLE * cons)
{
	/* 如果下移一行会超出控制台内存限制，则回到控制台开始处 */
	if(scr_oo_zone(cons, cons->scr_org+LINE_BYTES)) {
		gs2gs_memcpy((char *)cons->org, (char *)cons->scr_org+LINE_BYTES,
			SCR_BYTES-LINE_BYTES);
		cons->scr_org = cons->org;
	} else {
		cons->scr_org += LINE_BYTES;
	}
	set_scr_org(cons, cons->scr_org);
	/* 清除屏幕下移之后的最后一行的字符 */
	clean_line(cons->scr_org+SCR_BYTES-LINE_BYTES);
}

/*
 * 控制台写函数。输入缓冲在fs所指段内。
 */
void console_write(char *buff, long count)
{
	if(count <= 0)
		return;

	int index = 0;	/* 写缓冲索引 */
	int i = 0;		/* 临时控制循环变量 */
	char fs_c;
	while(count--) {
		fs_c = get_fs_byte(&buff[index]);
		switch(fs_c) {
		case '\b':	/* 退格 */
			clean_char(&console);
			break;
		case '\t':	/* 水平制表，4字符对齐 */
			i = 4 - ((console.cur_pos/2) % 4);/* NOTE: cur_pos/2 */
			for(; i>0; i--) {
				write_char(&console, CLEAR_CHAR);
			}
			break;
		case '\n':	/* 换行 */
			/* 光标处于最后一行，则屏幕下滚 */
			if(console.cur_pos-console.scr_org >= SCR_BYTES-LINE_BYTES) {
				screen_down(&console);
				set_cur_pos(&console, ALIGN_LINE(console.scr_org+SCR_BYTES-LINE_BYTES));
			} else {
				set_cur_pos(&console, ALIGN_LINE(console.cur_pos+LINE_BYTES));
			}
			break;
		case '\r':	/* 回车 */
			set_cur_pos(&console, ALIGN_LINE(console.cur_pos));
			break;
		default:	/* 写字符 */
			write_char(&console, fs_c);
			break;
		}

		index++;
	}
}

/*
 * 该程序用于初始化一个控制台。
 */
void _console_init(CONSOLE * cons, unsigned long org,
			unsigned long size, unsigned long curpos)
{
	cons->org		= org;
	cons->size		= size;
	cons->scr_org	= org;
	cons->cur_pos	= curpos;
	set_scr_org(cons, cons->scr_org);
	set_cur_pos(cons, cons->cur_pos);
}

void console_init(void)
{
	/* 取得光标位置，光标位置在boot中被保存于0x90000! 注意保持同步! */
	unsigned long pos = (unsigned long)*((unsigned short*)0x90000);
	
	/* 32KB。注意：head.s中显存段描述符需设置为限长32KB */
	_console_init(&console, 0, 32*1024, pos*2);	/* NOTE: pos*2 */
}
